#!/bin/sh
# Builds libwebp_ffi as one shared library with libwebp, libpng, libjpeg-turbo,
# libtiff and zlib linked in statically. CI uses it for the prebuilt gems.
#
# Output in tmp/native/out (or $WEBP_FFI_NATIVE_DIR/out):
#   libwebp_ffi.so or libwebp_ffi.bundle
#   licenses/      license files of the bundled libraries
#   versions.txt   versions of the bundled libraries
#
# Needs a C compiler, cmake, make, curl and tar. nasm is optional: without it
# libjpeg-turbo builds without SIMD on x86_64.
set -eu

ZLIB_VERSION=1.3.2
ZLIB_URL="https://github.com/madler/zlib/releases/download/v${ZLIB_VERSION}/zlib-${ZLIB_VERSION}.tar.gz"
ZLIB_SHA256=bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16

LIBPNG_VERSION=1.6.59
LIBPNG_URL="https://downloads.sourceforge.net/project/libpng/libpng16/${LIBPNG_VERSION}/libpng-${LIBPNG_VERSION}.tar.xz"
LIBPNG_SHA256=d80dd2a38a37f803cb9b6ac7b14bd6e74ddc3b654780a8380bdf93523fdb4389

LIBJPEG_TURBO_VERSION=3.2.1
LIBJPEG_TURBO_URL="https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/${LIBJPEG_TURBO_VERSION}/libjpeg-turbo-${LIBJPEG_TURBO_VERSION}.tar.gz"
LIBJPEG_TURBO_SHA256=c27ad72a68da151177929aa969ae773a2da5ac2af9ce946079314fdac8cece7f

LIBTIFF_VERSION=4.7.2
LIBTIFF_URL="https://download.osgeo.org/libtiff/tiff-${LIBTIFF_VERSION}.tar.gz"
LIBTIFF_SHA256=672bd7d10aee4606171afb864f3570b83340f6a33e2c186dc0512f7145ffdf6a

LIBWEBP_VERSION=1.6.0
LIBWEBP_URL="https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-${LIBWEBP_VERSION}.tar.gz"
LIBWEBP_SHA256=e4ab7009bf0629fd11982d4c2aa83964cf244cffba7347ecd39019a9e38c4564

ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=${WEBP_FFI_NATIVE_DIR:-$ROOT/tmp/native}
SRC=$WORK/src
BUILD=$WORK/build
PREFIX=$WORK/prefix
OUT=$WORK/out
CC=${CC:-cc}
JOBS=${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}

case "$(uname -s)" in
  Darwin)
    LIB_NAME=libwebp_ffi.bundle
    if [ -z "${MACOSX_DEPLOYMENT_TARGET:-}" ]; then
      case "$(uname -m)" in
        arm64) MACOSX_DEPLOYMENT_TARGET=11.0 ;;
        *) MACOSX_DEPLOYMENT_TARGET=10.15 ;;
      esac
    fi
    export MACOSX_DEPLOYMENT_TARGET
    ;;
  Linux)
    LIB_NAME=libwebp_ffi.so
    ;;
  *)
    echo "build-native.sh: unsupported system $(uname -s)" >&2
    exit 1
    ;;
esac

sha256() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | cut -d' ' -f1
  else
    shasum -a 256 "$1" | cut -d' ' -f1
  fi
}

# fetch NAME URL SHA256: download once, verify, unpack into $BUILD/NAME
fetch() {
  file="$SRC/$(basename "$2")"
  if [ ! -f "$file" ]; then
    curl -fsSL --retry 3 -o "$file.part" "$2"
    mv "$file.part" "$file"
  fi
  actual=$(sha256 "$file")
  if [ "$actual" != "$3" ]; then
    echo "build-native.sh: checksum mismatch for $file (expected $3, got $actual)" >&2
    rm -f "$file"
    exit 1
  fi
  rm -rf "${BUILD:?}/$1"
  mkdir -p "$BUILD/$1"
  tar -xf "$file" -C "$BUILD/$1" --strip-components=1
}

# cmake_build NAME [options]: static, position independent, installed into $PREFIX
cmake_build() {
  name=$1
  shift
  cmake -S "$BUILD/$name" -B "$BUILD/$name/_build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    "$@"
  cmake --build "$BUILD/$name/_build" --parallel "$JOBS"
  cmake --install "$BUILD/$name/_build"
}

# copy_licenses NAME FILES...: keep the license texts the bundled libraries require
copy_licenses() {
  name=$1
  shift
  for license in "$@"; do
    cp "$BUILD/$name/$license" "$OUT/licenses/$name-$license"
  done
}

rm -rf "$PREFIX" "$OUT"
mkdir -p "$SRC" "$BUILD" "$PREFIX" "$OUT/licenses"

ZLIB_FLAGS="-DZLIB_LIBRARY=$PREFIX/lib/libz.a -DZLIB_INCLUDE_DIR=$PREFIX/include"

fetch zlib "$ZLIB_URL" "$ZLIB_SHA256"
(cd "$BUILD/zlib" && CFLAGS="-O2 -fPIC" ./configure --static --prefix="$PREFIX" && make -j"$JOBS" install)
copy_licenses zlib LICENSE

fetch libpng "$LIBPNG_URL" "$LIBPNG_SHA256"
# shellcheck disable=SC2086
cmake_build libpng $ZLIB_FLAGS \
  -DPNG_SHARED=OFF -DPNG_STATIC=ON -DPNG_TESTS=OFF -DPNG_TOOLS=OFF -DPNG_FRAMEWORK=OFF
copy_licenses libpng LICENSE

fetch libjpeg-turbo "$LIBJPEG_TURBO_URL" "$LIBJPEG_TURBO_SHA256"
cmake_build libjpeg-turbo \
  -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DWITH_TURBOJPEG=OFF -DWITH_TOOLS=OFF -DWITH_TESTS=OFF
copy_licenses libjpeg-turbo LICENSE.md README.ijg

fetch libtiff "$LIBTIFF_URL" "$LIBTIFF_SHA256"
# shellcheck disable=SC2086
cmake_build libtiff $ZLIB_FLAGS \
  -DJPEG_LIBRARY="$PREFIX/lib/libjpeg.a" -DJPEG_INCLUDE_DIR="$PREFIX/include" \
  -Dtiff-tools=OFF -Dtiff-tests=OFF -Dtiff-contrib=OFF -Dtiff-docs=OFF \
  -Dtiff-cxx=OFF -Dtiff-opengl=OFF -Dtiff-framework=OFF \
  -Dzlib=ON -Djpeg=ON -Dpixarlog=ON \
  -Djbig=OFF -Dlerc=OFF -Dlzma=OFF -Dzstd=OFF -Dwebp=OFF -Dlibdeflate=OFF
copy_licenses libtiff LICENSE.md

fetch libwebp "$LIBWEBP_URL" "$LIBWEBP_SHA256"
cmake_build libwebp \
  -DWEBP_BUILD_ANIM_UTILS=OFF -DWEBP_BUILD_CWEBP=OFF -DWEBP_BUILD_DWEBP=OFF \
  -DWEBP_BUILD_GIF2WEBP=OFF -DWEBP_BUILD_IMG2WEBP=OFF -DWEBP_BUILD_VWEBP=OFF \
  -DWEBP_BUILD_WEBPINFO=OFF -DWEBP_BUILD_LIBWEBPMUX=OFF -DWEBP_BUILD_WEBPMUX=OFF \
  -DWEBP_BUILD_EXTRAS=OFF
copy_licenses libwebp COPYING PATENTS

# Compile the extension against the bundled headers only.
mkdir -p "$BUILD/webp_ffi"
rm -f "$BUILD"/webp_ffi/*.o
for source in "$ROOT"/ext/webp_ffi/*.c; do
  "$CC" -O2 -fPIC -fno-strict-aliasing -I"$PREFIX/include" \
    -c "$source" -o "$BUILD/webp_ffi/$(basename "$source" .c).o"
done

# Libraries go after the code that uses them.
ARCHIVES="$PREFIX/lib/libwebp.a $PREFIX/lib/libsharpyuv.a $PREFIX/lib/libtiff.a $PREFIX/lib/libpng16.a $PREFIX/lib/libjpeg.a $PREFIX/lib/libz.a"
if [ "$LIB_NAME" = "libwebp_ffi.bundle" ]; then
  # shellcheck disable=SC2086
  "$CC" -bundle -o "$OUT/$LIB_NAME" "$BUILD"/webp_ffi/*.o $ARCHIVES -lm -Wl,-dead_strip
  strip -x "$OUT/$LIB_NAME"
  DEPENDENCIES=$(otool -L "$OUT/$LIB_NAME" | tail -n +2)
else
  # --exclude-libs keeps the bundled libraries private, so they never clash
  # with other copies of libpng or libtiff loaded in the same process
  # shellcheck disable=SC2086
  "$CC" -shared -o "$OUT/$LIB_NAME" "$BUILD"/webp_ffi/*.o \
    -Wl,--exclude-libs,ALL -Wl,-soname,libwebp_ffi.so $ARCHIVES -lm -lpthread
  strip --strip-unneeded "$OUT/$LIB_NAME"
  DEPENDENCIES=$(readelf -d "$OUT/$LIB_NAME" | grep NEEDED || true)
fi

echo "Dynamic dependencies of $LIB_NAME:"
echo "$DEPENDENCIES"
if echo "$DEPENDENCIES" | grep -E -i 'webp|png|jpeg|tiff|libz' >/dev/null; then
  echo "build-native.sh: $LIB_NAME still depends on a shared image library" >&2
  exit 1
fi

cat > "$OUT/versions.txt" <<VERSIONS
libwebp $LIBWEBP_VERSION
libpng $LIBPNG_VERSION
libjpeg-turbo $LIBJPEG_TURBO_VERSION
libtiff $LIBTIFF_VERSION
zlib $ZLIB_VERSION
VERSIONS

echo "Built $OUT/$LIB_NAME"

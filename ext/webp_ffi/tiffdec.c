#include "./tiffdec.h"
#include "./util.h"

#include <stdarg.h>
#include <sys/types.h>
#include <tiffio.h>
#include <tiffvers.h>

// libtiff 4.5.0 added error and warning handlers per opened file.
#if defined(TIFFLIB_AT_LEAST)
#if TIFFLIB_AT_LEAST(4, 5, 0)
#define UTIL_TIFF_HAS_OPEN_OPTIONS
#endif
#endif

#ifdef UTIL_TIFF_HAS_OPEN_OPTIONS
// libtiff prints errors and warnings to stderr by default. A library should
// stay quiet. Returning 1 also stops libtiff from calling its global handlers.
static int SilentTIFFHandler(TIFF* tif, void* user_data, const char* module,
                             const char* fmt, va_list ap) {
  (void)tif;
  (void)user_data;
  (void)module;
  (void)fmt;
  (void)ap;
  return 1;
}
#endif

// libtiff reads through these callbacks, so the same code works for files and
// for in-memory data opened with fmemopen.
static tmsize_t UtilTIFFRead(thandle_t handle, void* buffer, tmsize_t size) {
  if (size <= 0) return 0;
  return (tmsize_t)fread(buffer, 1, (size_t)size, (FILE*)handle);
}

static tmsize_t UtilTIFFWrite(thandle_t handle, void* buffer, tmsize_t size) {
  (void)handle;
  (void)buffer;
  (void)size;
  return 0;  // read only
}

static toff_t UtilTIFFSeek(thandle_t handle, toff_t offset, int whence) {
  FILE* const file = (FILE*)handle;
  if (fseeko(file, (off_t)offset, whence) != 0) return (toff_t)-1;
  return (toff_t)ftello(file);
}

static int UtilTIFFClose(thandle_t handle) {
  (void)handle;
  return 0;  // the caller closes the file
}

static toff_t UtilTIFFSize(thandle_t handle) {
  FILE* const file = (FILE*)handle;
  const off_t position = ftello(file);
  off_t size;
  if (position < 0 || fseeko(file, 0, SEEK_END) != 0) return 0;
  size = ftello(file);
  fseeko(file, position, SEEK_SET);
  return size < 0 ? 0 : (toff_t)size;
}

static int UtilTIFFMap(thandle_t handle, void** base, toff_t* size) {
  (void)handle;
  (void)base;
  (void)size;
  return 0;  // no memory mapping
}

static void UtilTIFFUnmap(thandle_t handle, void* base, toff_t size) {
  (void)handle;
  (void)base;
  (void)size;
}

// "m" turns off memory mapping, which a FILE* can't provide
static TIFF* OpenTIFF(FILE* const file) {
#ifdef UTIL_TIFF_HAS_OPEN_OPTIONS
  TIFF* tif;
  TIFFOpenOptions* const opts = TIFFOpenOptionsAlloc();
  if (opts == NULL) return NULL;
  TIFFOpenOptionsSetErrorHandlerExtR(opts, SilentTIFFHandler, NULL);
  TIFFOpenOptionsSetWarningHandlerExtR(opts, SilentTIFFHandler, NULL);
  tif = TIFFClientOpenExt("webp-ffi", "rm", (thandle_t)file,
                          UtilTIFFRead, UtilTIFFWrite, UtilTIFFSeek, UtilTIFFClose,
                          UtilTIFFSize, UtilTIFFMap, UtilTIFFUnmap, opts);
  TIFFOpenOptionsFree(opts);
  return tif;
#else
  // Older libtiff only has process-wide handlers. Changing those would affect
  // other code in the same process, so its messages still reach stderr.
  return TIFFClientOpen("webp-ffi", "rm", (thandle_t)file,
                        UtilTIFFRead, UtilTIFFWrite, UtilTIFFSeek, UtilTIFFClose,
                        UtilTIFFSize, UtilTIFFMap, UtilTIFFUnmap);
#endif
}

const char* UtilTIFFVersion(void) {
  return TIFFGetVersion();
}

// libtiff's RGBA interface always returns premultiplied (associated) alpha,
// whatever kind of alpha the file stores, so undo the multiplication.
// Based on MultARGBRow in libwebp's imageio/tiffdec.c (taken from libwebp's
// dsp/alpha_processing).
#define MFIX 24    // 24bit fixed-point arithmetic
#define HALF ((1u << MFIX) >> 1)

static uint32_t Unmult(uint8_t x, uint32_t mult) {
  const uint32_t v = (x * mult + HALF) >> MFIX;
  return (v > 255u) ? 255u : v;
}

static uint32_t GetScale(uint32_t a) {
  return (255u << MFIX) / a;
}

#undef MFIX
#undef HALF

static void UnmultiplyRow(uint8_t* ptr, uint32_t width) {
  uint32_t x;
  for (x = 0; x < width; ++x, ptr += 4) {
    const uint32_t alpha = ptr[3];
    if (alpha < 255) {
      if (alpha == 0) {
        ptr[0] = ptr[1] = ptr[2] = 0;
      } else {
        const uint32_t scale = GetScale(alpha);
        ptr[0] = (uint8_t)Unmult(ptr[0], scale);
        ptr[1] = (uint8_t)Unmult(ptr[1], scale);
        ptr[2] = (uint8_t)Unmult(ptr[2], scale);
      }
    }
  }
}

// True when the first extra sample is alpha, associated or not.
static int HasAlphaSample(TIFF* const tif) {
  uint16_t count = 0;
  uint16_t* samples = NULL;
  return TIFFGetField(tif, TIFFTAG_EXTRASAMPLES, &count, &samples) && count > 0 &&
         samples != NULL && (samples[0] == EXTRASAMPLE_ASSOCALPHA ||
                             samples[0] == EXTRASAMPLE_UNASSALPHA);
}

static int ExtractMetadataFromTIFF(TIFF* const tif, Metadata* const metadata) {
  uint32_t length = 0;
  void* data = NULL;
  if (TIFFGetField(tif, TIFFTAG_ICCPROFILE, &length, &data) &&
      !MetadataCopy(data, length, &metadata->iccp)) {
    return 0;
  }
  length = 0;
  data = NULL;
  if (TIFFGetField(tif, TIFFTAG_XMLPACKET, &length, &data) &&
      !MetadataCopy(data, length, &metadata->xmp)) {
    return 0;
  }
  return 1;
}

int UtilReadTIFF(FILE* const in_file,
                    WebPPicture* const pic, int keep_alpha,
                    Metadata* const metadata) {
  TIFF* const tif = OpenTIFF(in_file);
  uint32_t width, height;
  uint32_t* raster;
  int stride;
  size_t raster_size;
  int ok = 0;

  if (tif == NULL) {
    return 0;
  }

  // Multi-page TIFF files are not supported: only the first page is used.
  if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width) ||
      !TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height) ||
      !UtilCheckImageSize(width, height, sizeof(*raster),
                          &stride, &raster_size) ||
      (metadata != NULL && !ExtractMetadataFromTIFF(tif, metadata))) {
    TIFFClose(tif);
    return 0;
  }

  raster = (uint32_t*)_TIFFmalloc(raster_size);
  if (raster != NULL) {
    if (TIFFReadRGBAImageOriented(tif, width, height, raster,
                                  ORIENTATION_TOPLEFT, 1)) {
      pic->width = (int)width;
      pic->height = (int)height;
      // TIFF data is ABGR
#ifdef __BIG_ENDIAN__
      TIFFSwabArrayOfLong(raster, raster_size / sizeof(*raster));
#endif
      if (keep_alpha && HasAlphaSample(tif)) {
        uint32_t y;
        for (y = 0; y < height; ++y) {
          UnmultiplyRow((uint8_t*)raster + (size_t)y * stride, width);
        }
      }
      ok = keep_alpha
         ? WebPPictureImportRGBA(pic, (const uint8_t*)raster, stride)
         : WebPPictureImportRGBX(pic, (const uint8_t*)raster, stride);
    }
    _TIFFfree(raster);
  }

  if (ok && keep_alpha == 2) {
    WebPCleanupTransparentArea(pic);
  }

  TIFFClose(tif);
  return ok;
}



static void PutLE16(uint8_t* const dst, uint32_t value) {
  dst[0] = (value >> 0) & 0xff;
  dst[1] = (value >> 8) & 0xff;
}

static void PutLE32(uint8_t* const dst, uint32_t value) {
  PutLE16(dst + 0, (value >>  0) & 0xffff);
  PutLE16(dst + 2, (value >> 16) & 0xffff);
}


#define NUM_IFD_ENTRIES 15
#define EXTRA_DATA_SIZE 16
// 10b for signature/header + n * 12b entries + 4b for IFD terminator:
#define EXTRA_DATA_OFFSET (10 + 12 * NUM_IFD_ENTRIES + 4)
#define TIFF_HEADER_SIZE (EXTRA_DATA_OFFSET + EXTRA_DATA_SIZE)

int UtilWriteTIFF(FILE* fout, const WebPDecBuffer* const buffer) {
  const int has_alpha = (buffer->colorspace != MODE_RGB);
  const uint32_t width = buffer->width;
  const uint32_t height = buffer->height;
  const uint8_t* const rgba = buffer->u.RGBA.rgba;
  const int stride = buffer->u.RGBA.stride;
  const uint8_t bytes_per_px = has_alpha ? 4 : 3;
  // For non-alpha case, we omit tag 0x152 (ExtraSamples).
  const uint8_t num_ifd_entries = has_alpha ? NUM_IFD_ENTRIES
                                            : NUM_IFD_ENTRIES - 1;
  uint8_t tiff_header[TIFF_HEADER_SIZE] = {
    0x49, 0x49, 0x2a, 0x00,   // little endian signature
    8, 0, 0, 0,               // offset to the unique IFD that follows
    // IFD (offset = 8). Entries must be written in increasing tag order.
    num_ifd_entries, 0,       // Number of entries in the IFD (12 bytes each).
    0x00, 0x01, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0,    //  10: Width  (TBD)
    0x01, 0x01, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0,    //  22: Height (TBD)
    0x02, 0x01, 3, 0, bytes_per_px, 0, 0, 0,     //  34: BitsPerSample: 8888
        EXTRA_DATA_OFFSET + 0, 0, 0, 0,
    0x03, 0x01, 3, 0, 1, 0, 0, 0, 1, 0, 0, 0,    //  46: Compression: none
    0x06, 0x01, 3, 0, 1, 0, 0, 0, 2, 0, 0, 0,    //  58: Photometric: RGB
    0x11, 0x01, 4, 0, 1, 0, 0, 0,                //  70: Strips offset:
        TIFF_HEADER_SIZE, 0, 0, 0,               //      data follows header
    0x12, 0x01, 3, 0, 1, 0, 0, 0, 1, 0, 0, 0,    //  82: Orientation: topleft
    0x15, 0x01, 3, 0, 1, 0, 0, 0,                //  94: SamplesPerPixels
        bytes_per_px, 0, 0, 0,
    0x16, 0x01, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0,    // 106: Rows per strip (TBD)
    0x17, 0x01, 4, 0, 1, 0, 0, 0, 0, 0, 0, 0,    // 118: StripByteCount (TBD)
    0x1a, 0x01, 5, 0, 1, 0, 0, 0,                // 130: X-resolution
        EXTRA_DATA_OFFSET + 8, 0, 0, 0,
    0x1b, 0x01, 5, 0, 1, 0, 0, 0,                // 142: Y-resolution
        EXTRA_DATA_OFFSET + 8, 0, 0, 0,
    0x1c, 0x01, 3, 0, 1, 0, 0, 0, 1, 0, 0, 0,    // 154: PlanarConfiguration
    0x28, 0x01, 3, 0, 1, 0, 0, 0, 2, 0, 0, 0,    // 166: ResolutionUnit (inch)
    0x52, 0x01, 3, 0, 1, 0, 0, 0, 1, 0, 0, 0,    // 178: ExtraSamples: rgbA
    0, 0, 0, 0,                                  // 190: IFD terminator
    // EXTRA_DATA_OFFSET:
    8, 0, 8, 0, 8, 0, 8, 0,      // BitsPerSample
    72, 0, 0, 0, 1, 0, 0, 0      // 72 pixels/inch, for X/Y-resolution
  };
  uint32_t y;

  // Fill placeholders in IFD:
  PutLE32(tiff_header + 10 + 8, width);
  PutLE32(tiff_header + 22 + 8, height);
  PutLE32(tiff_header + 106 + 8, height);
  PutLE32(tiff_header + 118 + 8, width * bytes_per_px * height);
  if (!has_alpha) PutLE32(tiff_header + 178, 0);  // IFD terminator

  // write header
  if (fwrite(tiff_header, sizeof(tiff_header), 1, fout) != 1) {
    return 0;
  }
  // write pixel values
  for (y = 0; y < height; ++y) {
    if (fwrite(rgba + y * stride, bytes_per_px, width, fout) != width) {
      return 0;
    }
  }

  return 1;
}

#undef TIFF_HEADER_SIZE
#undef EXTRA_DATA_OFFSET
#undef EXTRA_DATA_SIZE
#undef NUM_IFD_ENTRIES

#define BMP_HEADER_SIZE 54
int UtilWriteBMP(FILE* fout, const WebPDecBuffer* const buffer) {
  const int has_alpha = (buffer->colorspace != MODE_BGR);
  const uint32_t width = buffer->width;
  const uint32_t height = buffer->height;
  const uint8_t* const rgba = buffer->u.RGBA.rgba;
  const int stride = buffer->u.RGBA.stride;
  const uint32_t bytes_per_px = has_alpha ? 4 : 3;
  uint32_t y;
  const uint32_t line_size = bytes_per_px * width;
  const uint32_t bmp_stride = (line_size + 3) & ~3;   // pad to 4
  const uint32_t total_size = bmp_stride * height + BMP_HEADER_SIZE;
  uint8_t bmp_header[BMP_HEADER_SIZE] = { 0 };

  // bitmap file header
  PutLE16(bmp_header + 0, 0x4d42);                // signature 'BM'
  PutLE32(bmp_header + 2, total_size);            // size including header
  PutLE32(bmp_header + 6, 0);                     // reserved
  PutLE32(bmp_header + 10, BMP_HEADER_SIZE);      // offset to pixel array
  // bitmap info header
  PutLE32(bmp_header + 14, 40);                   // DIB header size
  PutLE32(bmp_header + 18, width);                // dimensions
  PutLE32(bmp_header + 22, -(int)height);         // vertical flip!
  PutLE16(bmp_header + 26, 1);                    // number of planes
  PutLE16(bmp_header + 28, bytes_per_px * 8);     // bits per pixel
  PutLE32(bmp_header + 30, 0);                    // no compression (BI_RGB)
  PutLE32(bmp_header + 34, 0);                    // image size (dummy)
  PutLE32(bmp_header + 38, 2400);                 // x pixels/meter
  PutLE32(bmp_header + 42, 2400);                 // y pixels/meter
  PutLE32(bmp_header + 46, 0);                    // number of palette colors
  PutLE32(bmp_header + 50, 0);                    // important color count

  // TODO(skal): color profile

  // write header
  if (fwrite(bmp_header, sizeof(bmp_header), 1, fout) != 1) {
    return 0;
  }

  // write pixel array
  for (y = 0; y < height; ++y) {
    if (fwrite(rgba + y * stride, line_size, 1, fout) != 1) {
      return 0;
    }
    // write padding zeroes
    if (bmp_stride != line_size) {
      const uint8_t zeroes[3] = { 0 };
      if (fwrite(zeroes, bmp_stride - line_size, 1, fout) != 1) {
        return 0;
      }
    }
  }
  return 1;
}
#undef BMP_HEADER_SIZE

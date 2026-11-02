#!/usr/bin/env sh
set -e
wget https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-$LIBWEBP_VERSION.tar.gz
tar xvzf libwebp-$LIBWEBP_VERSION.tar.gz
cd libwebp-$LIBWEBP_VERSION
./configure --prefix=$HOME/opt
make && make install

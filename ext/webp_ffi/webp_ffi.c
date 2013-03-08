#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "webp/decode.h"
#include "webp/encode.h"

// utils
#include "./util.h"
#include "./webp_ffi.h"

#ifdef WEBP_HAVE_PNG
#include <png.h>
#endif

#ifdef WEBP_HAVE_JPEG
#include <setjmp.h>   // note: this must be included *after* png.h
#include <jpeglib.h>
#endif

#ifdef WEBP_HAVE_TIFF
#include <tiffio.h>
#endif


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

void decoder_version(char *version) {
  int v = WebPGetDecoderVersion();
  sprintf(version, "%d.%d.%d",
    (v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff);
}

void encoder_version(char *version) {
  int v = WebPGetEncoderVersion();
  sprintf(version, "%d.%d.%d",
    (v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff);
}

// test
int test(int n) {
  return n + 100;
}


#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
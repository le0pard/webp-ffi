#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

const char *decoder_version(void) {
  const char buffer[20];
  const int version = WebPGetDecoderVersion();
  sprintf(buffer, "%d.%d.%d",
    (version >> 16) & 0xff, (version >> 8) & 0xff, version & 0xff);
  return buffer;
}

const char *encoder_version(void) {
  const char buffer[20];
  const int version = WebPGetEncoderVersion();
  sprintf(buffer, "%d.%d.%d",
    (version >> 16) & 0xff, (version >> 8) & 0xff, version & 0xff);
  return buffer;
}

// test
int test(int n) {
  return n + 100;
}


#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
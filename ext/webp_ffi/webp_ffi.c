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

// utils
typedef enum {
  PNG_ = 0,
  JPEG_,
  TIFF_,  // 'TIFF' clashes with libtiff
  UNSUPPORTED
} InputFileFormat;

static InputFileFormat GetImageType(const uint8_t* buf) {
  InputFileFormat format = UNSUPPORTED;
  unsigned int magic;

  magic = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
  if (magic == 0x89504E47U) {
    format = PNG_;
  } else if (magic >= 0xFFD8FF00U && magic <= 0xFFD8FFFFU) {
    format = JPEG_;
  } else if (magic == 0x49492A00 || magic == 0x4D4D002A) {
    format = TIFF_;
  }
  return format;
}

// main functions

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

int webp_get_info(const uint8_t* data, size_t data_size, int* width, int* height) {
  return WebPGetInfo(data, data_size, width, height);
}

size_t webp_encode_rgb(const uint8_t* rgb, int width, int height, int stride, float quality_factor, uint8_t** output) {
  return WebPEncodeRGB(rgb, width, height, stride, quality_factor, output);
}

int webp_encode(const uint8_t* data, size_t data_size, const WebPConfig* config, uint8_t** output) {
  int return_value = -1;
  WebPPicture picture;
  // NOT finished
  return return_value;
}

// test
int test(int n) {
  return n + 100;
}


#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
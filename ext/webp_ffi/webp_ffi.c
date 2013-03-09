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

int webp_encode(const uint8_t* data, size_t data_size, const FfiWebpConfig* ffi_config, uint8_t** output) {
  int return_value = -1;
  WebPPicture picture;
  WebPConfig config;
  if (!WebPPictureInit(&picture) ||
      !WebPConfigInit(&config)) {
    fprintf(stderr, "Error! Version mismatch!\n");
    return -1;
  }
  
  config.sns_strength = 90;
  config.filter_sharpness = 6;
  config.alpha_quality = 90;
  
  if (!WebPValidateConfig(&config)) {
    fprintf(stderr, "Error! Invalid configuration.\n");
    return -1;
  }
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
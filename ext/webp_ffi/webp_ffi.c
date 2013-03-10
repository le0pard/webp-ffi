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

typedef enum {
  PNG = 0,
  PAM,
  PPM,
  PGM,
  ALPHA_PLANE_ONLY  // this is for experimenting only
} OutputFileFormat;

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

int webp_decode(const uint8_t* data, size_t data_size, uint8_t* output) {
  int return_value = -1;
  WebPDecoderConfig config;
  WebPDecBuffer* const output_buffer = &config.output;
  WebPBitstreamFeatures* const bitstream = &config.input;
  VP8StatusCode status = VP8_STATUS_OK;
  OutputFileFormat format = PNG;
  int ok;

  if (!WebPInitDecoderConfig(&config)) {
    fprintf(stderr, "Library version mismatch!\n");
    return -1;
  }
  
  status = WebPGetFeatures(data, data_size, bitstream);
  if (status != VP8_STATUS_OK) {
    fprintf(stderr, "Library WebPGetFeatures problem!\n");
    return -1;
  }
  
  switch (format) {
    case PNG:
      output_buffer->colorspace = bitstream->has_alpha ? MODE_RGBA : MODE_RGB;
      break;
    case PAM:
      output_buffer->colorspace = MODE_RGBA;
      break;
    case PPM:
      output_buffer->colorspace = MODE_RGB;  // drops alpha for PPM
      break;
    case PGM:
      output_buffer->colorspace = bitstream->has_alpha ? MODE_YUVA : MODE_YUV;
      break;
    case ALPHA_PLANE_ONLY:
      output_buffer->colorspace = MODE_YUVA;
      break;
    default:
      return -1;
  }
  
  int bufferSize = config.input.width * config.input.height * 4;
  config.output.u.RGBA.stride = config.input.width * 4;
  config.output.u.RGBA.size = bufferSize;
  config.output.is_external_memory = 1;
  config.output.u.RGBA.rgba = (uint8_t*)output;
  
  status = WebPDecode(data, data_size, &config);
  ok = (status == VP8_STATUS_OK);
  if (!ok) {
    fprintf(stderr, "Decoding failed, %i.\n", status);
    return -1;
  }
  WebPFreeDecBuffer(output_buffer);
  return return_value;
}

int webp_encode(const uint8_t* data, size_t data_size, const FfiWebpConfig* ffi_config, uint8_t* output) {
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "webp/decode.h"
#include "webp/encode.h"

// utils
#include "./util.h"
#include "./webp_ffi.h"


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



int webp_decode(const char *in_file, const char *out_file) {
  WebPDecoderConfig config;
  WebPDecBuffer* const output_buffer = &config.output;
  WebPBitstreamFeatures* const bitstream = &config.input;
  OutputFileFormat format = PNG;

  if (!WebPInitDecoderConfig(&config)) {
    fprintf(stderr, "Library version mismatch!\n");
    return -1;
  }
  
  VP8StatusCode status = VP8_STATUS_OK;
  size_t data_size = 0;
  const uint8_t* data = NULL;
  
  if (!UtilReadFile(in_file, &data, &data_size)) return -1;
  
  status = WebPGetFeatures(data, data_size, bitstream);
  if (status != VP8_STATUS_OK) {
    fprintf(stderr, "This is invalid webp image!\n");
    free((void*)data);
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
      free((void*)data);
      return -1;
  }
  status = WebPDecode(data, data_size, &config);
  
  free((void*)data);
  if (status != VP8_STATUS_OK) {
    fprintf(stderr, "Decoding of %s failed.\n", in_file);
    return -1;
  }
  printf("Decoded %s. Dimensions: %d x %d%s. Now saving...\n", in_file,
         output_buffer->width, output_buffer->height,
         bitstream->has_alpha ? " (with alpha)" : "");
  UtilSaveOutput(output_buffer, format, out_file);
  WebPFreeDecBuffer(output_buffer);
  return 0;
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
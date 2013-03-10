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

static int EncodeWriter(const uint8_t* data, size_t data_size,
                    const WebPPicture* const pic) {
  FILE* const out = (FILE*)pic->custom_ptr;
  return data_size ? (fwrite(data, data_size, 1, out) == 1) : 1;
}

static void AllocExtraInfo(WebPPicture* const pic) {
  const int mb_w = (pic->width + 15) / 16;
  const int mb_h = (pic->height + 15) / 16;
  pic->extra_info = (uint8_t*)malloc(mb_w * mb_h * sizeof(*pic->extra_info));
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
  if (WebPGetInfo(data, data_size, width, height) == 1) {
    return 0;
  } else {
    return 1;
  }
}



int webp_decode(const char *in_file, const char *out_file) {
  WebPDecoderConfig config;
  WebPDecBuffer* const output_buffer = &config.output;
  WebPBitstreamFeatures* const bitstream = &config.input;
  OutputFileFormat format = PNG;

  if (!WebPInitDecoderConfig(&config)) {
    fprintf(stderr, "Library version mismatch!\n");
    return 1;
  }
  
  VP8StatusCode status = VP8_STATUS_OK;
  size_t data_size = 0;
  const uint8_t* data = NULL;
  
  if (!UtilReadFile(in_file, &data, &data_size)) return -1;
  
  status = WebPGetFeatures(data, data_size, bitstream);
  if (status != VP8_STATUS_OK) {
    fprintf(stderr, "This is invalid webp image!\n");
    free((void*)data);
    return 2;
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
      return 3;
  }
  status = WebPDecode(data, data_size, &config);
  
  free((void*)data);
  if (status != VP8_STATUS_OK) {
    fprintf(stderr, "Decoding of %s failed.\n", in_file);
    return 4;
  }
  UtilSaveOutput(output_buffer, format, out_file);
  WebPFreeDecBuffer(output_buffer);
  return 0;
}



int webp_encode(const char *in_file, const char *out_file) {
  FILE *out = NULL;
  int keep_alpha = 1;
  WebPPicture picture;
  WebPConfig config;
  
  
  if (!WebPPictureInit(&picture) ||
      !WebPConfigInit(&config)) {
    fprintf(stderr, "Error! Version mismatch!\n");
    return 1;
  }
  
  if (!WebPValidateConfig(&config)) {
    fprintf(stderr, "Error! Invalid configuration.\n");
    return 2;
  }
  
  if (!UtilReadPicture(in_file, &picture, keep_alpha)) {
    fprintf(stderr, "Error! Cannot read input picture file '%s'\n", in_file);
    return 3;
  }
  
  out = fopen(out_file, "wb");
  if (out == NULL) {
    fprintf(stderr, "Error! Cannot open output file '%s'\n", out_file);
    return 5;
  }
  picture.writer = EncodeWriter;
  picture.custom_ptr = (void*)out;
  
  if (picture.extra_info_type > 0) {
    AllocExtraInfo(&picture);
  }
  
  if (!WebPEncode(&config, &picture)) {
    fprintf(stderr, "Error! Cannot encode picture as WebP\n");
    return 4;
  }
  
  free(picture.extra_info);
  WebPPictureFree(&picture);
  if (out != NULL) {
    fclose(out);
  }

  return 0;
}

// test
int test(int n) {
  return n + 100;
}


#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
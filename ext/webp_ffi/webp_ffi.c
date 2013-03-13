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



int webp_encode(const char *in_file, const char *out_file, const FfiWebpEncodeConfig *encode_config) {
  int return_value = -1;
  FILE *out = NULL;
  int keep_alpha = 1;
  WebPPicture picture;
  WebPConfig config;
  // OPTIONS BEGIN
  if (encode_config->lossless == 0 || encode_config->lossless == 1){
    config.lossless = encode_config->lossless;
  }
  if (encode_config->quality >= 0 && encode_config->quality <= 100){
    config.quality = encode_config->quality;
  }
  if (encode_config->method >= 0 && encode_config->method <= 6){
    config.method = encode_config->method;
  }
  if (encode_config->target_size > 0){
    config.target_size = encode_config->target_size;
  }
  if (encode_config->target_PSNR > 0){
    config.target_PSNR = encode_config->target_PSNR;
  }
  if (encode_config->segments >= 0 && encode_config->segments <= 4){
    config.segments = encode_config->segments;
  }
  if (encode_config->sns_strength >= 0 && encode_config->sns_strength <= 100){
    config.sns_strength = encode_config->sns_strength;
  }
  if (encode_config->filter_strength >= 0 && encode_config->filter_strength <= 100){
    config.filter_strength = encode_config->filter_strength;
  }
  if (encode_config->filter_sharpness >= 0 && encode_config->filter_sharpness <= 7){
    config.filter_sharpness = encode_config->filter_sharpness;
  }
  if (encode_config->filter_type == 0 || encode_config->filter_type == 1){
    config.filter_type = encode_config->filter_type;
  }
  if (encode_config->autofilter == 0 || encode_config->autofilter == 1){
    config.autofilter = encode_config->autofilter;
  }
  if (encode_config->alpha_compression == 0 || encode_config->alpha_compression == 1){
    config.alpha_compression = encode_config->alpha_compression;
  }
  if (encode_config->alpha_filtering >= 0 && encode_config->alpha_filtering <= 2){
    config.alpha_filtering = encode_config->alpha_filtering;
  }
  if (encode_config->alpha_quality >= 0 && encode_config->alpha_quality <= 100){
    config.alpha_quality = encode_config->alpha_quality;
  }
  if (encode_config->pass >= 0 && encode_config->pass <= 10){
    config.pass = encode_config->pass;
  }
  if (encode_config->show_compressed >= 0){
    config.show_compressed = encode_config->show_compressed;
  }
  if (encode_config->preprocessing == 0 || encode_config->preprocessing == 1){
    config.preprocessing = encode_config->preprocessing;
  }
  if (encode_config->partitions >= 0 && encode_config->partitions <= 3){
    config.partitions = encode_config->partitions;
  }
  if (encode_config->partition_limit >= 0 && encode_config->partition_limit <= 100){
    config.partition_limit = encode_config->partition_limit;
  }
  if ((encode_config->width | encode_config->height) > 0){
    picture.width = encode_config->width;
    picture.height = encode_config->height;
  }
  // OPTIONS END
  
  if (!WebPPictureInit(&picture) ||
      !WebPConfigInit(&config)) {
    //fprintf(stderr, "Error! Version mismatch!\n");
    return 1;
  }
  
  if (!WebPValidateConfig(&config)) {
    //fprintf(stderr, "Error! Invalid configuration.\n");
    return_value = 2;
    goto Error;
  }
  
  if (!UtilReadPicture(in_file, &picture, keep_alpha)) {
    //fprintf(stderr, "Error! Cannot read input picture file '%s'\n", in_file);
    return_value = 3;
    goto Error;
  }
  
  out = fopen(out_file, "wb");
  if (out == NULL) {
    //fprintf(stderr, "Error! Cannot open output file '%s'\n", out_file);
    return_value = 4;
    goto Error;
  }
  picture.writer = EncodeWriter;
  picture.custom_ptr = (void*)out;
  
  if ((encode_config->crop_w | encode_config->crop_h) > 0){
    if (!WebPPictureView(&picture, encode_config->crop_x, encode_config->crop_y, encode_config->crop_w, encode_config->crop_h, &picture)) {
      //fprintf(stderr, "Error! Cannot crop picture\n");
      return_value = 5;
      goto Error;
    }
  }
  
  if ((encode_config->resize_w | encode_config->resize_h) > 0) {
    if (!WebPPictureRescale(&picture, encode_config->resize_w, encode_config->resize_h)) {
      //fprintf(stderr, "Error! Cannot resize picture\n");
      return_value = 6;
      goto Error;
    }
  }
  
  if (picture.extra_info_type > 0) {
    AllocExtraInfo(&picture);
  }
  
  if (!WebPEncode(&config, &picture)) {
    //fprintf(stderr, "Error! Cannot encode picture as WebP\n");
    return_value = 7;
    goto Error;
  }
  return_value = 0;
  
Error:
  free(picture.extra_info);
  WebPPictureFree(&picture);
  if (out != NULL) {
    fclose(out);
  }

  return return_value;
}



int webp_decode(const char *in_file, const char *out_file, const FfiWebpDecodeConfig *decode_config) {
  int return_value = -1;
  WebPDecoderConfig config;
  WebPDecBuffer* const output_buffer = &config.output;
  WebPBitstreamFeatures* const bitstream = &config.input;
  OutputFileFormat format = PNG;

  if (!WebPInitDecoderConfig(&config)) {
    //fprintf(stderr, "Library version mismatch!\n");
    return 1;
  }
  
  if (decode_config->output_format != format){
    format = decode_config->output_format;
  }
  if (decode_config->no_fancy_upsampling > 0){
    config.options.no_fancy_upsampling = 1;
  }
  if (decode_config->bypass_filtering > 0){
    config.options.bypass_filtering = 1;
  }
  if (decode_config->use_threads > 0){
    config.options.use_threads = 1;
  }
  if ((decode_config->crop_w | decode_config->crop_h) > 0){
    config.options.use_cropping = 1;
    config.options.crop_left   = decode_config->crop_x;
    config.options.crop_top    = decode_config->crop_y;
    config.options.crop_width  = decode_config->crop_w;
    config.options.crop_height = decode_config->crop_h;
  }
  if ((decode_config->resize_w | decode_config->resize_h) > 0){
    config.options.use_scaling = 1;
    config.options.scaled_width  = decode_config->resize_w;
    config.options.scaled_height = decode_config->resize_h;
  }
  
  VP8StatusCode status = VP8_STATUS_OK;
  size_t data_size = 0;
  const uint8_t* data = NULL;
  
  if (!UtilReadFile(in_file, &data, &data_size)) return -1;
  
  status = WebPGetFeatures(data, data_size, bitstream);
  if (status != VP8_STATUS_OK) {
    //fprintf(stderr, "This is invalid webp image!\n");
    return_value = 2;
    goto Error;
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
  
  if (status != VP8_STATUS_OK) {
    //fprintf(stderr, "Decoding of %s failed.\n", in_file);
    return_value = 4;
    goto Error;
  }
  UtilSaveOutput(output_buffer, format, out_file);
  return_value = 0;
  
Error:  
  free((void*)data);
  WebPFreeDecBuffer(output_buffer);
  return return_value;
}


// test
int test_c(int n) {
  return n + 100;
}


#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
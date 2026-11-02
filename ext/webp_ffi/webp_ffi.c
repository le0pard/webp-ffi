#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// utils
#include "./util.h"
#include "./webp_ffi.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// Error codes returned to Ruby. lib/webp/error.rb has the messages.
enum {
  ENCODE_VERSION_MISMATCH = 1,
  ENCODE_INVALID_CONFIG = 2,
  ENCODE_CANNOT_READ_INPUT = 3,
  ENCODE_CANNOT_OPEN_OUTPUT = 4,
  ENCODE_CANNOT_CROP = 5,
  ENCODE_CANNOT_RESIZE = 6,
  ENCODE_FAILED = 7,
  ENCODE_OUT_OF_MEMORY = 8
};

enum {
  DECODE_VERSION_MISMATCH = 1,
  DECODE_INVALID_WEBP = 2,
  DECODE_INVALID_FORMAT = 3,
  DECODE_FAILED = 4,
  DECODE_CANNOT_READ_INPUT = 5,
  DECODE_CANNOT_WRITE_OUTPUT = 6,
  DECODE_OUT_OF_MEMORY = 7
};

static int EncodeWriter(const uint8_t* data, size_t data_size,
                    const WebPPicture* const pic) {
  FILE* const out = (FILE*)pic->custom_ptr;
  return data_size ? (fwrite(data, data_size, 1, out) == 1) : 1;
}

// main functions

// Versions are packed as 0xMMmmpp (major, minor, patch).
int decoder_version(void) {
  return WebPGetDecoderVersion();
}

int encoder_version(void) {
  return WebPGetEncoderVersion();
}

int webp_get_info(const uint8_t* data, size_t data_size, int* width, int* height) {
  if (WebPGetInfo(data, data_size, width, height) == 1) {
    return 0;
  } else {
    return 1;
  }
}

// Frees the output of webp_encode_data and webp_decode_data.
void webp_free_data(uint8_t* data) {
  free(data);
}

// Initializes 'config' and 'picture' and applies the options from Ruby.
static int SetupEncoder(const FfiWebpEncodeConfig* const encode_config,
                        WebPConfig* const config, WebPPicture* const picture) {
  if (!WebPPictureInit(picture) || !WebPConfigInit(config)) {
    return ENCODE_VERSION_MISMATCH;
  }

  if (encode_config->lossless == 0 || encode_config->lossless == 1){
    config->lossless = encode_config->lossless;
  }

#if WEBP_ENCODER_ABI_VERSION >= 0x0209  // near_lossless was added in libwebp 0.5.0
  if (encode_config->near_lossless >= 0 && encode_config->near_lossless <= 100){
    config->near_lossless = encode_config->near_lossless;
    config->lossless = 1;  // use near-lossless only with lossless
  }
#endif

  if (encode_config->quality >= 0 && encode_config->quality <= 100){
    config->quality = encode_config->quality;
  }
  if (encode_config->method >= 0 && encode_config->method <= 6){
    config->method = encode_config->method;
  }
  if (encode_config->target_size > 0){
    config->target_size = encode_config->target_size;
  }
  if (encode_config->target_PSNR > 0){
    config->target_PSNR = encode_config->target_PSNR;
  }
  if (encode_config->segments >= 0 && encode_config->segments <= 4){
    config->segments = encode_config->segments;
  }
  if (encode_config->sns_strength >= 0 && encode_config->sns_strength <= 100){
    config->sns_strength = encode_config->sns_strength;
  }
  if (encode_config->filter_strength >= 0 && encode_config->filter_strength <= 100){
    config->filter_strength = encode_config->filter_strength;
  }
  if (encode_config->filter_sharpness >= 0 && encode_config->filter_sharpness <= 7){
    config->filter_sharpness = encode_config->filter_sharpness;
  }
  if (encode_config->filter_type == 0 || encode_config->filter_type == 1){
    config->filter_type = encode_config->filter_type;
  }
  if (encode_config->autofilter == 0 || encode_config->autofilter == 1){
    config->autofilter = encode_config->autofilter;
  }
  if (encode_config->alpha_compression == 0 || encode_config->alpha_compression == 1){
    config->alpha_compression = encode_config->alpha_compression;
  }
  if (encode_config->alpha_filtering >= 0 && encode_config->alpha_filtering <= 2){
    config->alpha_filtering = encode_config->alpha_filtering;
  }
  if (encode_config->alpha_quality >= 0 && encode_config->alpha_quality <= 100){
    config->alpha_quality = encode_config->alpha_quality;
  }
  if (encode_config->pass >= 0 && encode_config->pass <= 10){
    config->pass = encode_config->pass;
  }
  if (encode_config->show_compressed >= 0){
    config->show_compressed = encode_config->show_compressed;
  }
  if (encode_config->preprocessing == 0 || encode_config->preprocessing == 1){
    config->preprocessing = encode_config->preprocessing;
  }
  if (encode_config->partitions >= 0 && encode_config->partitions <= 3){
    config->partitions = encode_config->partitions;
  }
  if (encode_config->partition_limit >= 0 && encode_config->partition_limit <= 100){
    config->partition_limit = encode_config->partition_limit;
  }
  // a size means the input is raw YUV
  if ((encode_config->width | encode_config->height) > 0){
    picture->width = encode_config->width;
    picture->height = encode_config->height;
  }

  if (!WebPValidateConfig(config)) {
    return ENCODE_INVALID_CONFIG;
  }
  return 0;
}

// Crops, resizes and encodes 'picture', writing the WebP data to 'out'.
static int EncodePicture(const FfiWebpEncodeConfig* const encode_config,
                         const WebPConfig* const config,
                         WebPPicture* const picture, FILE* const out) {
  picture->writer = EncodeWriter;
  picture->custom_ptr = (void*)out;

  if ((encode_config->crop_w | encode_config->crop_h) > 0){
    if (!WebPPictureView(picture, encode_config->crop_x, encode_config->crop_y,
                         encode_config->crop_w, encode_config->crop_h, picture)) {
      return ENCODE_CANNOT_CROP;
    }
  }

  if ((encode_config->resize_w | encode_config->resize_h) > 0) {
    if (!WebPPictureRescale(picture, encode_config->resize_w, encode_config->resize_h)) {
      return ENCODE_CANNOT_RESIZE;
    }
  }

  if (!WebPEncode(config, picture)) {
    return ENCODE_FAILED;
  }
  return 0;
}

int webp_encode(const char *in_file, const char *out_file, const FfiWebpEncodeConfig *encode_config) {
  WebPPicture picture;
  WebPConfig config;
  FILE* out = NULL;
  int result = SetupEncoder(encode_config, &config, &picture);

  // after a version mismatch 'picture' is not initialized, so it can't be freed
  if (result == ENCODE_VERSION_MISMATCH) return result;
  if (result != 0) goto End;

  if (!UtilReadPicture(in_file, &picture, 1)) {
    result = ENCODE_CANNOT_READ_INPUT;
    goto End;
  }

  out = fopen(out_file, "wb");
  if (out == NULL) {
    result = ENCODE_CANNOT_OPEN_OUTPUT;
    goto End;
  }
  result = EncodePicture(encode_config, &config, &picture, out);

 End:
  WebPPictureFree(&picture);
  // fclose flushes buffered data, so it can also fail (for example, disk full)
  if (out != NULL && fclose(out) != 0 && result == 0) {
    result = ENCODE_FAILED;
  }
  return result;
}

int webp_encode_data(const uint8_t* data, size_t data_size,
                     uint8_t** output, size_t* output_size,
                     const FfiWebpEncodeConfig* encode_config) {
  WebPPicture picture;
  WebPConfig config;
  FILE* in = NULL;
  FILE* out = NULL;
  char* buffer = NULL;
  size_t buffer_size = 0;
  int result;

  *output = NULL;
  *output_size = 0;
  result = SetupEncoder(encode_config, &config, &picture);
  if (result == ENCODE_VERSION_MISMATCH) return result;
  if (result != 0) goto End;

  if (data == NULL || data_size == 0) {
    result = ENCODE_CANNOT_READ_INPUT;
    goto End;
  }
  // the readers take a FILE*, so wrap the data in one; "rb" never writes to it
  in = fmemopen((void*)data, data_size, "rb");
  if (in == NULL) {
    result = ENCODE_OUT_OF_MEMORY;
    goto End;
  }
  if (!UtilReadPictureFile(in, &picture, 1)) {
    result = ENCODE_CANNOT_READ_INPUT;
    goto End;
  }

  // a FILE* that collects the WebP data in a growing malloc'ed buffer
  out = open_memstream(&buffer, &buffer_size);
  if (out == NULL) {
    result = ENCODE_OUT_OF_MEMORY;
    goto End;
  }
  result = EncodePicture(encode_config, &config, &picture, out);

 End:
  WebPPictureFree(&picture);
  if (in != NULL) fclose(in);
  // 'buffer' and 'buffer_size' are final only after fclose
  if (out != NULL && fclose(out) != 0 && result == 0) {
    result = ENCODE_OUT_OF_MEMORY;
  }
  if (result == 0) {
    *output = (uint8_t*)buffer;
    *output_size = buffer_size;
  } else {
    free(buffer);
  }
  return result;
}

// Initializes 'config' and applies the options from Ruby.
static int SetupDecoder(const FfiWebpDecodeConfig* const decode_config,
                        WebPDecoderConfig* const config,
                        OutputFileFormat* const format) {
  if (!WebPInitDecoderConfig(config)) {
    return DECODE_VERSION_MISMATCH;
  }

  *format = decode_config->output_format;
  if (decode_config->no_fancy_upsampling > 0){
    config->options.no_fancy_upsampling = 1;
  }
  if (decode_config->bypass_filtering > 0){
    config->options.bypass_filtering = 1;
  }
  if (decode_config->use_threads > 0){
    config->options.use_threads = 1;
  }
  if ((decode_config->crop_w | decode_config->crop_h) > 0){
    config->options.use_cropping = 1;
    config->options.crop_left   = decode_config->crop_x;
    config->options.crop_top    = decode_config->crop_y;
    config->options.crop_width  = decode_config->crop_w;
    config->options.crop_height = decode_config->crop_h;
  }
  if ((decode_config->resize_w | decode_config->resize_h) > 0){
    config->options.use_scaling = 1;
    config->options.scaled_width  = decode_config->resize_w;
    config->options.scaled_height = decode_config->resize_h;
  }
  return 0;
}

// Decodes 'data' into config->output, in the colorspace that 'format' needs.
static int DecodeData(const uint8_t* const data, size_t data_size,
                      OutputFileFormat format, WebPDecoderConfig* const config) {
  WebPDecBuffer* const output_buffer = &config->output;
  WebPBitstreamFeatures* const bitstream = &config->input;

  if (data == NULL || data_size == 0 ||
      WebPGetFeatures(data, data_size, bitstream) != VP8_STATUS_OK) {
    return DECODE_INVALID_WEBP;
  }

  switch (format) {
    case oPNG:
      output_buffer->colorspace = bitstream->has_alpha ? MODE_RGBA : MODE_RGB;
      break;
    case oPAM:
      output_buffer->colorspace = MODE_RGBA;
      break;
    case oPPM:
      output_buffer->colorspace = MODE_RGB;  // drops alpha for PPM
      break;
    case oBMP:
      output_buffer->colorspace = bitstream->has_alpha ? MODE_BGRA : MODE_BGR;
      break;
    case oTIFF_:    // note: force pre-multiplied alpha
      output_buffer->colorspace =
          bitstream->has_alpha ? MODE_rgbA : MODE_RGB;
      break;
    case oPGM:
    case oYUV:
      output_buffer->colorspace = bitstream->has_alpha ? MODE_YUVA : MODE_YUV;
      break;
    case ALPHA_PLANE_ONLY:
      output_buffer->colorspace = MODE_YUVA;
      break;
    default:
      return DECODE_INVALID_FORMAT;
  }

  if (WebPDecode(data, data_size, config) != VP8_STATUS_OK) {
    return DECODE_FAILED;
  }
  return 0;
}

int webp_decode(const char *in_file, const char *out_file, const FfiWebpDecodeConfig *decode_config) {
  WebPDecoderConfig config;
  OutputFileFormat format;
  const uint8_t* data = NULL;
  size_t data_size = 0;
  int result = SetupDecoder(decode_config, &config, &format);

  if (result != 0) return result;
  if (!UtilReadFile(in_file, &data, &data_size)) {
    return DECODE_CANNOT_READ_INPUT;
  }

  result = DecodeData(data, data_size, format, &config);
  if (result == 0 && !UtilSaveOutput(&config.output, format, out_file)) {
    result = DECODE_CANNOT_WRITE_OUTPUT;
  }

  free((void*)data);
  WebPFreeDecBuffer(&config.output);
  return result;
}

int webp_decode_data(const uint8_t* data, size_t data_size,
                     uint8_t** output, size_t* output_size,
                     const FfiWebpDecodeConfig* decode_config) {
  WebPDecoderConfig config;
  OutputFileFormat format;
  FILE* out = NULL;
  char* buffer = NULL;
  size_t buffer_size = 0;
  int result;

  *output = NULL;
  *output_size = 0;
  result = SetupDecoder(decode_config, &config, &format);
  if (result != 0) return result;

  result = DecodeData(data, data_size, format, &config);
  if (result == 0) {
    // a FILE* that collects the output in a growing malloc'ed buffer
    out = open_memstream(&buffer, &buffer_size);
    if (out == NULL) {
      result = DECODE_OUT_OF_MEMORY;
    } else {
      const int written = UtilWriteOutput(&config.output, format, out);
      // writing to memory only fails when memory runs out
      if (fclose(out) != 0 || !written) {
        result = DECODE_OUT_OF_MEMORY;
      }
    }
  }
  WebPFreeDecBuffer(&config.output);

  if (result == 0) {
    *output = (uint8_t*)buffer;
    *output_size = buffer_size;
  } else {
    free(buffer);
  }
  return result;
}

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

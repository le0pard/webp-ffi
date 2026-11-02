#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// utils
#include "./metadata.h"
#include "./util.h"
#include "./webp_ffi.h"

// libwebp 1.0.0 is the oldest supported version (encoder ABI 0x020e)
#if WEBP_ENCODER_ABI_VERSION < 0x020e || WEBP_DECODER_ABI_VERSION < 0x0208
#error "webp-ffi needs libwebp 1.0.0 or newer"
#endif

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

int webp_get_features(const uint8_t* data, size_t data_size,
                      FfiWebpFeatures* features) {
  WebPBitstreamFeatures bitstream;
  const VP8StatusCode status = WebPGetFeatures(data, data_size, &bitstream);
  if (status != VP8_STATUS_OK) return (int)status;
  features->width = bitstream.width;
  features->height = bitstream.height;
  features->has_alpha = bitstream.has_alpha;
  features->has_animation = bitstream.has_animation;
  features->format = bitstream.format;
  return 0;
}

// Frees the output of the in-memory functions.
void webp_free_data(uint8_t* data) {
  free(data);
}

// Initializes 'config' and 'picture' and applies the options from Ruby.
static int SetupEncoder(const FfiWebpEncodeConfig* const encode_config,
                        WebPConfig* const config, WebPPicture* const picture) {
  if (!WebPPictureInit(picture) || !WebPConfigInit(config)) {
    return ENCODE_VERSION_MISMATCH;
  }

  // presets first, so the options below can still change their settings
  if (encode_config->preset >= WEBP_PRESET_DEFAULT && encode_config->preset <= WEBP_PRESET_TEXT) {
    const float quality = (encode_config->quality >= 0 && encode_config->quality <= 100)
                              ? encode_config->quality : 75.f;
    if (!WebPConfigPreset(config, (WebPPreset)encode_config->preset, quality)) {
      return ENCODE_VERSION_MISMATCH;
    }
  }
  if (encode_config->lossless_level >= 0 && encode_config->lossless_level <= 9) {
    if (!WebPConfigLosslessPreset(config, encode_config->lossless_level)) {
      return ENCODE_INVALID_CONFIG;
    }
  }

  if (encode_config->lossless == 0 || encode_config->lossless == 1){
    config->lossless = encode_config->lossless;
  }
  if (encode_config->near_lossless >= 0 && encode_config->near_lossless <= 100){
    config->near_lossless = encode_config->near_lossless;
    config->lossless = 1;  // use near-lossless only with lossless
  }
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
  if (encode_config->image_hint >= WEBP_HINT_DEFAULT && encode_config->image_hint < WEBP_HINT_LAST){
    config->image_hint = (WebPImageHint)encode_config->image_hint;
  }
  if (encode_config->exact == 0 || encode_config->exact == 1){
    config->exact = encode_config->exact;
  }
  if (encode_config->use_sharp_yuv == 0 || encode_config->use_sharp_yuv == 1){
    config->use_sharp_yuv = encode_config->use_sharp_yuv;
  }
  if (encode_config->thread_level == 0 || encode_config->thread_level == 1){
    config->thread_level = encode_config->thread_level;
  }
  if (encode_config->low_memory == 0 || encode_config->low_memory == 1){
    config->low_memory = encode_config->low_memory;
  }
  if (encode_config->emulate_jpeg_size == 0 || encode_config->emulate_jpeg_size == 1){
    config->emulate_jpeg_size = encode_config->emulate_jpeg_size;
  }
  // a size means the input is raw YUV
  if ((encode_config->width | encode_config->height) > 0){
    picture->width = encode_config->width;
    picture->height = encode_config->height;
  }

  // The rules below come from libwebp's examples/cwebp.c.
  // A target size or PSNR is only searched for with more than one pass.
  if ((config->target_size > 0 || config->target_PSNR > 0) && config->pass == 1) {
    config->pass = 6;
  }
  // Keep RGB samples when the encoding needs them; otherwise the readers
  // convert to YUV right away, which loses detail before lossless encoding
  // and makes sharp YUV conversion impossible.
  picture->use_argb = config->lossless || config->use_sharp_yuv ||
                      config->preprocessing > 0 ||
                      (encode_config->crop_w | encode_config->crop_h) > 0 ||
                      (encode_config->resize_w | encode_config->resize_h) > 0;

  if (!WebPValidateConfig(config)) {
    return ENCODE_INVALID_CONFIG;
  }
  return 0;
}

// Crops, resizes and encodes 'picture'. On success '*webp' holds the WebP
// file (free it with free()).
static int EncodeToMemory(const FfiWebpEncodeConfig* const encode_config,
                          const WebPConfig* const config, WebPPicture* const picture,
                          char** const webp, size_t* const webp_size, int* const detail) {
  int result = 0;
  FILE* const out = open_memstream(webp, webp_size);
  if (out == NULL) return ENCODE_OUT_OF_MEMORY;

  picture->writer = EncodeWriter;
  picture->custom_ptr = (void*)out;
  if ((encode_config->crop_w | encode_config->crop_h) > 0 &&
      !WebPPictureView(picture, encode_config->crop_x, encode_config->crop_y,
                       encode_config->crop_w, encode_config->crop_h, picture)) {
    result = ENCODE_CANNOT_CROP;
  } else if ((encode_config->resize_w | encode_config->resize_h) > 0 &&
             !WebPPictureRescale(picture, encode_config->resize_w, encode_config->resize_h)) {
    result = ENCODE_CANNOT_RESIZE;
  } else if (!WebPEncode(config, picture)) {
    result = ENCODE_FAILED;
  }
  if (result != 0) *detail = (int)picture->error_code;

  // '*webp' and '*webp_size' are final only after fclose
  if (fclose(out) != 0 && result == 0) result = ENCODE_OUT_OF_MEMORY;
  if (result != 0) {
    free(*webp);
    *webp = NULL;
  }
  return result;
}

// Reads a picture (and its metadata, when asked for) from 'in' and encodes it.
// On success '*webp' holds the encoder output (free it with free()).
static int EncodeInput(FILE* const in, const FfiWebpEncodeConfig* const encode_config,
                       const WebPConfig* const config, WebPPicture* const picture,
                       Metadata* const metadata, char** const webp,
                       size_t* const webp_size, int* const detail) {
  if (!UtilReadPictureFile(in, picture, 1, encode_config->metadata > 0 ? metadata : NULL)) {
    return ENCODE_CANNOT_READ_INPUT;
  }
  return EncodeToMemory(encode_config, config, picture, webp, webp_size, detail);
}

// Writes the WebP file with the requested metadata to 'out_file'. Opening it
// only now means a failed encode leaves no file, and the input file can be the
// output file. When writing fails, a file this call created is removed again.
static int WriteWebPFile(const char* const out_file, const char* const webp,
                         size_t webp_size, const WebPPicture* const picture,
                         const Metadata* const metadata, int keep_metadata,
                         int* const detail) {
  const int existed = UtilFileExists(out_file);
  FILE* const out = fopen(out_file, "wb");
  int ok;

  if (out == NULL) return ENCODE_CANNOT_OPEN_OUTPUT;
  ok = MetadataWriteWebP(out, (const uint8_t*)webp, webp_size, picture->width,
                         picture->height, metadata, keep_metadata);
  // fclose flushes buffered data, so it can also fail (for example, disk full)
  if (fclose(out) != 0) ok = 0;
  if (ok) return 0;
  if (!existed) remove(out_file);
  *detail = VP8_ENC_ERROR_BAD_WRITE;
  return ENCODE_FAILED;
}

int webp_encode(const char *in_file, const char *out_file,
                const FfiWebpEncodeConfig *encode_config, int* detail) {
  WebPPicture picture;
  WebPConfig config;
  Metadata metadata;
  FILE* in;
  char* webp = NULL;
  size_t webp_size = 0;
  int result;

  *detail = 0;
  result = SetupEncoder(encode_config, &config, &picture);
  // after a version mismatch 'picture' is not initialized, so it can't be freed
  if (result == ENCODE_VERSION_MISMATCH) return result;
  MetadataInit(&metadata);
  if (result != 0) goto End;

  in = fopen(in_file, "rb");
  if (in == NULL) {
    result = ENCODE_CANNOT_READ_INPUT;
    goto End;
  }
  // the input is read completely and closed before the output is opened
  result = EncodeInput(in, encode_config, &config, &picture, &metadata,
                       &webp, &webp_size, detail);
  fclose(in);
  if (result == 0) {
    result = WriteWebPFile(out_file, webp, webp_size, &picture, &metadata,
                           encode_config->metadata, detail);
  }

 End:
  free(webp);
  MetadataFree(&metadata);
  WebPPictureFree(&picture);
  return result;
}

int webp_encode_data(const uint8_t* data, size_t data_size,
                     uint8_t** output, size_t* output_size,
                     const FfiWebpEncodeConfig* encode_config, int* detail) {
  WebPPicture picture;
  WebPConfig config;
  Metadata metadata;
  FILE* in;
  FILE* out;
  char* webp = NULL;
  size_t webp_size = 0;
  char* buffer = NULL;
  size_t buffer_size = 0;
  int result;

  *output = NULL;
  *output_size = 0;
  *detail = 0;
  result = SetupEncoder(encode_config, &config, &picture);
  if (result == ENCODE_VERSION_MISMATCH) return result;
  MetadataInit(&metadata);
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
  result = EncodeInput(in, encode_config, &config, &picture, &metadata,
                       &webp, &webp_size, detail);
  fclose(in);
  if (result != 0) goto End;

  // a FILE* that collects the WebP file in a growing malloc'ed buffer
  out = open_memstream(&buffer, &buffer_size);
  if (out == NULL) {
    result = ENCODE_OUT_OF_MEMORY;
    goto End;
  }
  if (!MetadataWriteWebP(out, (const uint8_t*)webp, webp_size, picture.width,
                         picture.height, &metadata, encode_config->metadata)) {
    result = ENCODE_OUT_OF_MEMORY;
  }
  // 'buffer' and 'buffer_size' are final only after fclose
  if (fclose(out) != 0 && result == 0) result = ENCODE_OUT_OF_MEMORY;

 End:
  free(webp);
  MetadataFree(&metadata);
  WebPPictureFree(&picture);
  if (result == 0) {
    *output = (uint8_t*)buffer;
    *output_size = buffer_size;
  } else {
    free(buffer);
  }
  return result;
}

int webp_encode_rgba(const uint8_t* rgba, int width, int height,
                     uint8_t** output, size_t* output_size,
                     const FfiWebpEncodeConfig* encode_config, int* detail) {
  WebPPicture picture;
  WebPConfig config;
  char* webp = NULL;
  size_t webp_size = 0;
  int stride;
  int result;

  *output = NULL;
  *output_size = 0;
  *detail = 0;
  result = SetupEncoder(encode_config, &config, &picture);
  if (result == ENCODE_VERSION_MISMATCH) return result;
  if (result != 0) goto End;

  if (rgba == NULL || width <= 0 || height <= 0 ||
      !UtilCheckImageSize((uint64_t)width, (uint64_t)height, 4, &stride, NULL)) {
    result = ENCODE_CANNOT_READ_INPUT;
    goto End;
  }
  picture.width = width;
  picture.height = height;
  picture.use_argb = 1;
  if (!WebPPictureImportRGBA(&picture, rgba, stride)) {
    result = ENCODE_OUT_OF_MEMORY;
    *detail = (int)picture.error_code;
    goto End;
  }
  // raw pixels have no metadata, so the encoder output is the whole file
  result = EncodeToMemory(encode_config, &config, &picture, &webp, &webp_size, detail);
  if (result == 0) {
    *output = (uint8_t*)webp;
    *output_size = webp_size;
  }

 End:
  WebPPictureFree(&picture);
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

// The colorspace each output format is written from.
static int OutputColorspace(OutputFileFormat format, int has_alpha,
                            WEBP_CSP_MODE* const colorspace) {
  switch (format) {
    case oPNG:
      *colorspace = has_alpha ? MODE_RGBA : MODE_RGB;
      return 1;
    case oPAM:
      *colorspace = MODE_RGBA;
      return 1;
    case oPPM:
      *colorspace = MODE_RGB;  // drops alpha for PPM
      return 1;
    case oBMP:
      *colorspace = has_alpha ? MODE_BGRA : MODE_BGR;
      return 1;
    case oTIFF_:    // note: force pre-multiplied alpha
      *colorspace = has_alpha ? MODE_rgbA : MODE_RGB;
      return 1;
    case oPGM:
    case oYUV:
      *colorspace = has_alpha ? MODE_YUVA : MODE_YUV;
      return 1;
    case ALPHA_PLANE_ONLY:
      *colorspace = MODE_YUVA;
      return 1;
    default:
      return 0;
  }
}

// Decodes 'data' into config->output. 'format' picks the colorspace; a
// negative 'format' decodes to RGBA.
static int DecodeData(const uint8_t* const data, size_t data_size, int format,
                      WebPDecoderConfig* const config, int* const detail) {
  WebPDecBuffer* const output_buffer = &config->output;
  VP8StatusCode status;

  if (data == NULL || data_size == 0) {
    *detail = VP8_STATUS_NOT_ENOUGH_DATA;
    return DECODE_INVALID_WEBP;
  }
  status = WebPGetFeatures(data, data_size, &config->input);
  if (status != VP8_STATUS_OK) {
    *detail = (int)status;
    return DECODE_INVALID_WEBP;
  }
  if (format < 0) {
    output_buffer->colorspace = MODE_RGBA;
  } else if (!OutputColorspace((OutputFileFormat)format, config->input.has_alpha,
                               &output_buffer->colorspace)) {
    return DECODE_INVALID_FORMAT;
  }
  status = WebPDecode(data, data_size, config);
  if (status != VP8_STATUS_OK) {
    *detail = (int)status;
    return DECODE_FAILED;
  }
  return 0;
}

int webp_decode(const char *in_file, const char *out_file,
                const FfiWebpDecodeConfig *decode_config, int* detail) {
  WebPDecoderConfig config;
  OutputFileFormat format;
  const uint8_t* data = NULL;
  size_t data_size = 0;
  int result;

  *detail = 0;
  result = SetupDecoder(decode_config, &config, &format);
  if (result != 0) return result;
  if (!UtilReadFile(in_file, &data, &data_size)) {
    return DECODE_CANNOT_READ_INPUT;
  }

  result = DecodeData(data, data_size, (int)format, &config, detail);
  if (result == 0 && !UtilSaveOutput(&config.output, format, out_file)) {
    result = DECODE_CANNOT_WRITE_OUTPUT;
  }

  free((void*)data);
  WebPFreeDecBuffer(&config.output);
  return result;
}

int webp_decode_data(const uint8_t* data, size_t data_size,
                     uint8_t** output, size_t* output_size,
                     const FfiWebpDecodeConfig* decode_config, int* detail) {
  WebPDecoderConfig config;
  OutputFileFormat format;
  FILE* out = NULL;
  char* buffer = NULL;
  size_t buffer_size = 0;
  int result;

  *output = NULL;
  *output_size = 0;
  *detail = 0;
  result = SetupDecoder(decode_config, &config, &format);
  if (result != 0) return result;

  result = DecodeData(data, data_size, (int)format, &config, detail);
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

int webp_decode_rgba(const uint8_t* data, size_t data_size,
                     uint8_t** output, size_t* output_size, int* width, int* height,
                     const FfiWebpDecodeConfig* decode_config, int* detail) {
  WebPDecoderConfig config;
  OutputFileFormat format;
  int result;

  *output = NULL;
  *output_size = 0;
  *width = 0;
  *height = 0;
  *detail = 0;
  result = SetupDecoder(decode_config, &config, &format);
  if (result != 0) return result;

  result = DecodeData(data, data_size, -1, &config, detail);
  if (result == 0) {
    const WebPDecBuffer* const buffer = &config.output;
    const WebPRGBABuffer* const rgba = &buffer->u.RGBA;
    const size_t row_size = (size_t)buffer->width * 4;
    uint8_t* pixels = (uint8_t*)malloc(row_size * (size_t)buffer->height);
    if (pixels == NULL) {
      result = DECODE_OUT_OF_MEMORY;
    } else {
      int y;
      // copy row by row: libwebp's stride may be wider than the image
      for (y = 0; y < buffer->height; ++y) {
        memcpy(pixels + (size_t)y * row_size, rgba->rgba + (size_t)y * rgba->stride, row_size);
      }
      *output = pixels;
      *output_size = row_size * (size_t)buffer->height;
      *width = buffer->width;
      *height = buffer->height;
    }
  }
  WebPFreeDecBuffer(&config.output);
  return result;
}

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

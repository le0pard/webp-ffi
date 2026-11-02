#ifndef _WEBP_FFI_H_
#define _WEBP_FFI_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

  typedef struct {
    int lossless;           // Lossless encoding (0=lossy(default), 1=lossless).
    int near_lossless;      // use near-lossless image preprocessing (0..100=off)
    float quality;          // between 0 (smallest file) and 100 (biggest)
    int method;             // quality/speed trade-off (0=fast, 6=slower-better)

    // Parameters related to lossy compression only:
    int target_size;        // if non-zero, set the desired target size in bytes.
                            // Takes precedence over the 'compression' parameter.
    float target_PSNR;      // if non-zero, specifies the minimal distortion to
                            // try to achieve. Takes precedence over target_size.
    int segments;           // maximum number of segments to use, in [1..4]
    int sns_strength;       // Spatial Noise Shaping. 0=off, 100=maximum.
    int filter_strength;    // range: [0 = off .. 100 = strongest]
    int filter_sharpness;   // range: [0 = off .. 7 = least sharp]
    int filter_type;        // filtering type: 0 = simple, 1 = strong (only used
                            // if filter_strength > 0 or autofilter > 0)
    int autofilter;         // Auto adjust filter's strength [0 = off, 1 = on]
    int alpha_compression;  // Algorithm for encoding the alpha plane (0 = none,
                            // 1 = compressed with WebP lossless). Default is 1.
    int alpha_filtering;    // Predictive filtering method for alpha plane.
                            //  0: none, 1: fast, 2: best. Default if 1.
    int alpha_quality;      // Between 0 (smallest size) and 100 (lossless).
                            // Default is 100.
    int pass;               // number of entropy-analysis passes (in [1..10]).

    int show_compressed;    // if true, export the compressed picture back.
                            // In-loop filtering is not applied.
    int preprocessing;      // preprocessing filter (0=none, 1=segment-smooth)
    int partitions;         // log2(number of token partitions) in [0..3]
                            // Default is set to 0 for easier progressive decoding.
    int partition_limit;    // quality degradation allowed to fit the 512k limit on
                            // prediction modes coding (0: no degradation,
                            // 100: maximum possible degradation).
    int width, height;
    int crop_x, crop_y, crop_w, crop_h;
    int resize_w, resize_h;
    // -1 keeps the libwebp default for the options below
    int preset;             // WebPPreset, applied before the other options
    int image_hint;         // WebPImageHint
    int exact;              // keep RGB values under transparent areas
    int use_sharp_yuv;      // sharper (and slower) RGB to YUV conversion
    int thread_level;       // multi-threaded encoding
    int low_memory;         // less memory, more CPU
    int emulate_jpeg_size;  // quality comparable to JPEG at the same size
    int lossless_level;     // 0..9, WebPConfigLosslessPreset (turns on lossless)
    int metadata;           // METADATA_* flags: what to copy from the input
  } FfiWebpEncodeConfig;

  typedef struct {
    OutputFileFormat output_format;
    int bypass_filtering;               // if true, skip the in-loop filtering
    int no_fancy_upsampling;            // if true, use faster pointwise upsampler
    int crop_x, crop_y, crop_w, crop_h;
    int resize_w, resize_h;
    int use_threads;                    // if true, use multi-threaded decoding
  } FfiWebpDecodeConfig;


  typedef struct {
    int width, height;
    int has_alpha, has_animation;
    int format;  // 0 = mixed or unknown, 1 = lossy, 2 = lossless
  } FfiWebpFeatures;

  // Versions are packed as 0xMMmmpp (major, minor, patch).
  int decoder_version(void);
  int encoder_version(void);
  int webp_get_info(const uint8_t* data, size_t data_size, int* width, int* height);
  // Returns a VP8StatusCode, 0 when 'features' was filled.
  int webp_get_features(const uint8_t* data, size_t data_size, FfiWebpFeatures* features);

  // All functions below return 0 on success or an error code (see
  // lib/webp/error.rb). On failure '*detail' holds libwebp's own reason when
  // there is one: a WebPEncodingError when encoding, a VP8StatusCode when
  // decoding, otherwise 0.
  int webp_encode(const char *in_file, const char *out_file,
                  const FfiWebpEncodeConfig *encode_config, int* detail);
  int webp_decode(const char *in_file, const char *out_file,
                  const FfiWebpDecodeConfig *decode_config, int* detail);

  // In-memory versions: on success '*output' holds 'output_size' bytes that the
  // caller frees with webp_free_data. On failure '*output' is NULL.
  int webp_encode_data(const uint8_t* data, size_t data_size,
                       uint8_t** output, size_t* output_size,
                       const FfiWebpEncodeConfig* encode_config, int* detail);
  int webp_decode_data(const uint8_t* data, size_t data_size,
                       uint8_t** output, size_t* output_size,
                       const FfiWebpDecodeConfig* decode_config, int* detail);
  // Raw pixels: 'rgba' holds width * height * 4 bytes, row after row.
  int webp_encode_rgba(const uint8_t* rgba, int width, int height,
                       uint8_t** output, size_t* output_size,
                       const FfiWebpEncodeConfig* encode_config, int* detail);
  int webp_decode_rgba(const uint8_t* data, size_t data_size,
                       uint8_t** output, size_t* output_size, int* width, int* height,
                       const FfiWebpDecodeConfig* decode_config, int* detail);
  void webp_free_data(uint8_t* data);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif	/* _WEBP_FFI_H_ */

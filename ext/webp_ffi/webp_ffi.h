#ifndef _WEBP_FFI_H_
#define _WEBP_FFI_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

  typedef struct {
    int lossless;           // Lossless encoding (0=lossy(default), 1=lossless).
    float quality;          // between 0 (smallest file) and 100 (biggest)
    int method;             // quality/speed trade-off (0=fast, 6=slower-better)
    int segments;           // maximum number of segments to use, in [1..4]
    int sns_strength;       // Spatial Noise Shaping. 0=off, 100=maximum.
    int alpha_quality;      // Between 0 (smallest size) and 100 (lossless). Default is 100.
    int alpha_compression;  // Algorithm for encoding the alpha plane
    int alpha_filtering;    // Predictive filtering method for alpha plane.
    int width;
    int height;
    int crop_x;
    int crop_y;
    int crop_w;
    int crop_h;
    int resize_w;
    int resize_h;
  } FfiWebpEncodeConfig;

  void decoder_version(char *version);
  void encoder_version(char *version);
  int webp_get_info(const uint8_t* data, size_t data_size, int* width, int* height);
  int webp_decode(const char *in_file, const char *out_file);
  int webp_encode(const char *in_file, const char *out_file, const FfiWebpEncodeConfig *encode_config);
  int test_c(int n);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif	/* _WEBP_FFI_H_ */
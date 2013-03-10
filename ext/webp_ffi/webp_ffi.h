#ifndef _WEBP_FFI_H_
#define _WEBP_FFI_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

  typedef struct {
    int lossless;           // Lossless encoding (0=lossy(default), 1=lossless).
    float quality;          // between 0 (smallest file) and 100 (biggest)
    int method;             // quality/speed trade-off (0=fast, 6=slower-better)
  } FfiWebpConfig;

  void decoder_version(char *version);
  void encoder_version(char *version);
  int webp_get_info(const uint8_t* data, size_t data_size, int* width, int* height);
  int webp_decode(const uint8_t* data, size_t data_size, uint8_t* output);
  int webp_encode(const uint8_t* data, size_t data_size, const FfiWebpConfig* ffi_config, uint8_t* output);
  int test(int n);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif	/* _WEBP_FFI_H_ */
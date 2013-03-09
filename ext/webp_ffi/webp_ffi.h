#ifndef _WEBP_FFI_H_
#define _WEBP_FFI_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

  void decoder_version(char *version);
  void encoder_version(char *version);
  int webp_get_info(const uint8_t* data, size_t data_size, int* width, int* height);
  size_t webp_encode_rgb(const uint8_t* rgb, int width, int height, int stride, float quality_factor, uint8_t** output);
  int webp_encode(const uint8_t* data, size_t data_size, const WebPConfig* config, uint8_t** output);
  int test(int n);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif	/* _WEBP_FFI_H_ */
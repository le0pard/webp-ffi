#ifndef _WEBP_FFI_H_
#define _WEBP_FFI_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

  void decoder_version(char *version);
  void encoder_version(char *version);
  int webp_get_info(const uint8_t* data, size_t data_size, int* width, int* height);
  uint8_t* webp_decode_rgba(const uint8_t* data, size_t data_size, int* width, int* height);
  int test(int n);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif	/* _WEBP_FFI_H_ */
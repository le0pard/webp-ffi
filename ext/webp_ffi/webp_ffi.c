#include "webp_ffi.h"


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

int webp_get_info(const uint8_t* data, size_t data_size, int* width, int* height) {
  return WebPGetInfo(data, data_size, width, height);
}

// test
int test(int n)
{
  return n + 100;
}


#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
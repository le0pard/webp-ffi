#include "webp/decode.h"
#include "webp/encode.h"

#ifndef _WEBP_FFI_H_
#define _WEBP_FFI_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
//webp
int WebPGetInfo(const uint8_t* data, size_t data_size, int* width, int* height);
uint8_t* WebPDecodeRGBA(const uint8_t* data, size_t data_size, int* width, int* height);

//own
int test(int n);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif	/* _WEBP_FFI_H_ */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// libwebp
#include "webp/decode.h"
#include "webp/encode.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WEBP_HAVE_PNG
#include <png.h>
#endif

#ifdef WEBP_HAVE_JPEG
#include <setjmp.h>   // note: this must be included *after* png.h
#include <jpeglib.h>
#endif

#ifdef WEBP_HAVE_TIFF
#include <tiffio.h>
#endif

// utils
#include "./util.h"
#include "./webp_ffi.h"


//#if defined(__cplusplus) || defined(c_plusplus)
//extern "C" {
//#endif

int webp_get_info(const uint8_t* data, size_t data_size, int* width, int* height) {
  return WebPGetInfo(data, data_size, width, height);
}

// test
int test(int n)
{
  return n + 100;
}


//#if defined(__cplusplus) || defined(c_plusplus)
//}    // extern "C"
//#endif
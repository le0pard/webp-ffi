#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// utils
#include "./util.h"
#include "./webp_ffi.h"

#ifdef HAVE_CONFIG_H
#include "webp/config.h"
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


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

//get webp info
int webp_get_features(const uint8_t* data, size_t data_size, WebPBitstreamFeatures* features){
  if (WebPGetFeatures(data, data_size, features) != VP8_STATUS_OK) {
    return 0;
  }
  return 1;
}

// test
int test(int n)
{
  return n + 100;
}


#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
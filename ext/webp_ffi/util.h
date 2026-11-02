#ifndef _WEBP_FFI_UTIL_H_
#define _WEBP_FFI_UTIL_H_

#include <stddef.h>
#include <stdint.h>

#include "webp/decode.h"
#include "webp/encode.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum {
  oPNG = 0,
  oPAM,
  oPPM,
  oPGM,
  oBMP,
  oTIFF_,
  oYUV,
  ALPHA_PLANE_ONLY  // this is for experimenting only
} OutputFileFormat;

typedef enum {
  iPNG_ = 0,
  iJPEG_,
  iTIFF_,  // 'TIFF' clashes with libtiff
  UNSUPPORTED
} InputFileFormat;

int UtilSaveOutput(const WebPDecBuffer* const buffer, OutputFileFormat format, const char* const out_file);
// Allocates storage for entire file 'file_name' and returns contents and size
// in 'data' and 'data_size'. Returns 1 on success, 0 otherwise. '*data' should
// be deleted using free().
int UtilReadFile(const char* const file_name,
                   const uint8_t** data, size_t* data_size);

int UtilReadPicture(const char* const filename, WebPPicture* const pic,
                      int keep_alpha);

// Checks that a 'width' x 'height' image with 'bytes_per_px' bytes per pixel
// fits in memory, using 64-bit math so the size can't wrap around.
// Returns 1 on success and stores the row size in 'stride' and the buffer size
// in 'size' (either pointer may be NULL). Returns 0 for empty or oversized images.
int UtilCheckImageSize(uint64_t width, uint64_t height, uint64_t bytes_per_px,
                       int* const stride, size_t* const size);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif  /* _WEBP_FFI_UTIL_H_ */

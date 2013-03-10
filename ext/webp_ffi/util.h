#include "webp/decode.h"
#include "webp/encode.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum {
  PNG = 0,
  PAM,
  PPM,
  PGM,
  ALPHA_PLANE_ONLY  // this is for experimenting only
} OutputFileFormat;

typedef enum {
  PNG_ = 0,
  JPEG_,
  TIFF_,  // 'TIFF' clashes with libtiff
  UNSUPPORTED
} InputFileFormat;

int UtilSaveOutput(const WebPDecBuffer* const buffer, OutputFileFormat format, const char* const out_file);
// Allocates storage for entire file 'file_name' and returns contents and size
// in 'data' and 'data_size'. Returns 1 on success, 0 otherwise. '*data' should
// be deleted using free().
int UtilReadFile(const char* const file_name,
                   const uint8_t** data, size_t* data_size);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
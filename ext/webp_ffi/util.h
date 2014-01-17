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

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
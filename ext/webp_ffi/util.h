#include "webp/types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// Allocates storage for entire file 'file_name' and returns contents and size
// in 'data' and 'data_size'. Returns 1 on success, 0 otherwise. '*data' should
// be deleted using free().
int WebpFfiReadFile(const char* const file_name,
                   const uint8_t** data, size_t* data_size);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
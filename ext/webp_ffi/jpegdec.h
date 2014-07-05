#include <stdio.h>

#include "webp/encode.h"
#include "webp/decode.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reads a JPEG from 'in_file', returning the decoded output in 'pic'.
// The output is RGB.
// Returns true on success.
int UtilReadJPEG(FILE* in_file, struct WebPPicture* const pic);

#ifdef __cplusplus
}    // extern "C"
#endif

#include <stdio.h>

#include "webp/encode.h"
#include "webp/decode.h"
#include "./metadata.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reads a JPEG from 'in_file', returning the decoded output in 'pic'.
// The output is RGB. When 'metadata' is not NULL, the ICC profile, EXIF and
// XMP data are copied into it.
// Returns true on success.
int UtilReadJPEG(FILE* in_file, struct WebPPicture* const pic,
                 Metadata* const metadata);

#ifdef __cplusplus
}    // extern "C"
#endif

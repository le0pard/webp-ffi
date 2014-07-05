#include <stdio.h>
#include <stdlib.h>

#include "webp/encode.h"
#include "webp/decode.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reads a TIFF from 'filename', returning the decoded output in 'pic'.
// If 'keep_alpha' is true and the TIFF has an alpha channel, the output is RGBA
// otherwise it will be RGB.
// Returns true on success.
int UtilReadTIFF(const char* const filename,
             struct WebPPicture* const pic, int keep_alpha);

int UtilWriteTIFF(FILE* fout, const WebPDecBuffer* const buffer);
int UtilWriteBMP(FILE* fout, const WebPDecBuffer* const buffer);

#ifdef __cplusplus
}    // extern "C"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "webp/encode.h"
#include "webp/decode.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reads a PNG from 'in_file', returning the decoded output in 'pic'.
// If 'keep_alpha' is true and the PNG has an alpha channel, the output is RGBA
// otherwise it will be RGB.
// Returns true on success.
int UtilReadPNG(FILE* in_file, struct WebPPicture* const pic, int keep_alpha);

int UtilWritePNG(FILE* out_file, const WebPDecBuffer* const buffer);
int UtilWritePPM(FILE* fout, const WebPDecBuffer* const buffer, int alpha);
int UtilWriteAlphaPlane(FILE* fout, const WebPDecBuffer* const buffer);
int UtilWritePGM(FILE* fout, const WebPDecBuffer* const buffer);

#ifdef __cplusplus
}    // extern "C"
#endif

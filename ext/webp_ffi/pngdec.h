#include <stdio.h>
#include <stdlib.h>

#include "webp/encode.h"
#include "webp/decode.h"
#include "./metadata.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reads a PNG from 'in_file', returning the decoded output in 'pic'.
// If 'keep_alpha' is true and the PNG has an alpha channel, the output is RGBA
// otherwise it will be RGB. When 'metadata' is not NULL, the ICC profile, EXIF
// and XMP data are copied into it.
// Returns true on success.
int UtilReadPNG(FILE* in_file, struct WebPPicture* const pic, int keep_alpha,
                Metadata* const metadata);

int UtilWritePNG(FILE* out_file, const WebPDecBuffer* const buffer);
int UtilWritePPM(FILE* fout, const WebPDecBuffer* const buffer, int alpha);
int UtilWriteAlphaPlane(FILE* fout, const WebPDecBuffer* const buffer);
int UtilWritePGM(FILE* fout, const WebPDecBuffer* const buffer);
// Raw planes: Y, then U and V (half size, rounded up), then alpha if any.
int UtilWriteYUV(FILE* fout, const WebPDecBuffer* const buffer);

#ifdef __cplusplus
}    // extern "C"
#endif

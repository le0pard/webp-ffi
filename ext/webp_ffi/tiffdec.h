#include <stdio.h>
#include <stdlib.h>

#include "webp/encode.h"
#include "webp/decode.h"
#include "./metadata.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reads a TIFF from 'in_file', returning the decoded output in 'pic'.
// If 'keep_alpha' is true and the TIFF has an alpha channel, the output is RGBA
// otherwise it will be RGB. 'in_file' must be seekable and stays open.
// When 'metadata' is not NULL, the ICC profile and XMP data are copied into it.
// Like libwebp's own reader, EXIF is not read from TIFF files.
// Returns true on success.
int UtilReadTIFF(FILE* const in_file,
             struct WebPPicture* const pic, int keep_alpha,
             Metadata* const metadata);

int UtilWriteTIFF(FILE* fout, const WebPDecBuffer* const buffer);
int UtilWriteBMP(FILE* fout, const WebPDecBuffer* const buffer);

// Version string of the libtiff in use. Prebuilt gems keep the bundled
// libtiff symbols private, so the specs ask through this function.
const char* UtilTIFFVersion(void);

#ifdef __cplusplus
}    // extern "C"
#endif

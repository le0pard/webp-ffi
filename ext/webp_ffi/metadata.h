#ifndef _WEBP_FFI_METADATA_H_
#define _WEBP_FFI_METADATA_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// Which metadata to copy from the input picture into the WebP file.
#define METADATA_EXIF 1
#define METADATA_ICC  2
#define METADATA_XMP  4
#define METADATA_ALL  (METADATA_EXIF | METADATA_ICC | METADATA_XMP)

typedef struct {
  uint8_t* bytes;
  size_t size;
} MetadataPayload;

typedef struct {
  MetadataPayload exif;
  MetadataPayload iccp;
  MetadataPayload xmp;
} Metadata;

void MetadataInit(Metadata* const metadata);
void MetadataFree(Metadata* const metadata);

// Copies 'size' bytes into an empty 'payload'. Returns 1 on success, or when
// the payload already holds data (the first copy wins). Returns 0 when out of
// memory.
int MetadataCopy(const void* const data, size_t size, MetadataPayload* const payload);

// Writes a complete WebP file to 'out': the encoder output 'webp' plus the
// metadata selected by 'keep' (METADATA_* flags), using the extended format
// (VP8X) when any metadata is written. 'width' and 'height' are the size of the
// encoded picture. Returns 1 on success.
int MetadataWriteWebP(FILE* const out, const uint8_t* webp, size_t webp_size,
                      int width, int height, const Metadata* const metadata,
                      int keep);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif  /* _WEBP_FFI_METADATA_H_ */

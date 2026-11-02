#include <stdlib.h>
#include <string.h>

#include "./metadata.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// Based on WriteWebPWithMetadata in libwebp's examples/cwebp.c.

#define TAG_SIZE 4
#define CHUNK_HEADER_SIZE 8
#define RIFF_HEADER_SIZE 12
#define VP8X_CHUNK_SIZE 18  // chunk header and 10 bytes of payload
#define MAX_RIFF_SIZE 0xfffffff6ULL  // the RIFF size is a 32-bit even number

// VP8X flags
#define FLAG_ICCP  0x20
#define FLAG_ALPHA 0x10
#define FLAG_EXIF  0x08
#define FLAG_XMP   0x04

void MetadataInit(Metadata* const metadata) {
  memset(metadata, 0, sizeof(*metadata));
}

static void PayloadFree(MetadataPayload* const payload) {
  free(payload->bytes);
  payload->bytes = NULL;
  payload->size = 0;
}

void MetadataFree(Metadata* const metadata) {
  PayloadFree(&metadata->exif);
  PayloadFree(&metadata->iccp);
  PayloadFree(&metadata->xmp);
}

int MetadataCopy(const void* const data, size_t size,
                 MetadataPayload* const payload) {
  if (payload->bytes != NULL || data == NULL || size == 0) return 1;
  payload->bytes = (uint8_t*)malloc(size);
  if (payload->bytes == NULL) return 0;
  memcpy(payload->bytes, data, size);
  payload->size = size;
  return 1;
}

static int WriteLE(FILE* const out, uint32_t value, int bytes) {
  uint8_t buffer[4];
  int i;
  for (i = 0; i < bytes; ++i) {
    buffer[i] = (uint8_t)((value >> (8 * i)) & 0xff);
  }
  return fwrite(buffer, (size_t)bytes, 1, out) == 1;
}

// Chunk payloads have an even size; odd ones get one zero byte of padding.
static uint64_t ChunkSize(const MetadataPayload* const payload) {
  return CHUNK_HEADER_SIZE + (uint64_t)payload->size + (payload->size & 1);
}

static int WriteChunk(FILE* const out, const char* const tag,
                      const MetadataPayload* const payload) {
  const uint8_t zero = 0;
  int ok = (fwrite(tag, TAG_SIZE, 1, out) == 1);
  ok = ok && WriteLE(out, (uint32_t)payload->size, 4);
  ok = ok && (fwrite(payload->bytes, payload->size, 1, out) == 1);
  if (payload->size & 1) ok = ok && (fwrite(&zero, 1, 1, out) == 1);
  return ok;
}

int MetadataWriteWebP(FILE* const out, const uint8_t* webp, size_t webp_size,
                      int width, int height, const Metadata* const metadata,
                      int keep) {
  const int write_exif = (keep & METADATA_EXIF) && metadata->exif.size > 0;
  const int write_iccp = (keep & METADATA_ICC) && metadata->iccp.size > 0;
  const int write_xmp = (keep & METADATA_XMP) && metadata->xmp.size > 0;
  uint32_t flags = 0;
  uint64_t metadata_size = 0;
  uint64_t riff_size;
  int has_vp8x;
  int ok;

  if (webp_size < RIFF_HEADER_SIZE + CHUNK_HEADER_SIZE) return 0;
  if (!write_exif && !write_iccp && !write_xmp) {
    return fwrite(webp, webp_size, 1, out) == 1;
  }

  if (write_exif) { flags |= FLAG_EXIF; metadata_size += ChunkSize(&metadata->exif); }
  if (write_iccp) { flags |= FLAG_ICCP; metadata_size += ChunkSize(&metadata->iccp); }
  if (write_xmp)  { flags |= FLAG_XMP;  metadata_size += ChunkSize(&metadata->xmp); }

  has_vp8x = !memcmp(webp + RIFF_HEADER_SIZE, "VP8X", TAG_SIZE);
  riff_size = (uint64_t)webp_size - CHUNK_HEADER_SIZE +
              (has_vp8x ? 0 : VP8X_CHUNK_SIZE) + metadata_size;
  if (riff_size > MAX_RIFF_SIZE) return 0;

  // "RIFF", the new size, then "WEBP"
  ok = (fwrite(webp, TAG_SIZE, 1, out) == 1);
  ok = ok && WriteLE(out, (uint32_t)riff_size, 4);
  ok = ok && (fwrite(webp + CHUNK_HEADER_SIZE, TAG_SIZE, 1, out) == 1);
  webp += RIFF_HEADER_SIZE;
  webp_size -= RIFF_HEADER_SIZE;

  if (has_vp8x) {
    // lossy with alpha: the encoder already wrote a VP8X chunk, add our flags
    uint8_t vp8x[VP8X_CHUNK_SIZE];
    memcpy(vp8x, webp, VP8X_CHUNK_SIZE);
    vp8x[CHUNK_HEADER_SIZE] |= (uint8_t)(flags & 0xff);
    ok = ok && (fwrite(vp8x, VP8X_CHUNK_SIZE, 1, out) == 1);
    webp += VP8X_CHUNK_SIZE;
    webp_size -= VP8X_CHUNK_SIZE;
  } else {
    // lossless images keep their alpha bit in the VP8L header
    if (!memcmp(webp, "VP8L", TAG_SIZE) && webp_size > CHUNK_HEADER_SIZE + 4 &&
        (webp[CHUNK_HEADER_SIZE + 4] & (1 << 4))) {
      flags |= FLAG_ALPHA;
    }
    ok = ok && (fwrite("VP8X", TAG_SIZE, 1, out) == 1);
    ok = ok && WriteLE(out, VP8X_CHUNK_SIZE - CHUNK_HEADER_SIZE, 4);
    ok = ok && WriteLE(out, flags, 4);
    ok = ok && WriteLE(out, (uint32_t)(width - 1), 3);
    ok = ok && WriteLE(out, (uint32_t)(height - 1), 3);
  }

  // the chunk order is fixed: ICCP, image data, EXIF, XMP
  if (write_iccp) ok = ok && WriteChunk(out, "ICCP", &metadata->iccp);
  ok = ok && (fwrite(webp, webp_size, 1, out) == 1);
  if (write_exif) ok = ok && WriteChunk(out, "EXIF", &metadata->exif);
  if (write_xmp) ok = ok && WriteChunk(out, "XMP ", &metadata->xmp);
  return ok;
}

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

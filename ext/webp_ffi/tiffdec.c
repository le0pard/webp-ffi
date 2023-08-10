#include "./tiffdec.h"

#include <tiffio.h>

int UtilReadTIFF(const char* const filename,
                    WebPPicture* const pic, int keep_alpha) {
  TIFF* const tif = TIFFOpen(filename, "r");
  uint32_t width, height;
  uint32_t* raster;
  int ok = 0;
  int dircount = 1;

  if (tif == NULL) {
    //fprintf(stderr, "Error! Cannot open TIFF file '%s'\n", filename);
    return 0;
  }

  while (TIFFReadDirectory(tif)) ++dircount;

  if (dircount > 1) {
    fprintf(stderr, "Warning: multi-directory TIFF files are not supported.\n"
                    "Only the first will be used, %d will be ignored.\n",
                    dircount - 1);
  }

  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  raster = (uint32_t*)_TIFFmalloc(width * height * sizeof(*raster));
  if (raster != NULL) {
    if (TIFFReadRGBAImageOriented(tif, width, height, raster,
                                  ORIENTATION_TOPLEFT, 1)) {
      const int stride = width * sizeof(*raster);
      pic->width = width;
      pic->height = height;
      // TIFF data is ABGR
#ifdef __BIG_ENDIAN__
      TIFFSwabArrayOfLong(raster, width * height);
#endif
      ok = keep_alpha
         ? WebPPictureImportRGBA(pic, (const uint8_t*)raster, stride)
         : WebPPictureImportRGBX(pic, (const uint8_t*)raster, stride);
    }
    _TIFFfree(raster);
  } else {
    //fprintf(stderr, "Error allocating TIFF RGBA memory!\n");
  }

  if (ok && keep_alpha == 2) {
    WebPCleanupTransparentArea(pic);
  }

  TIFFClose(tif);
  return ok;
}



static void PutLE16(uint8_t* const dst, uint32_t value) {
  dst[0] = (value >> 0) & 0xff;
  dst[1] = (value >> 8) & 0xff;
}

static void PutLE32(uint8_t* const dst, uint32_t value) {
  PutLE16(dst + 0, (value >>  0) & 0xffff);
  PutLE16(dst + 2, (value >> 16) & 0xffff);
}


#define NUM_IFD_ENTRIES 15
#define EXTRA_DATA_SIZE 16
// 10b for signature/header + n * 12b entries + 4b for IFD terminator:
#define EXTRA_DATA_OFFSET (10 + 12 * NUM_IFD_ENTRIES + 4)
#define TIFF_HEADER_SIZE (EXTRA_DATA_OFFSET + EXTRA_DATA_SIZE)

int UtilWriteTIFF(FILE* fout, const WebPDecBuffer* const buffer) {
  const int has_alpha = (buffer->colorspace != MODE_RGB);
  const uint32_t width = buffer->width;
  const uint32_t height = buffer->height;
  const uint8_t* const rgba = buffer->u.RGBA.rgba;
  const int stride = buffer->u.RGBA.stride;
  const uint8_t bytes_per_px = has_alpha ? 4 : 3;
  // For non-alpha case, we omit tag 0x152 (ExtraSamples).
  const uint8_t num_ifd_entries = has_alpha ? NUM_IFD_ENTRIES
                                            : NUM_IFD_ENTRIES - 1;
  uint8_t tiff_header[TIFF_HEADER_SIZE] = {
    0x49, 0x49, 0x2a, 0x00,   // little endian signature
    8, 0, 0, 0,               // offset to the unique IFD that follows
    // IFD (offset = 8). Entries must be written in increasing tag order.
    num_ifd_entries, 0,       // Number of entries in the IFD (12 bytes each).
    0x00, 0x01, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0,    //  10: Width  (TBD)
    0x01, 0x01, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0,    //  22: Height (TBD)
    0x02, 0x01, 3, 0, bytes_per_px, 0, 0, 0,     //  34: BitsPerSample: 8888
        EXTRA_DATA_OFFSET + 0, 0, 0, 0,
    0x03, 0x01, 3, 0, 1, 0, 0, 0, 1, 0, 0, 0,    //  46: Compression: none
    0x06, 0x01, 3, 0, 1, 0, 0, 0, 2, 0, 0, 0,    //  58: Photometric: RGB
    0x11, 0x01, 4, 0, 1, 0, 0, 0,                //  70: Strips offset:
        TIFF_HEADER_SIZE, 0, 0, 0,               //      data follows header
    0x12, 0x01, 3, 0, 1, 0, 0, 0, 1, 0, 0, 0,    //  82: Orientation: topleft
    0x15, 0x01, 3, 0, 1, 0, 0, 0,                //  94: SamplesPerPixels
        bytes_per_px, 0, 0, 0,
    0x16, 0x01, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0,    // 106: Rows per strip (TBD)
    0x17, 0x01, 4, 0, 1, 0, 0, 0, 0, 0, 0, 0,    // 118: StripByteCount (TBD)
    0x1a, 0x01, 5, 0, 1, 0, 0, 0,                // 130: X-resolution
        EXTRA_DATA_OFFSET + 8, 0, 0, 0,
    0x1b, 0x01, 5, 0, 1, 0, 0, 0,                // 142: Y-resolution
        EXTRA_DATA_OFFSET + 8, 0, 0, 0,
    0x1c, 0x01, 3, 0, 1, 0, 0, 0, 1, 0, 0, 0,    // 154: PlanarConfiguration
    0x28, 0x01, 3, 0, 1, 0, 0, 0, 2, 0, 0, 0,    // 166: ResolutionUnit (inch)
    0x52, 0x01, 3, 0, 1, 0, 0, 0, 1, 0, 0, 0,    // 178: ExtraSamples: rgbA
    0, 0, 0, 0,                                  // 190: IFD terminator
    // EXTRA_DATA_OFFSET:
    8, 0, 8, 0, 8, 0, 8, 0,      // BitsPerSample
    72, 0, 0, 0, 1, 0, 0, 0      // 72 pixels/inch, for X/Y-resolution
  };
  uint32_t y;

  // Fill placeholders in IFD:
  PutLE32(tiff_header + 10 + 8, width);
  PutLE32(tiff_header + 22 + 8, height);
  PutLE32(tiff_header + 106 + 8, height);
  PutLE32(tiff_header + 118 + 8, width * bytes_per_px * height);
  if (!has_alpha) PutLE32(tiff_header + 178, 0);  // IFD terminator

  // write header
  if (fwrite(tiff_header, sizeof(tiff_header), 1, fout) != 1) {
    return 0;
  }
  // write pixel values
  for (y = 0; y < height; ++y) {
    if (fwrite(rgba + y * stride, bytes_per_px, width, fout) != width) {
      return 0;
    }
  }

  return 1;
}

#undef TIFF_HEADER_SIZE
#undef EXTRA_DATA_OFFSET
#undef EXTRA_DATA_SIZE
#undef NUM_IFD_ENTRIES

#define BMP_HEADER_SIZE 54
int UtilWriteBMP(FILE* fout, const WebPDecBuffer* const buffer) {
  const int has_alpha = (buffer->colorspace != MODE_BGR);
  const uint32_t width = buffer->width;
  const uint32_t height = buffer->height;
  const uint8_t* const rgba = buffer->u.RGBA.rgba;
  const int stride = buffer->u.RGBA.stride;
  const uint32_t bytes_per_px = has_alpha ? 4 : 3;
  uint32_t y;
  const uint32_t line_size = bytes_per_px * width;
  const uint32_t bmp_stride = (line_size + 3) & ~3;   // pad to 4
  const uint32_t total_size = bmp_stride * height + BMP_HEADER_SIZE;
  uint8_t bmp_header[BMP_HEADER_SIZE] = { 0 };

  // bitmap file header
  PutLE16(bmp_header + 0, 0x4d42);                // signature 'BM'
  PutLE32(bmp_header + 2, total_size);            // size including header
  PutLE32(bmp_header + 6, 0);                     // reserved
  PutLE32(bmp_header + 10, BMP_HEADER_SIZE);      // offset to pixel array
  // bitmap info header
  PutLE32(bmp_header + 14, 40);                   // DIB header size
  PutLE32(bmp_header + 18, width);                // dimensions
  PutLE32(bmp_header + 22, -(int)height);         // vertical flip!
  PutLE16(bmp_header + 26, 1);                    // number of planes
  PutLE16(bmp_header + 28, bytes_per_px * 8);     // bits per pixel
  PutLE32(bmp_header + 30, 0);                    // no compression (BI_RGB)
  PutLE32(bmp_header + 34, 0);                    // image size (dummy)
  PutLE32(bmp_header + 38, 2400);                 // x pixels/meter
  PutLE32(bmp_header + 42, 2400);                 // y pixels/meter
  PutLE32(bmp_header + 46, 0);                    // number of palette colors
  PutLE32(bmp_header + 50, 0);                    // important color count

  // TODO(skal): color profile

  // write header
  if (fwrite(bmp_header, sizeof(bmp_header), 1, fout) != 1) {
    return 0;
  }

  // write pixel array
  for (y = 0; y < height; ++y) {
    if (fwrite(rgba + y * stride, line_size, 1, fout) != 1) {
      return 0;
    }
    // write padding zeroes
    if (bmp_stride != line_size) {
      const uint8_t zeroes[3] = { 0 };
      if (fwrite(zeroes, bmp_stride - line_size, 1, fout) != 1) {
        return 0;
      }
    }
  }
  return 1;
}
#undef BMP_HEADER_SIZE

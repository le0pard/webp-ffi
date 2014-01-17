#include "./util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include <setjmp.h>   // note: this must be included *after* png.h
#include <jpeglib.h>

#include <tiffio.h>

#include "webp/decode.h"
#include "webp/encode.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int UtilReadYUV(FILE* in_file, WebPPicture* const pic) {
  const int use_argb = pic->use_argb;
  const int uv_width = (pic->width + 1) / 2;
  const int uv_height = (pic->height + 1) / 2;
  int y;
  int ok = 0;

  pic->use_argb = 0;
  if (!WebPPictureAlloc(pic)) return ok;

  for (y = 0; y < pic->height; ++y) {
    if (fread(pic->y + y * pic->y_stride, pic->width, 1, in_file) != 1) {
      goto End;
    }
  }
  for (y = 0; y < uv_height; ++y) {
    if (fread(pic->u + y * pic->uv_stride, uv_width, 1, in_file) != 1)
      goto End;
  }
  for (y = 0; y < uv_height; ++y) {
    if (fread(pic->v + y * pic->uv_stride, uv_width, 1, in_file) != 1)
      goto End;
  }
  ok = 1;
  if (use_argb) ok = WebPPictureYUVAToARGB(pic);

 End:
  return ok;
}

struct my_error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

static void my_error_exit(j_common_ptr dinfo) {
  struct my_error_mgr* myerr = (struct my_error_mgr*) dinfo->err;
  (*dinfo->err->output_message) (dinfo);
  longjmp(myerr->setjmp_buffer, 1);
}

static int UtilReadJPEG(FILE* in_file, WebPPicture* const pic) {
  int ok = 0;
  int stride, width, height;
  uint8_t* rgb = NULL;
  uint8_t* row_ptr = NULL;
  struct jpeg_decompress_struct dinfo;
  struct my_error_mgr jerr;
  JSAMPARRAY buffer;

  dinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  if (setjmp(jerr.setjmp_buffer)) {
 Error:
    jpeg_destroy_decompress(&dinfo);
    goto End;
  }

  jpeg_create_decompress(&dinfo);
  jpeg_stdio_src(&dinfo, in_file);
  jpeg_read_header(&dinfo, TRUE);

  dinfo.out_color_space = JCS_RGB;
  dinfo.dct_method = JDCT_IFAST;
  dinfo.do_fancy_upsampling = TRUE;

  jpeg_start_decompress(&dinfo);

  if (dinfo.output_components != 3) {
    goto Error;
  }

  width = dinfo.output_width;
  height = dinfo.output_height;
  stride = dinfo.output_width * dinfo.output_components * sizeof(*rgb);

  rgb = (uint8_t*)malloc(stride * height);
  if (rgb == NULL) {
    goto End;
  }
  row_ptr = rgb;

  buffer = (*dinfo.mem->alloc_sarray) ((j_common_ptr) &dinfo,
                                       JPOOL_IMAGE, stride, 1);
  if (buffer == NULL) {
    goto End;
  }

  while (dinfo.output_scanline < dinfo.output_height) {
    if (jpeg_read_scanlines(&dinfo, buffer, 1) != 1) {
      goto End;
    }
    memcpy(row_ptr, buffer[0], stride);
    row_ptr += stride;
  }

  jpeg_finish_decompress(&dinfo);
  jpeg_destroy_decompress(&dinfo);

  // WebP conversion.
  pic->width = width;
  pic->height = height;
  ok = WebPPictureImportRGB(pic, rgb, stride);

 End:
  if (rgb) {
    free(rgb);
  }
  return ok;
}

static void PNGAPI error_function(png_structp png, png_const_charp dummy) {
  (void)dummy;  // remove variable-unused warning
  longjmp(png_jmpbuf(png), 1);
}

static int UtilReadPNG(FILE* in_file, WebPPicture* const pic, int keep_alpha) {
  png_structp png;
  png_infop info;
  int color_type, bit_depth, interlaced;
  int has_alpha;
  int num_passes;
  int p;
  int ok = 0;
  png_uint_32 width, height, y;
  int stride;
  uint8_t* rgb = NULL;

  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if (png == NULL) {
    goto End;
  }

  png_set_error_fn(png, 0, error_function, NULL);
  if (setjmp(png_jmpbuf(png))) {
 Error:
    png_destroy_read_struct(&png, NULL, NULL);
    free(rgb);
    goto End;
  }

  info = png_create_info_struct(png);
  if (info == NULL) goto Error;

  png_init_io(png, in_file);
  png_read_info(png, info);
  if (!png_get_IHDR(png, info,
                    &width, &height, &bit_depth, &color_type, &interlaced,
                    NULL, NULL)) goto Error;

  png_set_strip_16(png);
  png_set_packing(png);
  if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    if (bit_depth < 8) {
      png_set_expand_gray_1_2_4_to_8(png);
    }
    png_set_gray_to_rgb(png);
  }
  if (png_get_valid(png, info, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png);
    has_alpha = 1;
  } else {
    has_alpha = !!(color_type & PNG_COLOR_MASK_ALPHA);
  }

  if (!keep_alpha) {
    png_set_strip_alpha(png);
    has_alpha = 0;
  }

  num_passes = png_set_interlace_handling(png);
  png_read_update_info(png, info);
  stride = (has_alpha ? 4 : 3) * width * sizeof(*rgb);
  rgb = (uint8_t*)malloc(stride * height);
  if (rgb == NULL) goto Error;
  for (p = 0; p < num_passes; ++p) {
    for (y = 0; y < height; ++y) {
      png_bytep row = rgb + y * stride;
      png_read_rows(png, &row, NULL, 1);
    }
  }
  png_read_end(png, info);
  png_destroy_read_struct(&png, &info, NULL);

  pic->width = width;
  pic->height = height;
  ok = has_alpha ? WebPPictureImportRGBA(pic, rgb, stride)
                 : WebPPictureImportRGB(pic, rgb, stride);
  free(rgb);

  if (ok && has_alpha && keep_alpha == 2) {
    WebPCleanupTransparentArea(pic);
  }

 End:
  return ok;
}

static int UtilWritePNG(FILE* out_file, const WebPDecBuffer* const buffer) {
  const uint32_t width = buffer->width;
  const uint32_t height = buffer->height;
  unsigned char* const rgb = buffer->u.RGBA.rgba;
  const int stride = buffer->u.RGBA.stride;
  const int has_alpha = (buffer->colorspace == MODE_RGBA);
  png_structp png;
  png_infop info;
  png_uint_32 y;

  png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                NULL, error_function, NULL);
  if (png == NULL) {
    return 0;
  }
  info = png_create_info_struct(png);
  if (info == NULL) {
    png_destroy_write_struct(&png, NULL);
    return 0;
  }
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png, &info);
    return 0;
  }
  png_init_io(png, out_file);
  png_set_IHDR(png, info, width, height, 8,
               has_alpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);
  for (y = 0; y < height; ++y) {
    png_bytep row = rgb + y * stride;
    png_write_rows(png, &row, 1);
  }
  png_write_end(png, info);
  png_destroy_write_struct(&png, &info);
  return 1;
}

static int UtilWritePPM(FILE* fout, const WebPDecBuffer* const buffer, int alpha) {
  const uint32_t width = buffer->width;
  const uint32_t height = buffer->height;
  const unsigned char* const rgb = buffer->u.RGBA.rgba;
  const int stride = buffer->u.RGBA.stride;
  const size_t bytes_per_px = alpha ? 4 : 3;
  uint32_t y;

  if (alpha) {
    fprintf(fout, "P7\nWIDTH %d\nHEIGHT %d\nDEPTH 4\nMAXVAL 255\n"
                  "TUPLTYPE RGB_ALPHA\nENDHDR\n", width, height);
  } else {
    fprintf(fout, "P6\n%d %d\n255\n", width, height);
  }
  for (y = 0; y < height; ++y) {
    if (fwrite(rgb + y * stride, width, bytes_per_px, fout) != bytes_per_px) {
      return 0;
    }
  }
  return 1;
}

static void PutLE16(uint8_t* const dst, uint32_t value) {
  dst[0] = (value >> 0) & 0xff;
  dst[1] = (value >> 8) & 0xff;
}

static void PutLE32(uint8_t* const dst, uint32_t value) {
  PutLE16(dst + 0, (value >>  0) & 0xffff);
  PutLE16(dst + 2, (value >> 16) & 0xffff);
}

#define BMP_HEADER_SIZE 54
static int UtilWriteBMP(FILE* fout, const WebPDecBuffer* const buffer) {
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

#define NUM_IFD_ENTRIES 15
#define EXTRA_DATA_SIZE 16
// 10b for signature/header + n * 12b entries + 4b for IFD terminator:
#define EXTRA_DATA_OFFSET (10 + 12 * NUM_IFD_ENTRIES + 4)
#define TIFF_HEADER_SIZE (EXTRA_DATA_OFFSET + EXTRA_DATA_SIZE)

static int UtilWriteTIFF(FILE* fout, const WebPDecBuffer* const buffer) {
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

static int UtilWriteAlphaPlane(FILE* fout, const WebPDecBuffer* const buffer) {
  const uint32_t width = buffer->width;
  const uint32_t height = buffer->height;
  const unsigned char* const a = buffer->u.YUVA.a;
  const int a_stride = buffer->u.YUVA.a_stride;
  uint32_t y;
  assert(a != NULL);
  fprintf(fout, "P5\n%d %d\n255\n", width, height);
  for (y = 0; y < height; ++y) {
    if (fwrite(a + y * a_stride, width, 1, fout) != 1) {
      return 0;
    }
  }
  return 1;
}

static int UtilWritePGM(FILE* fout, const WebPDecBuffer* const buffer) {
  const int width = buffer->width;
  const int height = buffer->height;
  const WebPYUVABuffer* const yuv = &buffer->u.YUVA;
  // Save a grayscale PGM file using the IMC4 layout
  // (http://www.fourcc.org/yuv.php#IMC4). This is a very
  // convenient format for viewing the samples, esp. for
  // odd dimensions.
  int ok = 1;
  int y;
  const int uv_width = (width + 1) / 2;
  const int uv_height = (height + 1) / 2;
  const int out_stride = (width + 1) & ~1;
  const int a_height = yuv->a ? height : 0;
  fprintf(fout, "P5\n%d %d\n255\n", out_stride, height + uv_height + a_height);
  for (y = 0; ok && y < height; ++y) {
    ok &= (fwrite(yuv->y + y * yuv->y_stride, width, 1, fout) == 1);
    if (width & 1) fputc(0, fout);    // padding byte
  }
  for (y = 0; ok && y < uv_height; ++y) {
    ok &= (fwrite(yuv->u + y * yuv->u_stride, uv_width, 1, fout) == 1);
    ok &= (fwrite(yuv->v + y * yuv->v_stride, uv_width, 1, fout) == 1);
  }
  for (y = 0; ok && y < a_height; ++y) {
    ok &= (fwrite(yuv->a + y * yuv->a_stride, width, 1, fout) == 1);
    if (width & 1) fputc(0, fout);    // padding byte
  }
  return ok;
}

static int UtilReadTIFF(const char* const filename,
                    WebPPicture* const pic, int keep_alpha) {
  TIFF* const tif = TIFFOpen(filename, "r");
  uint32 width, height;
  uint32* raster;
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
  raster = (uint32*)_TIFFmalloc(width * height * sizeof(*raster));
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

static InputFileFormat GetImageType(FILE* in_file) {
  InputFileFormat format = UNSUPPORTED;
  unsigned int magic;
  unsigned char buf[4];

  if ((fread(&buf[0], 4, 1, in_file) != 1) ||
      (fseek(in_file, 0, SEEK_SET) != 0)) {
    return format;
  }

  magic = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
  if (magic == 0x89504E47U) {
    format = iPNG_;
  } else if (magic >= 0xFFD8FF00U && magic <= 0xFFD8FFFFU) {
    format = iJPEG_;
  } else if (magic == 0x49492A00 || magic == 0x4D4D002A) {
    format = iTIFF_;
  }
  return format;
}

int UtilReadPicture(const char* const filename, WebPPicture* const pic,
                       int keep_alpha) {
  int ok = 0;
  FILE* in_file = fopen(filename, "rb");
  if (in_file == NULL) {
    //fprintf(stderr, "Error! Cannot open input file '%s'\n", filename);
    return ok;
  }

  if (pic->width == 0 || pic->height == 0) {
    // If no size specified, try to decode it as PNG/JPEG (as appropriate).
    const InputFileFormat format = GetImageType(in_file);
    if (format == iPNG_) {
      ok = UtilReadPNG(in_file, pic, keep_alpha);
    } else if (format == iJPEG_) {
      ok = UtilReadJPEG(in_file, pic);
    } else if (format == iTIFF_) {
      ok = UtilReadTIFF(filename, pic, keep_alpha);
    }
  } else {
    // If image size is specified, infer it as YUV format.
    ok = UtilReadYUV(in_file, pic);
  }

  fclose(in_file);
  return ok;
}

int UtilSaveOutput(const WebPDecBuffer* const buffer,
                       OutputFileFormat format, const char* const out_file) {
  FILE* fout = NULL;
  int ok = 1;

  fout = fopen(out_file, "wb");
  if (!fout) {
    fprintf(stderr, "Error opening output file %s\n", out_file);
    return 0;
  }

  if (format == oPNG) {
    ok &= UtilWritePNG(fout, buffer);
  } else if (format == oPAM) {
    ok &= UtilWritePPM(fout, buffer, 1);
  } else if (format == oPPM) {
    ok &= UtilWritePPM(fout, buffer, 0);
  } else if (format == oBMP) {
    ok &= UtilWriteBMP(fout, buffer);
  } else if (format == oTIFF_) {
    ok &= UtilWriteTIFF(fout, buffer);
  } else if (format == oPGM || format == oYUV) {
    ok &= UtilWritePGM(fout, buffer);
  } else if (format == ALPHA_PLANE_ONLY) {
    ok &= UtilWriteAlphaPlane(fout, buffer);
  }

  if (fout) {
    fclose(fout);
  }
  return ok;
}

// -----------------------------------------------------------------------------
// File I/O

int UtilReadFile(const char* const file_name,
                   const uint8_t** data, size_t* data_size) {
  int ok;
  void* file_data;
  size_t file_size;
  FILE* in;

  if (file_name == NULL || data == NULL || data_size == NULL) return 0;
  *data = NULL;
  *data_size = 0;

  in = fopen(file_name, "rb");
  if (in == NULL) {
    fprintf(stderr, "cannot open input file '%s'\n", file_name);
    return 0;
  }
  fseek(in, 0, SEEK_END);
  file_size = ftell(in);
  fseek(in, 0, SEEK_SET);
  file_data = malloc(file_size);
  if (file_data == NULL) return 0;
  ok = (fread(file_data, file_size, 1, in) == 1);
  fclose(in);

  if (!ok) {
    fprintf(stderr, "Could not read %zu bytes of data from file %s\n",
            file_size, file_name);
    free(file_data);
    return 0;
  }
  *data = (uint8_t*)file_data;
  *data_size = file_size;
  return 1;
}

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

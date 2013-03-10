#include "./util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include <setjmp.h>   // note: this must be included *after* png.h
#include <jpeglib.h>

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
    format = PNG_;
  } else if (magic >= 0xFFD8FF00U && magic <= 0xFFD8FFFFU) {
    format = JPEG_;
  } else if (magic == 0x49492A00 || magic == 0x4D4D002A) {
    format = TIFF_;
  }
  return format;
}

int UtilReadPicture(const char* const filename, WebPPicture* const pic,
                       int keep_alpha) {
  int ok = 0;
  FILE* in_file = fopen(filename, "rb");
  if (in_file == NULL) {
    fprintf(stderr, "Error! Cannot open input file '%s'\n", filename);
    return ok;
  }

  if (pic->width == 0 || pic->height == 0) {
    // If no size specified, try to decode it as PNG/JPEG (as appropriate).
    const InputFileFormat format = GetImageType(in_file);
    if (format == PNG_) {
      ok = UtilReadPNG(in_file, pic, keep_alpha);
    }
    /*
    } else if (format == JPEG_) {
      ok = ReadJPEG(in_file, pic);
    } else if (format == TIFF_) {
      ok = ReadTIFF(filename, pic, keep_alpha);
    }
    */
  } else {
    // If image size is specified, infer it as YUV format.
    ok = UtilReadYUV(in_file, pic);
  }
  if (!ok) {
    fprintf(stderr, "Error! Could not process file %s\n", filename);
  }

  fclose(in_file);
  return ok;
}

void UtilSaveOutput(const WebPDecBuffer* const buffer,
                       OutputFileFormat format, const char* const out_file) {
  FILE* fout = NULL;
  int ok = 1;

  fout = fopen(out_file, "wb");
  if (!fout) {
    fprintf(stderr, "Error opening output file %s\n", out_file);
    return;
  }

  if (format == PNG) {
    ok &= UtilWritePNG(fout, buffer);
  }
  /*
  } else if (format == PAM) {
    ok &= WritePPM(fout, buffer, 1);
  } else if (format == PPM) {
    ok &= WritePPM(fout, buffer, 0);
  } else if (format == PGM) {
    ok &= WritePGM(fout, buffer);
  } else if (format == ALPHA_PLANE_ONLY) {
    ok &= WriteAlphaPlane(fout, buffer);
  }
  */
  if (fout) {
    fclose(fout);
  }
  if (ok) {
    printf("Saved file %s\n", out_file);
  } else {
    fprintf(stderr, "Error writing file %s !!\n", out_file);
  }
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

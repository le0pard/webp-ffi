#include "./pngdec.h"

#include <assert.h>
#include <stdio.h>
#include <png.h>
#include <setjmp.h>   // note: this must be included *after* png.h
#include <stdlib.h>
#include <string.h>

static void PNGAPI error_function(png_structp png, png_const_charp dummy) {
  (void)dummy;  // remove variable-unused warning
  longjmp(png_jmpbuf(png), 1);
}

int UtilReadPNG(FILE* in_file, WebPPicture* const pic, int keep_alpha) {
  png_structp png;
  png_infop info = NULL;
  png_infop end_info = NULL;
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
    png_destroy_read_struct(&png, &info, &end_info);
    goto End;
  }

  info = png_create_info_struct(png);
  if (info == NULL) goto Error;
  end_info = png_create_info_struct(png);
  if (end_info == NULL) goto Error;

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
  png_read_end(png, end_info);
  png_destroy_read_struct(&png, &info, &end_info);

  pic->width = width;
  pic->height = height;
  pic->use_argb = 1;
  ok = has_alpha ? WebPPictureImportRGBA(pic, rgb, stride)
                 : WebPPictureImportRGB(pic, rgb, stride);

  if (!ok) {
    goto Error;
  }

 End:
  free(rgb);
  return ok;
}





int UtilWritePNG(FILE* out_file, const WebPDecBuffer* const buffer) {
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


int UtilWritePPM(FILE* fout, const WebPDecBuffer* const buffer, int alpha) {
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

int UtilWriteAlphaPlane(FILE* fout, const WebPDecBuffer* const buffer) {
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

int UtilWritePGM(FILE* fout, const WebPDecBuffer* const buffer) {
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
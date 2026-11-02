#include "./pngdec.h"
#include "./util.h"

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

// libpng prints warnings to stderr by default. A library should stay quiet.
static void PNGAPI warning_function(png_structp png, png_const_charp dummy) {
  (void)png;
  (void)dummy;
}

// The metadata code below is based on libwebp's imageio/pngdec.c.

// Converts pairs of hex digits, possibly split by newlines, to 'length' bytes.
// Returns NULL for other characters or when there are fewer bytes.
static uint8_t* HexStringToBytes(const char* hexstring, size_t length) {
  const char* src = hexstring;
  size_t actual_length = 0;
  uint8_t* const raw_data = (uint8_t*)malloc(length > 0 ? length : 1);
  uint8_t* dst;

  if (raw_data == NULL) return NULL;
  for (dst = raw_data; actual_length < length && *src != '\0'; ++src) {
    char* end;
    char value[3];
    if (*src == '\n') continue;
    value[0] = *src++;
    value[1] = *src;
    value[2] = '\0';
    *dst++ = (uint8_t)strtol(value, &end, 16);
    if (end != value + 2) break;
    ++actual_length;
  }
  if (actual_length != length) {
    free(raw_data);
    return NULL;
  }
  return raw_data;
}

// ImageMagick and ExifTool store metadata in text chunks as "raw profiles":
// "\n<name>\n<length>\n<hex data>\n".
static int ProcessRawProfile(const char* profile, size_t profile_length,
                             MetadataPayload* const payload) {
  const char* src = profile;
  char* end;
  long length;

  if (profile == NULL || profile_length == 0 || *src != '\n') return 0;
  ++src;
  while (*src != '\0' && *src++ != '\n') {}  // skip the profile name
  length = strtol(src, &end, 10);
  if (*end != '\n') return 0;
  ++end;
  // two hex digits per byte, so a longer length cannot be right
  if (length < 0 || (size_t)length > profile_length / 2) return 0;
  payload->bytes = HexStringToBytes(end, (size_t)length);
  if (payload->bytes == NULL) return 0;
  payload->size = (size_t)length;
  return 1;
}

static int CopyText(const char* text, size_t length, MetadataPayload* const payload) {
  return MetadataCopy(text, length, payload);
}

static const struct {
  const char* key;
  int (*process)(const char* text, size_t length, MetadataPayload* const payload);
  int kind;  // METADATA_* flag
} kPNGTextMetadata[] = {
  // https://exiftool.org/TagNames/PNG.html#TextualData
  { "Raw profile type exif", ProcessRawProfile, METADATA_EXIF },
  { "Raw profile type xmp",  ProcessRawProfile, METADATA_XMP },
  // ExifTool puts EXIF data in an APP1 profile too; ImageMagick in "app1".
  { "Raw profile type APP1", ProcessRawProfile, METADATA_EXIF },
  { "Raw profile type app1", ProcessRawProfile, METADATA_EXIF },
  // XMP Specification Part 3, Section 3 #PNG
  { "XML:com.adobe.xmp",     CopyText,          METADATA_XMP },
  { NULL, NULL, 0 }
};

// Looks for metadata before and after the image data, preferring what comes
// first. Like libwebp, an eXIf chunk wins over EXIF in text chunks.
static int ExtractMetadataFromPNG(png_structp png, png_infop info,
                                  png_infop end_info, Metadata* const metadata) {
  int p;
  for (p = 0; p < 2; ++p) {
    png_infop const head = (p == 0) ? info : end_info;
    png_textp text = NULL;
    const png_uint_32 count = png_get_text(png, head, &text, NULL);
    png_uint_32 i;

#ifdef PNG_eXIf_SUPPORTED
    {
      png_bytep exif = NULL;
      png_uint_32 length = 0;
      if (png_get_eXIf_1(png, head, &length, &exif) == PNG_INFO_eXIf &&
          !MetadataCopy(exif, length, &metadata->exif)) {
        return 0;
      }
    }
#endif

    for (i = 0; i < count; ++i, ++text) {
      int j;
      for (j = 0; kPNGTextMetadata[j].key != NULL; ++j) {
        if (text->key != NULL && !strcmp(text->key, kPNGTextMetadata[j].key)) {
          MetadataPayload* const payload =
              (kPNGTextMetadata[j].kind == METADATA_EXIF) ? &metadata->exif : &metadata->xmp;
          // iTXt chunks keep their length in a separate field
          const size_t length = (text->compression == PNG_ITXT_COMPRESSION_NONE ||
                                 text->compression == PNG_ITXT_COMPRESSION_zTXt)
                                    ? text->itxt_length : text->text_length;
          // the first one wins; like cwebp, a broken one fails the read
          if (payload->bytes == NULL &&
              !kPNGTextMetadata[j].process(text->text, length, payload)) {
            return 0;
          }
          break;
        }
      }
    }

    {
      png_charp name;
      int compression_type;
      png_bytep profile;
      png_uint_32 length;
      if (png_get_iCCP(png, head, &name, &compression_type, &profile, &length) == PNG_INFO_iCCP &&
          !MetadataCopy(profile, length, &metadata->iccp)) {
        return 0;
      }
    }
  }
  return 1;
}

int UtilReadPNG(FILE* in_file, WebPPicture* const pic, int keep_alpha,
                Metadata* const metadata) {
  // Locals that change after setjmp() must be volatile, otherwise their values
  // are undefined when libpng jumps back to the error handler.
  volatile png_structp png = NULL;
  volatile png_infop info = NULL;
  volatile png_infop end_info = NULL;
  int color_type, bit_depth, interlaced;
  int has_alpha;
  int num_passes;
  int p;
  int ok = 0;
  png_uint_32 width, height, y;
  int stride;
  size_t rgb_size;
  uint8_t* volatile rgb = NULL;

  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png == NULL) {
    goto End;
  }

  png_set_error_fn(png, NULL, error_function, warning_function);
  if (setjmp(png_jmpbuf(png))) {
 Error:
    png_destroy_read_struct((png_structpp)&png, (png_infopp)&info,
                            (png_infopp)&end_info);
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
  if (!UtilCheckImageSize(width, height, has_alpha ? 4 : 3,
                          &stride, &rgb_size)) {
    goto Error;
  }
  rgb = (uint8_t*)malloc(rgb_size);
  if (rgb == NULL) goto Error;
  for (p = 0; p < num_passes; ++p) {
    for (y = 0; y < height; ++y) {
      png_bytep row = rgb + (size_t)y * stride;
      png_read_rows(png, &row, NULL, 1);
    }
  }
  png_read_end(png, end_info);
  if (metadata != NULL && !ExtractMetadataFromPNG(png, info, end_info, metadata)) {
    goto Error;
  }
  png_destroy_read_struct((png_structpp)&png, (png_infopp)&info,
                          (png_infopp)&end_info);

  pic->width = (int)width;
  pic->height = (int)height;
  pic->use_argb = 1;
  ok = has_alpha ? WebPPictureImportRGBA(pic, rgb, stride)
                 : WebPPictureImportRGB(pic, rgb, stride);

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
                                NULL, error_function, warning_function);
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

// Based on WebPWriteYUV in libwebp's imageio/image_enc.c.
int UtilWriteYUV(FILE* fout, const WebPDecBuffer* const buffer) {
  const int width = buffer->width;
  const int height = buffer->height;
  const WebPYUVABuffer* const yuv = &buffer->u.YUVA;
  const uint8_t* src_y = yuv->y;
  const uint8_t* src_u = yuv->u;
  const uint8_t* src_v = yuv->v;
  const uint8_t* src_a = yuv->a;
  const int uv_width = (width + 1) / 2;
  const int uv_height = (height + 1) / 2;
  const int a_height = (src_a != NULL) ? height : 0;
  int ok = 1;
  int y;

  if (src_y == NULL || src_u == NULL || src_v == NULL) return 0;
  for (y = 0; ok && y < height; ++y) {
    ok &= (fwrite(src_y, width, 1, fout) == 1);
    src_y += yuv->y_stride;
  }
  for (y = 0; ok && y < uv_height; ++y) {
    ok &= (fwrite(src_u, uv_width, 1, fout) == 1);
    src_u += yuv->u_stride;
  }
  for (y = 0; ok && y < uv_height; ++y) {
    ok &= (fwrite(src_v, uv_width, 1, fout) == 1);
    src_v += yuv->v_stride;
  }
  for (y = 0; ok && y < a_height; ++y) {
    ok &= (fwrite(src_a, width, 1, fout) == 1);
    src_a += yuv->a_stride;
  }
  return ok;
}

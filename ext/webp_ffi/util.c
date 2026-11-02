#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./util.h"
#include "./jpegdec.h"
#include "./pngdec.h"
#include "./tiffdec.h"

#include "webp/decode.h"
#include "webp/encode.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// Same memory cap as the image readers in libwebp (imageio/imageio_util.c).
#if SIZE_MAX > (1ULL << 34)
#define UTIL_MAX_ALLOCABLE_MEMORY (1ULL << 34)
#else
#define UTIL_MAX_ALLOCABLE_MEMORY ((1ULL << 31) - (1 << 16))
#endif
#define UTIL_MAX_BYTES_PER_PIXEL 8

int UtilCheckImageSize(uint64_t width, uint64_t height, uint64_t bytes_per_px,
                       int* const stride, size_t* const size) {
  uint64_t row_size, total_size;

  if (width == 0 || height == 0) return 0;
  if (bytes_per_px == 0 || bytes_per_px > UTIL_MAX_BYTES_PER_PIXEL) return 0;
  // WebPPicture and the row strides passed to libwebp are plain ints.
  if (width > INT_MAX || height > INT_MAX) return 0;

  // Both products stay far below 2^64 because of the limits checked above.
  row_size = width * bytes_per_px;
  if (row_size > INT_MAX) return 0;
  total_size = row_size * height;
  if (total_size > UTIL_MAX_ALLOCABLE_MEMORY) return 0;

  if (stride != NULL) *stride = (int)row_size;
  if (size != NULL) *size = (size_t)total_size;
  return 1;
}

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

static InputFileFormat GetImageType(FILE* in_file) {
  InputFileFormat format = UNSUPPORTED;
  unsigned int magic;
  unsigned char buf[4];

  if ((fread(&buf[0], 4, 1, in_file) != 1) ||
      (fseek(in_file, 0, SEEK_SET) != 0)) {
    return format;
  }

  // shift unsigned values: buf[0] << 24 as an int overflows for bytes >= 0x80
  magic = ((unsigned int)buf[0] << 24) | ((unsigned int)buf[1] << 16) |
          ((unsigned int)buf[2] << 8) | buf[3];
  if (magic == 0x89504E47U) {
    format = iPNG_;
  } else if (magic >= 0xFFD8FF00U && magic <= 0xFFD8FFFFU) {
    format = iJPEG_;
  } else if (magic == 0x49492A00 || magic == 0x4D4D002A) {
    format = iTIFF_;
  }
  return format;
}

int UtilReadPictureFile(FILE* const in_file, WebPPicture* const pic,
                        int keep_alpha, Metadata* const metadata) {
  int ok = 0;

  if (pic->width == 0 || pic->height == 0) {
    // If no size specified, try to decode it as PNG/JPEG/TIFF (as appropriate).
    const InputFileFormat format = GetImageType(in_file);
    if (format == iPNG_) {
      ok = UtilReadPNG(in_file, pic, keep_alpha, metadata);
    } else if (format == iJPEG_) {
      ok = UtilReadJPEG(in_file, pic, metadata);
    } else if (format == iTIFF_) {
      ok = UtilReadTIFF(in_file, pic, keep_alpha, metadata);
    }
  } else {
    // If image size is specified, infer it as YUV format.
    ok = UtilReadYUV(in_file, pic);
  }
  return ok;
}

int UtilFileExists(const char* const path) {
  struct stat info;
  return stat(path, &info) == 0;
}

int UtilSaveOutput(const WebPDecBuffer* const buffer,
                       OutputFileFormat format, const char* const out_file) {
  // a failed write removes the file, but only when this call created it
  const int existed = UtilFileExists(out_file);
  int ok;
  FILE* const fout = fopen(out_file, "wb");
  if (!fout) {
    return 0;
  }
  ok = UtilWriteOutput(buffer, format, fout);
  // fclose flushes buffered data, so it can also fail (for example, disk full)
  if (fclose(fout) != 0) {
    ok = 0;
  }
  if (!ok && !existed) remove(out_file);
  return ok;
}

int UtilWriteOutput(const WebPDecBuffer* const buffer,
                    OutputFileFormat format, FILE* const fout) {
  int ok = 1;

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
  } else if (format == oPGM) {
    ok &= UtilWritePGM(fout, buffer);
  } else if (format == oYUV) {
    ok &= UtilWriteYUV(fout, buffer);
  } else if (format == ALPHA_PLANE_ONLY) {
    ok &= UtilWriteAlphaPlane(fout, buffer);
  } else {
    ok = 0;
  }
  return ok;
}

// -----------------------------------------------------------------------------
// File I/O

int UtilReadFile(const char* const file_name,
                   const uint8_t** data, size_t* data_size) {
  int ok;
  void* file_data;
  long file_size;
  struct stat info;
  FILE* in;

  if (file_name == NULL || data == NULL || data_size == NULL) return 0;
  *data = NULL;
  *data_size = 0;

  in = fopen(file_name, "rb");
  if (in == NULL) {
    return 0;
  }
  // only regular files: on Linux a folder opens and reports a huge size
  if (fstat(fileno(in), &info) != 0 || !S_ISREG(info.st_mode) ||
      fseek(in, 0, SEEK_END) != 0 ||
      (file_size = ftell(in)) <= 0 ||
      fseek(in, 0, SEEK_SET) != 0) {
    fclose(in);
    return 0;
  }
  file_data = malloc((size_t)file_size);
  if (file_data == NULL) {
    fclose(in);
    return 0;
  }
  ok = (fread(file_data, (size_t)file_size, 1, in) == 1);
  fclose(in);

  if (!ok) {
    free(file_data);
    return 0;
  }
  *data = (uint8_t*)file_data;
  *data_size = (size_t)file_size;
  return 1;
}

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

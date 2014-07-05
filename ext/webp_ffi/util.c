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

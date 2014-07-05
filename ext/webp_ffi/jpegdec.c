#include "./jpegdec.h"

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

struct my_error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

static void my_error_exit(j_common_ptr dinfo) {
  struct my_error_mgr* myerr = (struct my_error_mgr*) dinfo->err;
  (*dinfo->err->output_message) (dinfo);
  longjmp(myerr->setjmp_buffer, 1);
}

int UtilReadJPEG(FILE* in_file, WebPPicture* const pic) {
  int ok = 0;
  int stride, width, height;
  uint8_t* rgb = NULL;
  struct jpeg_decompress_struct dinfo;
  struct my_error_mgr jerr;
  JSAMPROW buffer[1];

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
  buffer[0] = (JSAMPLE*)rgb;

  while (dinfo.output_scanline < dinfo.output_height) {
    if (jpeg_read_scanlines(&dinfo, buffer, 1) != 1) {
      goto End;
    }
    buffer[0] += stride;
  }

  jpeg_finish_decompress(&dinfo);
  jpeg_destroy_decompress(&dinfo);

  // WebP conversion.
  pic->width = width;
  pic->height = height;
  ok = WebPPictureImportRGB(pic, rgb, stride);
  if (!ok) goto Error;

 End:
  free(rgb);
  return ok;
}


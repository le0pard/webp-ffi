#include "./jpegdec.h"
#include "./util.h"

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
  longjmp(myerr->setjmp_buffer, 1);
}

// libjpeg prints errors and warnings to stderr by default. A library should
// stay quiet.
static void my_output_message(j_common_ptr dinfo) {
  (void)dinfo;
}

int UtilReadJPEG(FILE* in_file, WebPPicture* const pic) {
  int ok = 0;
  int stride, width, height;
  size_t rgb_size;
  // 'rgb' changes after setjmp(), so it must be volatile to keep its value
  // when libjpeg jumps back to the error handler.
  uint8_t* volatile rgb = NULL;
  struct jpeg_decompress_struct dinfo;
  struct my_error_mgr jerr;
  JSAMPROW buffer[1];

  dinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  jerr.pub.output_message = my_output_message;

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

  if (!UtilCheckImageSize(dinfo.output_width, dinfo.output_height,
                          dinfo.output_components, &stride, &rgb_size)) {
    goto Error;
  }
  width = (int)dinfo.output_width;
  height = (int)dinfo.output_height;

  rgb = (uint8_t*)malloc(rgb_size);
  if (rgb == NULL) {
    goto Error;
  }
  buffer[0] = (JSAMPLE*)rgb;

  while (dinfo.output_scanline < dinfo.output_height) {
    if (jpeg_read_scanlines(&dinfo, buffer, 1) != 1) {
      goto Error;
    }
    buffer[0] += stride;
  }

  jpeg_finish_decompress(&dinfo);
  jpeg_destroy_decompress(&dinfo);

  // WebP conversion.
  pic->width = width;
  pic->height = height;
  ok = WebPPictureImportRGB(pic, rgb, stride);

 End:
  free(rgb);
  return ok;
}


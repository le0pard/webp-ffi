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

// Based on the metadata code in libwebp's imageio/jpegdec.c.

// An ICC profile can be split over several APP2 segments:
// "ICC_PROFILE\0", the 1-based sequence number, the segment count, then data.
static int StoreICCP(j_decompress_ptr dinfo, MetadataPayload* const iccp) {
  static const char kSignature[] = "ICC_PROFILE";  // 12 bytes with the '\0'
  const size_t signature_length = sizeof(kSignature);
  const size_t header_length = signature_length + 2;
  const JOCTET* segments[256];
  size_t lengths[256];
  int expected_count = 0;
  int found = 0;
  size_t total_size = 0;
  size_t offset = 0;
  jpeg_saved_marker_ptr marker;
  int i;

  memset(lengths, 0, sizeof(lengths));
  for (marker = dinfo->marker_list; marker != NULL; marker = marker->next) {
    if (marker->marker == JPEG_APP0 + 2 && marker->data_length > header_length &&
        !memcmp(marker->data, kSignature, signature_length)) {
      const int seq = marker->data[signature_length];
      const int count = marker->data[signature_length + 1];
      if (seq == 0 || count == 0 || seq > count) return 0;
      if (expected_count == 0) expected_count = count;
      if (count != expected_count || lengths[seq] != 0) return 0;
      segments[seq] = marker->data + header_length;
      lengths[seq] = marker->data_length - header_length;
      total_size += lengths[seq];
      ++found;
    }
  }
  if (found == 0) return 1;  // no profile
  if (found != expected_count) return 0;  // missing segments

  iccp->bytes = (uint8_t*)malloc(total_size);
  if (iccp->bytes == NULL) return 0;
  for (i = 1; i <= expected_count; ++i) {  // in sequence order
    memcpy(iccp->bytes + offset, segments[i], lengths[i]);
    offset += lengths[i];
  }
  iccp->size = total_size;
  return 1;
}

// EXIF and XMP live in APP1 segments, after a signature that is not copied.
static int ExtractMetadataFromJPEG(j_decompress_ptr dinfo,
                                   Metadata* const metadata) {
  static const char kExifSignature[] = "Exif\0";  // "Exif\0\0", 6 bytes
  static const char kXMPSignature[] = "http://ns.adobe.com/xap/1.0/";  // 29 bytes
  jpeg_saved_marker_ptr marker;

  if (!StoreICCP(dinfo, &metadata->iccp)) return 0;
  for (marker = dinfo->marker_list; marker != NULL; marker = marker->next) {
    if (marker->marker != JPEG_APP0 + 1) continue;
    if (marker->data_length > sizeof(kExifSignature) &&
        !memcmp(marker->data, kExifSignature, sizeof(kExifSignature))) {
      if (!MetadataCopy(marker->data + sizeof(kExifSignature),
                        marker->data_length - sizeof(kExifSignature),
                        &metadata->exif)) {
        return 0;
      }
    } else if (marker->data_length > sizeof(kXMPSignature) &&
               !memcmp(marker->data, kXMPSignature, sizeof(kXMPSignature))) {
      if (!MetadataCopy(marker->data + sizeof(kXMPSignature),
                        marker->data_length - sizeof(kXMPSignature),
                        &metadata->xmp)) {
        return 0;
      }
    }
  }
  return 1;
}

int UtilReadJPEG(FILE* in_file, WebPPicture* const pic,
                 Metadata* const metadata) {
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
  if (metadata != NULL) {
    jpeg_save_markers(&dinfo, JPEG_APP0 + 1, 0xffff);  // EXIF and XMP
    jpeg_save_markers(&dinfo, JPEG_APP0 + 2, 0xffff);  // ICC profile
  }
  jpeg_read_header(&dinfo, TRUE);
  if (metadata != NULL && !ExtractMetadataFromJPEG(&dinfo, metadata)) {
    goto Error;
  }

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


#include "mkd_internal.h"
#include "opt/png/png.h"

/* Generate C into (mkd.dst) from the provided decoded image.
 */
 
static int mkd_appicon_reencode(struct png_image *image) {
  if (png_image_reformat_in_place(image,8,6)<0) return -1; // Force RGBA.
  
  mkd.dst.c=0;
  if (sr_encode_fmt(&mkd.dst,"const int fmn_appicon_w=%d;\n",image->w)<0) return -1;
  if (sr_encode_fmt(&mkd.dst,"const int fmn_appicon_h=%d;\n",image->h)<0) return -1;
  if (sr_encode_fmt(&mkd.dst,"const unsigned char fmn_appicon[]={\n")<0) return -1;
  const uint8_t *row=image->pixels;
  int yi=image->h;
  for (;yi-->0;row+=image->stride) {
    const uint8_t *srcp=row;
    int xi=image->w;
    for (;xi-->0;srcp+=4) {
      if (sr_encode_fmt(&mkd.dst,"%d,%d,%d,%d,",srcp[0],srcp[1],srcp[2],srcp[3])<0) return -1;
    }
    if (sr_encode_u8(&mkd.dst,0x0a)<0) return -1;
  }
  if (sr_encode_fmt(&mkd.dst,"};\n")<0) return -1;
  
  return 0;
}

/* --appicon, main entry point.
 */
 
int mkd_main_appicon() {
  int err;
  if ((err=mkd_read_single_input())<0) return err;
  
  struct png_image *image=png_decode(mkd.src,mkd.srcc);
  if (!image) {
    fprintf(stderr,"%s: Failed to decode PNG.\n",mkd.config.srcpathv[0]);
    return -2;
  }
  
  err=mkd_appicon_reencode(image);
  png_image_del(image);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error reencoding.\n",mkd.config.srcpathv[0]);
    return -2;
  }
  
  if ((err=mkd_write_single_output())<0) return err;
  return 0;
}

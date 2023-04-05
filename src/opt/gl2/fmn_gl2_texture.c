#include "fmn_gl2_internal.h"
#include "opt/png/png.h"

/* Cleanup.
 */
 
void fmn_gl2_texture_cleanup(struct fmn_gl2_texture *texture) {
  if (texture->texid) glDeleteTextures(1,&texture->texid);
}

/* Initialize.
 */
 
int fmn_gl2_texture_init(struct fmn_gl2_texture *texture,const void *src,int srcc) {
  
  if (!texture->texid) {
    glGenTextures(1,&texture->texid);
    if (!texture->texid) {
      glGenTextures(1,&texture->texid);
      if (!texture->texid) return -1;
    }
  }
  
  glBindTexture(GL_TEXTURE_2D,texture->texid);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  
  struct png_image *image=png_decode(src,srcc);
  if (!image) {
    fprintf(stderr,"Failed to decode image:%d, %d bytes\n",texture->imageid,srcc);
    return -2;
  }
  
  uint8_t depth=image->depth;
  uint8_t colortype=image->colortype;
  png_depth_colortype_legal(&depth,&colortype);
  png_depth_colortype_8bit(&depth,&colortype);
  png_depth_colortype_rgb(&depth,&colortype);
  struct png_image *replace=png_image_reformat(image,0,0,image->w,image->h,depth,colortype,0);
  png_image_del(image);
  if (!(image=replace)) return -1;
  
  switch ((image->depth<<8)|image->colortype) {
    case 0x0806: glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,image->w,image->h,0,GL_RGBA,GL_UNSIGNED_BYTE,replace->pixels); break;
    case 0x0802: glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,image->w,image->h,0,GL_RGB,GL_UNSIGNED_BYTE,replace->pixels); break;
    default: png_image_del(image); return -1;
  }
  texture->w=image->w;
  texture->h=image->h;
  
  png_image_del(image);
  return 0;
}

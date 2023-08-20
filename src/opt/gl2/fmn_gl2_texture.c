#include "fmn_gl2_internal.h"
#include "opt/png/png.h"

/* Cleanup.
 */
 
void fmn_gl2_texture_cleanup(struct fmn_gl2_texture *texture) {
  if (texture->texid) glDeleteTextures(1,&texture->texid);
  if (texture->fbid) glDeleteFramebuffers(1,&texture->fbid);
  memset(texture,0,sizeof(struct fmn_gl2_texture));
}
 
void fmn_gl2_texture_del(struct fmn_gl2_texture *texture) {
  if (!texture) return;
  if (texture->texid) glDeleteTextures(1,&texture->texid);
  if (texture->fbid) glDeleteFramebuffers(1,&texture->fbid);
  free(texture);
}

/* New.
 */
 
struct fmn_gl2_texture *fmn_gl2_texture_new() {
  struct fmn_gl2_texture *texture=calloc(1,sizeof(struct fmn_gl2_texture));
  if (!texture) return 0;
  return texture;
}

/* Initialize from RGBA.
 */
 
int fmn_gl2_texture_init_rgba(struct fmn_gl2_texture *texture,int w,int h,const void *v) {
  if ((w<1)||(h<1)) return -1;
  
  if (!texture->texid) {
    glGenTextures(1,&texture->texid);
    if (!texture->texid) {
      glGenTextures(1,&texture->texid);
      if (!texture->texid) return -1;
    }
  
    glBindTexture(GL_TEXTURE_2D,texture->texid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  }
  
  glBindTexture(GL_TEXTURE_2D,texture->texid);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,v);
  texture->w=w;
  texture->h=h;
  
  return 0;
}
 
int fmn_gl2_texture_init_rgb(struct fmn_gl2_texture *texture,int w,int h,const void *v) {
  if ((w<1)||(h<1)) return -1;
  
  if (!texture->texid) {
    glGenTextures(1,&texture->texid);
    if (!texture->texid) {
      glGenTextures(1,&texture->texid);
      if (!texture->texid) return -1;
    }
  
    glBindTexture(GL_TEXTURE_2D,texture->texid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  }
  
  glBindTexture(GL_TEXTURE_2D,texture->texid);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,v);
  texture->w=w;
  texture->h=h;
  
  return 0;
}

/* Initialize from PNG.
 */
 
int fmn_gl2_texture_init(struct fmn_gl2_texture *texture,const void *src,int srcc) {
  
  if (!texture->texid) {
    glGenTextures(1,&texture->texid);
    if (!texture->texid) {
      glGenTextures(1,&texture->texid);
      if (!texture->texid) return -1;
    }
    glBindTexture(GL_TEXTURE_2D,texture->texid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  }
  
  
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

  glBindTexture(GL_TEXTURE_2D,texture->texid);
  switch ((image->depth<<8)|image->colortype) {
    case 0x0806: glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,image->w,image->h,0,GL_RGBA,GL_UNSIGNED_BYTE,replace->pixels); break;
    case 0x0802: glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,image->w,image->h,0,GL_RGB,GL_UNSIGNED_BYTE,replace->pixels); break;
    default: png_image_del(image); return -1;
  }
  texture->w=image->w;
  texture->h=image->h;
  
  png_image_del(image);
  return 0;
}

/* Require framebuffer.
 */
 
int fmn_gl2_texture_require_framebuffer(struct fmn_gl2_texture *texture) {
  if (texture->fbid) return 0;
  
  glGenFramebuffers(1,&texture->fbid);
  if (!texture->fbid) {
    glGenFramebuffers(1,&texture->fbid);
    if (!texture->fbid) {
      return -1;
    }
  }
  
  glBindFramebuffer(GL_FRAMEBUFFER,texture->fbid);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture->texid,0);
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  
  return 0;
}

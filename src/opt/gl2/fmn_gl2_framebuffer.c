static int dummy=0;
#if 0 /*XXX part of fmn_texture now */
#include "fmn_gl2_internal.h"

/* Cleanup.
 */
 
void fmn_gl2_framebuffer_cleanup(struct fmn_gl2_framebuffer *framebuffer) {
  if (framebuffer->texture.texid) glDeleteTextures(1,&framebuffer->texture.texid);
  if (framebuffer->fbid) glDeleteFramebuffers(1,&framebuffer->fbid);
}

/* Init.
 */
 
int fmn_gl2_framebuffer_init(struct fmn_gl2_framebuffer *framebuffer,int w,int h) {
  if ((w<1)||(h<1)||(w>4096)||(h>4096)) {
    fprintf(stderr,"%s: Unreasonable framebuffer size %dx%d\n",__func__,w,h);
    return -2;
  }
  
  if (!framebuffer->texture.texid) {
    glGenTextures(1,&framebuffer->texture.texid);
    if (!framebuffer->texture.texid) {
      glGenTextures(1,&framebuffer->texture.texid);
      if (!framebuffer->texture.texid) {
        return -1;
      }
    }
  }
  
  if (!framebuffer->fbid) {
    glGenFramebuffers(1,&framebuffer->fbid);
    if (!framebuffer->fbid) {
      glGenFramebuffers(1,&framebuffer->fbid);
      if (!framebuffer->fbid) {
        return -1;
      }
    }
  }
  
  framebuffer->texture.w=w;
  framebuffer->texture.h=h;

  glBindTexture(GL_TEXTURE_2D,framebuffer->texture.texid);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); // TODO configurable by user
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,0);

  glBindFramebuffer(GL_FRAMEBUFFER,framebuffer->fbid);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,framebuffer->texture.texid,0);
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  
  return 0;
}
#endif

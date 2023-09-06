#include "fmn_gl2_internal.h"
#include "opt/datafile/fmn_datafile.h"

/* Use program.
 */
 
void fmn_gl2_program_use(struct bigpc_render_driver *driver,struct fmn_gl2_program *program) {
  if (DRIVER->program==program) return;
  if (DRIVER->program) {
    int i=DRIVER->program->attrc;
    while (i-->0) glDisableVertexAttribArray(i);
  }
  if (program) {
    glUseProgram(program->programid);
    int i=program->attrc;
    while (i-->0) glEnableVertexAttribArray(i);
    if (DRIVER->framebuffer) {
      glUniform2f(program->loc_screensize,DRIVER->framebuffer->w,DRIVER->framebuffer->h);
      glUniform2f(program->loc_screenoffset,DRIVER->framebuffer->border,DRIVER->framebuffer->border);
    } else {
      glUniform2f(program->loc_screensize,driver->w,driver->h);
      glUniform2f(program->loc_screenoffset,0.0f,0.0f);
    }
    if (DRIVER->texture) {
      glUniform2f(program->loc_texsize,DRIVER->texture->w,DRIVER->texture->h);
      glUniform1i(program->loc_sampler,0);
    }
  } else {
    glUseProgram(0);
  }
  DRIVER->program=program;
}

/* Fetch encoded image from datafile.
 */

struct fmn_gl2_fetch_image_context {
  struct bigpc_render_driver *driver;
  const void *serial;
  int serialc;
};

static int fmn_gl2_fetch_image_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct fmn_gl2_fetch_image_context *ctx=userdata;
  //TODO Consider qualifier
  ctx->serial=v;
  ctx->serialc=c;
  return 1;
}
 
static int fmn_gl2_fetch_image(void *dstpp,struct bigpc_render_driver *driver,int imageid) {
  struct fmn_gl2_fetch_image_context ctx={
    .driver=driver,
  };
  fmn_datafile_for_each_of_id(driver->datafile,FMN_RESTYPE_IMAGE,imageid,fmn_gl2_fetch_image_cb,&ctx);
  if (!ctx.serial) return -1;
  *(const void**)dstpp=ctx.serial;
  return ctx.serialc;
}

/* Search textures.
 */
 
static int fmn_gl2_texture_search(const struct bigpc_render_driver *driver,int imageid) {
  int lo=0,hi=DRIVER->texturec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct fmn_gl2_texture *q=DRIVER->texturev[ck];
         if (imageid<q->imageid) hi=ck;
    else if (imageid>q->imageid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Add texture to list.
 */
 
static struct fmn_gl2_texture *fmn_gl2_add_texture(struct bigpc_render_driver *driver,int p,int imageid) {
  if ((p<0)||(p>DRIVER->texturec)) return 0;
  if (p&&(imageid<=DRIVER->texturev[p-1]->imageid)) return 0;
  if ((p<DRIVER->texturec-1)&&(imageid>=DRIVER->texturev[p]->imageid)) return 0;
  if (DRIVER->texturec>=DRIVER->texturea) {
    int na=DRIVER->texturea+16;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(DRIVER->texturev,sizeof(void*)*na);
    if (!nv) return 0;
    DRIVER->texturev=nv;
    DRIVER->texturea=na;
  }
  struct fmn_gl2_texture *texture=fmn_gl2_texture_new();
  if (!texture) return 0;
  memmove(DRIVER->texturev+p+1,DRIVER->texturev+p,sizeof(void*)*(DRIVER->texturec-p));
  DRIVER->texturec++;
  DRIVER->texturev[p]=texture;
  texture->imageid=imageid;
  return texture;
}

/* Remove texture from list.
 */
 
static void fmn_gl2_texture_remove(struct bigpc_render_driver *driver,int p) {
  if ((p<0)||(p>=DRIVER->texturec)) return;
  struct fmn_gl2_texture *texture=DRIVER->texturev[p];
  fmn_gl2_texture_del(texture);
  DRIVER->texturec--;
  memmove(DRIVER->texturev+p,DRIVER->texturev+p+1,sizeof(void*)*(DRIVER->texturec-p));
  if (DRIVER->texture==texture) DRIVER->texture=0;
  if (DRIVER->framebuffer==texture) DRIVER->framebuffer=0;
}

/* Get texture from store, optionally adding.
 */
 
struct fmn_gl2_texture *fmn_gl2_get_texture(struct bigpc_render_driver *driver,int imageid,int create) {
  if (imageid<1) return 0;
  int p=fmn_gl2_texture_search(driver,imageid);
  if (p>=0) return DRIVER->texturev[p];
  if (!create) return 0;
  p=-p-1;
  return fmn_gl2_add_texture(driver,p,imageid);
}

/* Use texture, from imageid.
 */
 
int fmn_gl2_texture_use_imageid(struct bigpc_render_driver *driver,int imageid) {
  if (!imageid) {
    if (DRIVER->texture) {
      DRIVER->texture=0;
      glBindTexture(GL_TEXTURE_2D,0);
      if (DRIVER->program) {
        glUniform2f(DRIVER->program->loc_texsize,0.0f,0.0f);
      }
    }
    return 0;
  }
  if (DRIVER->texture&&(DRIVER->texture->imageid==imageid)) return 0;
  int p=fmn_gl2_texture_search(driver,imageid);
  if (p<0) {
    p=-p-1;
    const void *serial=0;
    int serialc=fmn_gl2_fetch_image(&serial,driver,imageid);
    if (serialc<0) {
      if (serialc!=-2) fprintf(stderr,"image:%d not found!\n",imageid);
      return -2;
    }
    struct fmn_gl2_texture *texture=fmn_gl2_add_texture(driver,p,imageid);
    if (!texture) return -1;
    if (fmn_gl2_texture_init(texture,serial,serialc)<0) {
      fmn_gl2_texture_remove(driver,p);
      return -1;
    }
  }
  struct fmn_gl2_texture *texture=DRIVER->texturev[p];
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,texture->texid);
  DRIVER->texture=texture;
  if (DRIVER->program) {
    glUniform2f(DRIVER->program->loc_texsize,texture->w,texture->h);
  }
  return 0;
}

/* Use texture, from object.
 */
 
int fmn_gl2_texture_use_object(struct bigpc_render_driver *driver,struct fmn_gl2_texture *texture) {
  if (texture==DRIVER->texture) return 0;
  glActiveTexture(GL_TEXTURE0);
  if (texture) {
    glBindTexture(GL_TEXTURE_2D,texture->texid);
    if (DRIVER->program) {
      glUniform2f(DRIVER->program->loc_texsize,texture->w,texture->h);
    }
  } else {
    glBindTexture(GL_TEXTURE_2D,0);
    if (DRIVER->program) {
      glUniform2f(DRIVER->program->loc_texsize,0.0f,0.0f);
    }
  }
  DRIVER->texture=texture;
  return 0;
}

/* Use framebuffer.
 */
 
int fmn_gl2_framebuffer_use_object(struct bigpc_render_driver *driver,struct fmn_gl2_texture *framebuffer) {
  if (framebuffer==DRIVER->framebuffer) return 0;
  DRIVER->texture=0;
  if (framebuffer) {
    if (!framebuffer->fbid) {
      if (fmn_gl2_texture_require_framebuffer(framebuffer)<0) return -1;
    }
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer->fbid);
    if (DRIVER->program) {
      glUniform2f(DRIVER->program->loc_screensize,framebuffer->w,framebuffer->h);
      glUniform2f(DRIVER->program->loc_screenoffset,framebuffer->border,framebuffer->border);
    }
    glViewport(0,0,framebuffer->w,framebuffer->h);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    if (DRIVER->program) {
      glUniform2f(DRIVER->program->loc_screensize,driver->w,driver->h);
      glUniform2f(DRIVER->program->loc_screenoffset,0.0f,0.0f);
    }
    glViewport(0,0,driver->w*DRIVER->viewscale,driver->h*DRIVER->viewscale);
  }
  DRIVER->framebuffer=framebuffer;
  return 0;
}

/* Use framebuffer by imageid.
 */
 
int fmn_gl2_framebuffer_use_imageid(struct bigpc_render_driver *driver,int imageid) {
  if (!imageid) {
    // Image zero is special; it means the main framebuffer.
    // NB "main framebuffer", not the real "main output". That's not addressable by the client.
    return fmn_gl2_framebuffer_use_object(driver,&DRIVER->mainfb);
  }
  // Non-resource framebuffers should have imageid==0, they won't be accessible here but will gracefully show up as "not equal".
  if (DRIVER->framebuffer&&(DRIVER->framebuffer->imageid==imageid)) return 0;
  
  int p=fmn_gl2_texture_search(driver,imageid);
  if (p<0) {
    p=-p-1;
    // If there is an image resource, load it just like other textures.
    // The client is probably going to overwrite that content, but it's still a sensible place to start.
    // If it does not exist -- more likely -- then create it, with the main framebuffer bounds and border.
    const void *serial=0;
    int serialc=fmn_gl2_fetch_image(&serial,driver,imageid);
    struct fmn_gl2_texture *texture=fmn_gl2_add_texture(driver,p,imageid);
    if (!texture) return -1;
    int err;
    if (serialc>0) err=fmn_gl2_texture_init(texture,serial,serialc);
    else {
      err=fmn_gl2_texture_init_rgba(texture,DRIVER->mainfb.w,DRIVER->mainfb.h,0);
      texture->border=DRIVER->mainfb.border;
    }
    if (err<0) {
      fmn_gl2_texture_remove(driver,p);
      return -1;
    }
  }
  
  struct fmn_gl2_texture *texture=DRIVER->texturev[p];
  return fmn_gl2_framebuffer_use_object(driver,texture);
}

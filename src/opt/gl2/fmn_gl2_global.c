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
      glUniform2f(program->loc_screensize,DRIVER->framebuffer->texture.w,DRIVER->framebuffer->texture.h);
    } else {
      glUniform2f(program->loc_screensize,driver->w,driver->h);
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
  if (!ctx.serial) {
    fprintf(stderr,"image:%d not found!\n",imageid);
    return -2;
  }
  *(const void**)dstpp=ctx.serial;
  return ctx.serialc;
}

/* Search textures.
 */
 
static int fmn_gl2_texture_search(const struct bigpc_render_driver *driver,int imageid) {
  int lo=0,hi=DRIVER->texturec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct fmn_gl2_texture *q=DRIVER->texturev+ck;
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
  if (p&&(imageid<=DRIVER->texturev[p-1].imageid)) return 0;
  if ((p<DRIVER->texturec-1)&&(imageid>=DRIVER->texturev[p].imageid)) return 0;
  if (DRIVER->texturec>=DRIVER->texturea) {
    int na=DRIVER->texturea+16;
    if (na>INT_MAX/sizeof(struct fmn_gl2_texture)) return 0;
    void *nv=realloc(DRIVER->texturev,sizeof(struct fmn_gl2_texture)*na);
    if (!nv) return 0;
    DRIVER->texturev=nv;
    DRIVER->texturea=na;
  }
  struct fmn_gl2_texture *texture=DRIVER->texturev+p;
  memmove(texture+1,texture,sizeof(struct fmn_gl2_texture)*(DRIVER->texturec-p));
  DRIVER->texturec++;
  memset(texture,0,sizeof(struct fmn_gl2_texture));
  texture->imageid=imageid;
  DRIVER->texture=0;
  return texture;
}

/* Use texture, from imageid.
 */
 
int fmn_gl2_texture_use(struct bigpc_render_driver *driver,int imageid) {
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
    if (serialc<0) return serialc;
    struct fmn_gl2_texture *texture=fmn_gl2_add_texture(driver,p,imageid);
    if (!texture) return -1;
    if (fmn_gl2_texture_init(texture,serial,serialc)<0) {
      fmn_gl2_texture_cleanup(texture);
      DRIVER->texturec--;
      memmove(texture,texture+1,sizeof(struct fmn_gl2_texture)*(DRIVER->texturec-p));
      return -1;
    }
  }
  struct fmn_gl2_texture *texture=DRIVER->texturev+p;
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
 
void fmn_gl2_texture_use_object(struct bigpc_render_driver *driver,struct fmn_gl2_texture *texture) {
  if (texture==DRIVER->texture) return;
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
}

/* Use framebuffer.
 */
 
void fmn_gl2_framebuffer_use(struct bigpc_render_driver *driver,struct fmn_gl2_framebuffer *framebuffer) {
  if (framebuffer==DRIVER->framebuffer) return;
  if (framebuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer->fbid);
    if (DRIVER->program) {
      glUniform2f(DRIVER->program->loc_screensize,framebuffer->texture.w,framebuffer->texture.h);
    }
    glViewport(0,0,framebuffer->texture.w,framebuffer->texture.h);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    if (DRIVER->program) {
      glUniform2f(DRIVER->program->loc_screensize,driver->w,driver->h);
    }
    glViewport(0,0,driver->w,driver->h);
  }
  DRIVER->framebuffer=framebuffer;
}

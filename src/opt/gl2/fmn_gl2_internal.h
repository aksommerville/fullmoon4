#ifndef FMN_GL2_INTERNAL_H
#define FMN_GL2_INTERNAL_H

#include "opt/bigpc/bigpc_render.h"
#include "opt/bigpc/bigpc_video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

#define FMN_GL2_GLSL_VERSION 120

/* GL object wrappers.
 *************************************************************/
 
struct fmn_gl2_texture {
  int imageid;
  GLuint texid;
  int w,h;
};

struct fmn_gl2_framebuffer {
  GLuint fbid;
  struct fmn_gl2_texture texture;
};

struct fmn_gl2_program {
  GLuint programid;
  int attrc;
  GLuint loc_screensize;
  GLuint loc_texsize;
  GLuint loc_sampler;
};

void fmn_gl2_texture_cleanup(struct fmn_gl2_texture *texture);
int fmn_gl2_texture_init(struct fmn_gl2_texture *texture,const void *src,int srcc);

void fmn_gl2_framebuffer_cleanup(struct fmn_gl2_framebuffer *framebuffer);
int fmn_gl2_framebuffer_init(struct fmn_gl2_framebuffer *framebuffer,int w,int h);

void fmn_gl2_program_cleanup(struct fmn_gl2_program *program);

/* (vsrc) must declare its attributes one per line, no leading space.
 * We will bind them in the order they appear, zero first.
 * All programs should have "uniform vec2 screensize;", in pixels.
 */
int fmn_gl2_program_init(
  struct fmn_gl2_program *program,
  const char *name,
  const char *vsrc,int vsrcc,
  const char *fsrc,int fsrcc
);

/* Shader programs.
 ******************************************************/

#define FMN_GL2_FOR_EACH_PROGRAM \
  _(raw) \
  _(mintile) \
  _(maxtile) \
  _(decal)
  
#define _(tag) int fmn_gl2_program_##tag##_init(struct fmn_gl2_program *program,struct bigpc_render_driver *driver);
FMN_GL2_FOR_EACH_PROGRAM
#undef _

struct fmn_gl2_vertex_raw {
  GLshort x,y;
  GLubyte r,g,b,a;
};
void fmn_gl2_draw_raw(int mode,const struct fmn_gl2_vertex_raw *vtxv,int vtxc);

struct fmn_gl2_vertex_mintile {
  GLshort x,y; // center
  GLubyte tileid;
};
void fmn_gl2_draw_mintile(const struct fmn_gl2_vertex_mintile *vtxv,int vtxc);

struct fmn_gl2_vertex_maxtile {
  GLshort x,y;
  GLubyte tileid;
  GLubyte rotate; // 1/256 turns clockwise
  GLubyte size;
  GLubyte xform;
  GLubyte tr,tg,tb,ta; // tint
  GLubyte pr,pg,pb; // primary
  GLubyte alpha;
};
void fmn_gl2_draw_maxtile(const struct fmn_gl2_vertex_maxtile *vtxv,int vtxc);

void fmn_gl2_draw_decal(
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch
);
void fmn_gl2_draw_decal_swap(
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch
);

/* Driver.
 ****************************************************************/

struct bigpc_render_driver_gl2 {
  struct bigpc_render_driver hdr;
  
  struct fmn_gl2_texture *texturev;
  int texturec,texturea;
  struct fmn_gl2_texture *texture; // WEAK. Currently bound, or null.
  
  struct fmn_gl2_framebuffer mainfb;
  struct fmn_gl2_framebuffer *framebuffer; // WEAK
  
  #define _(tag) struct fmn_gl2_program program_##tag;
  FMN_GL2_FOR_EACH_PROGRAM
  #undef _
  struct fmn_gl2_program *program; // WEAK, points to one of the named programs or null
};

#define DRIVER ((struct bigpc_render_driver_gl2*)driver)

void fmn_gl2_program_use(struct bigpc_render_driver *driver,struct fmn_gl2_program *program);
int fmn_gl2_texture_use(struct bigpc_render_driver *driver,int imageid);
void fmn_gl2_texture_use_object(struct bigpc_render_driver *driver,struct fmn_gl2_texture *texture);
void fmn_gl2_framebuffer_use(struct bigpc_render_driver *driver,struct fmn_gl2_framebuffer *framebuffer);

#endif

#ifndef FMN_GL2_INTERNAL_H
#define FMN_GL2_INTERNAL_H

#include "app/fmn_platform.h"
#include "opt/bigpc/bigpc_render.h"
#include "opt/bigpc/bigpc_video.h"
#include "opt/bigpc/bigpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if FMN_USE_macos
  #define GL_GLEXT_PROTOTYPES 1
  #include <OpenGL/gl.h>
  #define GL_PROGRAM_POINT_SIZE 0x8642
  #define glDeleteFramebuffers glDeleteFramebuffersEXT
  #define glGenFramebuffers glGenFramebuffersEXT
  #define glBindFramebuffer glBindFramebufferEXT
  #define glFramebufferTexture2D glFramebufferTexture2DEXT
  #define glCheckFramebufferStatus glCheckFramebufferStatusEXT
#else
  #define GL_GLEXT_PROTOTYPES 1
  #include <GL/gl.h>
  #include <GL/glext.h> // mswin needs this explicitly included; linux does not
#endif

//TODO What's the right way to determine the GLSL version?
#if FMN_USE_bcm
  // a proxy for older raspberry pis, which need version 100
  #define FMN_GL2_GLSL_VERSION 100
#elif FMN_USE_macos
  #define FMN_GL2_GLSL_VERSION 120
#else
  #define FMN_GL2_GLSL_VERSION 120
#endif

/* GL object wrappers.
 *************************************************************/
 
struct fmn_gl2_texture {
  int imageid;
  GLuint texid;
  GLuint fbid; // nonzero if writeable
  int w,h;
};

struct fmn_gl2_program {
  GLuint programid;
  int attrc;
  GLuint loc_screensize;
  GLuint loc_texsize;
  GLuint loc_sampler;
  GLuint locv[8]; // for program-specific uniforms
};

/* Initialize texture with a PNG file.
 * The context provides a convenience using image ids.
 * Alternately, provide packed 32-bit RGBA, LRTB as usual. (v) may be null to initialize blank.
 */
void fmn_gl2_texture_cleanup(struct fmn_gl2_texture *texture);
void fmn_gl2_texture_del(struct fmn_gl2_texture *texture);
struct fmn_gl2_texture *fmn_gl2_texture_new();
int fmn_gl2_texture_init(struct fmn_gl2_texture *texture,const void *src,int srcc);
int fmn_gl2_texture_init_rgba(struct fmn_gl2_texture *texture,int w,int h,const void *v);
int fmn_gl2_texture_init_rgb(struct fmn_gl2_texture *texture,int w,int h,const void *v);
int fmn_gl2_texture_require_framebuffer(struct fmn_gl2_texture *texture);

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
  _(decal) \
  _(recal)
  
#define _(tag) int fmn_gl2_program_##tag##_init(struct fmn_gl2_program *program,struct bigpc_render_driver *driver);
FMN_GL2_FOR_EACH_PROGRAM
#undef _

struct fmn_gl2_vertex_raw {
  GLshort x,y;
  GLubyte r,g,b,a;
};
void fmn_gl2_draw_raw(int mode,const struct fmn_gl2_vertex_raw *vtxv,int vtxc);
void fmn_gl2_draw_raw_rect(int x,int y,int w,int h,uint32_t rgba);

// mintile and maxtile, I was careful to keep the public vertex definition amenable to OpenGL, use it directly.
#define fmn_gl2_vertex_mintile fmn_draw_mintile
void fmn_gl2_draw_mintile(const struct fmn_gl2_vertex_mintile *vtxv,int vtxc);

#define fmn_gl2_vertex_maxtile fmn_draw_maxtile
void fmn_gl2_draw_maxtile(const struct fmn_gl2_vertex_maxtile *vtxv,int vtxc);

void fmn_gl2_draw_decal(
  struct bigpc_render_driver *driver,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch
);
void fmn_gl2_draw_decal_swap(
  struct bigpc_render_driver *driver,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch
);
void fmn_gl2_draw_recal(
  struct bigpc_render_driver *driver,
  struct fmn_gl2_program *program,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch,
  uint32_t rgba
);
void fmn_gl2_draw_recal_swap(
  struct bigpc_render_driver *driver,
  struct fmn_gl2_program *program,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch,
  uint32_t rgba
);

/* Driver.
 ****************************************************************/

struct bigpc_render_driver_gl2 {
  struct bigpc_render_driver hdr;
  uint8_t pixfmt; // Always FMN_VIDEO_PIXFMT_RGBA.
  
  struct fmn_gl2_texture **texturev;
  int texturec,texturea;
  struct fmn_gl2_texture *texture; // WEAK. Currently bound, or null.
  
  struct fmn_gl2_texture mainfb;
  struct fmn_gl2_texture *framebuffer; // WEAK
  
  #define _(tag) struct fmn_gl2_program program_##tag;
  FMN_GL2_FOR_EACH_PROGRAM
  #undef _
  struct fmn_gl2_program *program; // WEAK, points to one of the named programs or null
  
  // Output bounds.
  int pvw,pvh; // total output size, so we can detect changes.
  int dstx,dsty,dstw,dsth;
  
  struct fmn_gl2_vertex_raw *rawvtxv;
  int rawvtxa;
};

#define DRIVER ((struct bigpc_render_driver_gl2*)driver)

void fmn_gl2_program_use(struct bigpc_render_driver *driver,struct fmn_gl2_program *program);
int fmn_gl2_texture_use_imageid(struct bigpc_render_driver *driver,int imageid);
int fmn_gl2_texture_use_object(struct bigpc_render_driver *driver,struct fmn_gl2_texture *texture);
struct fmn_gl2_texture *fmn_gl2_get_texture(struct bigpc_render_driver *driver,int imageid,int create);

/* For framebuffers, imageid zero is (DRIVER->mainfb), and object zero is the real main output.
 * Clients are not allowed to draw to the real main, we only do that internally.
 */
int fmn_gl2_framebuffer_use_imageid(struct bigpc_render_driver *driver,int imageid);
int fmn_gl2_framebuffer_use_object(struct bigpc_render_driver *driver,struct fmn_gl2_texture *texture);

int fmn_gl2_rawvtxv_require(struct bigpc_render_driver *driver,int c);

#endif

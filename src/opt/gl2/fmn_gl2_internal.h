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
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

//TODO What's the right way to determine the GLSL version?
#if FMN_USE_bcm
  // a proxy for older raspberry pis, which need version 100
  #define FMN_GL2_GLSL_VERSION 100
#else
  #define FMN_GL2_GLSL_VERSION 120
#endif

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
  GLuint locv[8]; // for program-specific uniforms
};

/* Initialize texture with a PNG file.
 * The context provides a convenience using image ids.
 * Alternately, provide packed 32-bit RGBA, LRTB as usual.
 */
void fmn_gl2_texture_cleanup(struct fmn_gl2_texture *texture);
int fmn_gl2_texture_init(struct fmn_gl2_texture *texture,const void *src,int srcc);
int fmn_gl2_texture_init_rgba(struct fmn_gl2_texture *texture,int w,int h,const void *v);

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

struct fmn_gl2_vertex_mintile {
  GLshort x,y; // center
  GLubyte tileid;
  GLubyte xform;
};
void fmn_gl2_draw_mintile(const struct fmn_gl2_vertex_mintile *vtxv,int vtxc);

struct fmn_gl2_vertex_maxtile {
  GLshort x,y;
  GLubyte tileid;
  GLbyte rotate; // 1/256 turns clockwise
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
void fmn_gl2_draw_recal(
  struct fmn_gl2_program *program,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch,
  uint32_t rgba
);
void fmn_gl2_draw_recal_swap(
  struct fmn_gl2_program *program,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch,
  uint32_t rgba
);

/* I did a bad job splitting platform from game, and the result is that platforms are expected to know a lot about the game.
 * I mean, a lot!
 * So we split it again here: Everything Full Moony goes into gl2's "game" object.
 * The rest of this gl2 unit is pretty much generic.
 *******************************************************************/
 
#define FMN_GL2_COMPASS_RATE_MIN 0.010f
#define FMN_GL2_COMPASS_RATE_MAX 0.200f
#define FMN_GL2_COMPASS_DISTANCE_MAX 40.0f

#define FMN_GL2_ILLUMINATION_PERIOD 70 /* frames per cycle */
#define FMN_GL2_ILLUMINATION_RANGE 0.250f /* full range width in alpha units */
#define FMN_GL2_ILLUMINATION_FADE_TIME 1.0f /* seconds */
#define FMN_GL2_ILLUMINATION_PEAK 0.750f /* RANGE..1 */

#define FMN_GL2_TRANSITION_FRAMEC 36 /* web: 600 ms */

// Wind and rain share a particle buffer.
#define FMN_GL2_PARTICLE_LIMIT 500
 
struct fmn_gl2_game {
  int map_dirty;
  struct fmn_gl2_framebuffer mapbits;
  int tilesize;
  struct fmn_gl2_vertex_mintile *mintile_vtxv;
  int mintile_vtxc,mintile_vtxa;
  int framec;
  int itemtime; // how many frames item has been active
  int ffchargeframe; // specific to werewolf
  float compassangle;
  int transition,transitionp,transitionc; // (p) counts up to (c)
  struct fmn_gl2_framebuffer transitionfrom,transitionto;
  uint32_t transitioncolor; // for fade. rgbx
  int16_t transitionfromx,transitionfromy,transitiontox,transitiontoy; // for spotlight
  int hero_above_transition;
  struct fmn_gl2_vertex_raw particlev[FMN_GL2_PARTICLE_LIMIT];
  int particlec;
  float illuminationp;
};

void fmn_gl2_game_cleanup(struct bigpc_render_driver *driver);
int fmn_gl2_game_init(struct bigpc_render_driver *driver);
void fmn_gl2_game_render(struct bigpc_render_driver *driver);

int fmn_gl2_game_add_mintile_vtx_pixcoord(
  struct bigpc_render_driver *driver,
  int16_t x,int16_t y,uint8_t tileid,uint8_t xform
);
 
#define fmn_gl2_game_add_mintile_vtx(driver,sx,sy,tileid,xform) \
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,(sx)*DRIVER->game.tilesize,(sy)*DRIVER->game.tilesize,tileid,xform)
  
void fmn_gl2_game_transition_begin(struct bigpc_render_driver *driver);
void fmn_gl2_game_transition_commit(struct bigpc_render_driver *driver);
void fmn_gl2_game_transition_apply(
  struct bigpc_render_driver *driver,
  int transition,int p,int c,
  struct fmn_gl2_framebuffer *from,struct fmn_gl2_framebuffer *to
);

void fmn_gl2_transition_get_hero_offset(int16_t *x,int16_t *y,struct bigpc_render_driver *driver);

/* Chalk bits are 0x000fffff, and points are (0,0) thru (2,2).
 * Pack the points in the four low nybbles:
 *   0x0000f000 ax
 *   0x00000f00 ay
 *   0x000000f0 bx
 *   0x0000000f by
 * In a canonical point, a<b. But we accept it in either order.
 */
uint32_t fmn_gl2_chalk_points_from_bit(uint32_t bit);
uint32_t fmn_gl2_chalk_bit_from_points(uint32_t points);

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
  
  struct fmn_gl2_game game;
  
  // Output bounds.
  int pvw,pvh; // total output size, so we can detect changes.
  int dstx,dsty,dstw,dsth;
};

#define DRIVER ((struct bigpc_render_driver_gl2*)driver)

void fmn_gl2_program_use(struct bigpc_render_driver *driver,struct fmn_gl2_program *program);
int fmn_gl2_texture_use(struct bigpc_render_driver *driver,int imageid);
void fmn_gl2_texture_use_object(struct bigpc_render_driver *driver,struct fmn_gl2_texture *texture);
void fmn_gl2_framebuffer_use(struct bigpc_render_driver *driver,struct fmn_gl2_framebuffer *framebuffer);

#endif

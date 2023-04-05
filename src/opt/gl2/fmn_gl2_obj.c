#include "fmn_gl2_internal.h"

/* Cleanup.
 */
 
static void _gl2_del(struct bigpc_render_driver *driver) {
  if (DRIVER->texturev) {
    while (DRIVER->texturec-->0) fmn_gl2_texture_cleanup(DRIVER->texturev+DRIVER->texturec);
    free(DRIVER->texturev);
  }
  fmn_gl2_framebuffer_cleanup(&DRIVER->mainfb);
  #define _(tag) fmn_gl2_program_cleanup(&DRIVER->program_##tag);
  FMN_GL2_FOR_EACH_PROGRAM
  #undef _
}

/* Init.
 */
 
static int _gl2_init(struct bigpc_render_driver *driver,struct bigpc_video_driver *video) {
  int err;

  #define _(tag) if ((err=fmn_gl2_program_##tag##_init(&DRIVER->program_##tag,driver))<0) { \
    if (err!=-2) fprintf(stderr,"gl2:%s: Failed to initialize shader.\n",#tag); \
    return -2; \
  }
  FMN_GL2_FOR_EACH_PROGRAM
  #undef _
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_POINT_SPRITE);
  
  if (fmn_gl2_framebuffer_init(&DRIVER->mainfb,video->fbw,video->fbh)<0) return -1;
  
  return 0;
}

/* Update.
 */
 
static uint8_t clock=0;
 
static int _gl2_update(struct bigpc_image *fb,struct bigpc_render_driver *driver) {
  clock++;
  //glViewport(0,0,driver->w,driver->h);
  
  fmn_gl2_framebuffer_use(driver,&DRIVER->mainfb);
  glClearColor(0.0f,0.3f,0.0f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  
  { // raw: ok
    struct fmn_gl2_vertex_raw vtxv[]={
      { 10, 10,0xff,0x00,0x00,0xff},
      { 15, 30,0x00,0xff,0x00,0xff},
      { 30, 15,0x00,0x00,0xff,0xff},
    };
    fmn_gl2_program_use(driver,&DRIVER->program_raw);
    fmn_gl2_draw_raw(GL_TRIANGLES,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
  
  { // mintile: ok
    struct fmn_gl2_vertex_mintile vtxv[]={
      {100, 20,0x00},
      {120, 20,0x10},
      {136, 20,0x11},
      {152, 20,0x12},
    };
    fmn_gl2_program_use(driver,&DRIVER->program_mintile);
    fmn_gl2_texture_use(driver,1);
    fmn_gl2_draw_mintile(vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
  
  { // maxtile: ok
    struct fmn_gl2_vertex_maxtile vtxv[]={
      // x,  y,tile, rot,size,xform, -r-  -g-  -b- tint, -r-   primary,alpha
      { 20, 50,0x00,0x00+clock,  16,    0,0xff,0x00,0x00,0x00,0x80,0x80,0x80,0xff},
      { 40, 50,0x00,0x10+clock,  20,    0,0x00,0xff,0x00,0x40,0x80,0x80,0x80,0xe0},
      { 60, 50,0x00,0x20+clock,  24,    0,0x00,0x00,0xff,0x80,0x80,0x80,0x80,0xc0},
      { 80, 50,0x00,0x30+clock,  28,    0,0xff,0xff,0x00,0xc0,0x80,0x80,0x80,0xa0},
      {100, 50,0x00,0x40+clock,  32,    0,0x00,0xff,0xff,0xff,0x80,0x80,0x80,0x80},
      { 00, 80,0x81,clock,  32,    4,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0xff},
      { 40, 80,0x81,0x00,  32,    5,0xff,0xff,0xff,clock,0xff,0x00,0x00,0xff},
      { 80, 80,0x81,0x00,  32,    6,0x00,0x00,0x00,clock,0x00,0xff,0x00,0xff},
      {120, 80,0x81,0x00,  32,    7,0xff,0x80,0x00,0xff-clock,0x00,0x00,0xff,0xff},
    };
    fmn_gl2_program_use(driver,&DRIVER->program_maxtile);
    fmn_gl2_texture_use(driver,2);
    fmn_gl2_draw_maxtile(vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
  
  // decal: ok
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  fmn_gl2_texture_use(driver,3);
  fmn_gl2_draw_decal(     150, 80, 50,100,0,0,16,16);
  fmn_gl2_draw_decal_swap(210, 80,100,100,16,32,16,16);
  
  fmn_gl2_framebuffer_use(driver,0);
  glClearColor(0.0f,0.0f,1.0f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  fmn_gl2_texture_use_object(driver,&DRIVER->mainfb.texture);
  fmn_gl2_draw_decal(
    10,10,driver->w-20,driver->h-20,
    0,DRIVER->mainfb.texture.h,DRIVER->mainfb.texture.w,-DRIVER->mainfb.texture.h
  );
  
  return 0;
}

/* Type.
 */
 
const struct bigpc_render_type bigpc_render_type_gl2={
  .name="gl2",
  .desc="OpenGL 2+. Recommended.",
  .objlen=sizeof(struct bigpc_render_driver_gl2),
  .del=_gl2_del,
  .init=_gl2_init,
  .update=_gl2_update,
};

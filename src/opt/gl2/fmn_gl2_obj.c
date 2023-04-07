#include "fmn_gl2_internal.h"

/* Cleanup.
 */
 
static void _gl2_del(struct bigpc_render_driver *driver) {
  fmn_gl2_game_cleanup(driver);
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
  
  if (fmn_gl2_game_init(driver)<0) return -1;
  
  return 0;
}

/* Freshen output bounds if needed.
 */
 
static void fmn_gl2_require_output_bounds(struct bigpc_render_driver *driver) {
  if ((DRIVER->pvw==driver->w)&&(DRIVER->pvh==driver->h)) return;
  int fbw=DRIVER->mainfb.texture.w;
  int fbh=DRIVER->mainfb.texture.h;
  
  int wforh=(fbw*driver->h)/fbh;
  if (wforh<=driver->w) {
    DRIVER->dstw=wforh;
    DRIVER->dsth=driver->h;
  } else {
    DRIVER->dstw=driver->w;
    DRIVER->dsth=(fbh*driver->w)/fbw;
  }
  
  //TODO If it's close, snap to integers.
  //TODO If it's close to full, and scaled a few times, snap to edges.
  //TODO Or toggle interpolation depending on scale?
  
  DRIVER->dstx=(driver->w>>1)-(DRIVER->dstw>>1);
  DRIVER->dsty=(driver->h>>1)-(DRIVER->dsth>>1);
  
  DRIVER->pvw=driver->w;
  DRIVER->pvh=driver->h;
}

/* Update.
 * The render code here is only for shovelling the framebuffer up to the main screen.
 * Everything else, the fun parts, is in fmn_gl2_game_render().
 */
 
static int _gl2_update(struct bigpc_image *fb,struct bigpc_render_driver *driver) {

  fmn_gl2_game_render(driver);
  
  fmn_gl2_framebuffer_use(driver,0);
  fmn_gl2_require_output_bounds(driver);
  
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  fmn_gl2_texture_use_object(driver,&DRIVER->mainfb.texture);
  fmn_gl2_draw_decal(
    DRIVER->dstx,DRIVER->dsty,DRIVER->dstw,DRIVER->dsth,
    0,DRIVER->mainfb.texture.h,DRIVER->mainfb.texture.w,-DRIVER->mainfb.texture.h
  );
  
  // When the projected framebuffer is smaller than the screen, we letterbox or pillarbox.
  // It's more efficient to draw the bars on top, rather than the perhaps more obvious clear output first.
  if (DRIVER->dstw<driver->w) {
    fmn_gl2_program_use(driver,&DRIVER->program_raw);
    fmn_gl2_draw_raw_rect(0,0,DRIVER->dstx,driver->h,0x000000ff);
    fmn_gl2_draw_raw_rect(DRIVER->dstx+DRIVER->dstw,0,driver->w-DRIVER->dstw-DRIVER->dstx,driver->h,0x000000ff);
  }
  if (DRIVER->dsth<driver->h) {
    fmn_gl2_program_use(driver,&DRIVER->program_raw);
    fmn_gl2_draw_raw_rect(0,0,driver->w,DRIVER->dsty,0x000000ff);
    fmn_gl2_draw_raw_rect(0,DRIVER->dsty+DRIVER->dsth,driver->w,driver->h-DRIVER->dsth-DRIVER->dsty,0x000000ff);
  }
  
  return 0;
}

/* Full Moon specific hooks.
 */
 
static void _gl2_map_dirty(struct bigpc_render_driver *driver) {
  DRIVER->game.map_dirty=1;
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
  .map_dirty=_gl2_map_dirty,
};

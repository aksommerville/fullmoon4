#include "../fmn_gl2_internal.h"
#include "opt/bigpc/bigpc_menu.h"

/* Render gameover menu.
 */
 
void fmn_gl2_render_menu_gameover(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  #if 0
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw_rect(0,0,DRIVER->mainfb.w,DRIVER->mainfb.h,0x000000ff);
  if (fmn_gl2_texture_use_imageid(driver,14)>=0) {
    fmn_gl2_program_use(driver,&DRIVER->program_decal);
    int16_t w=192,h=32;
    int16_t dstx=(DRIVER->mainfb.w>>1)-(w>>1);
    int16_t dsty=(DRIVER->mainfb.h>>1)-(h>>1);
    fmn_gl2_draw_decal(dstx,dsty,w,h,0,48,w,h);
  }
  #endif
}

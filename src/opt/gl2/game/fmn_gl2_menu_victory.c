#include "../fmn_gl2_internal.h"
#include "opt/bigpc/bigpc_menu.h"

/* Render victory menu.
 */
 
void fmn_gl2_render_menu_victory(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw_rect(0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,0x000000ff);
  if (fmn_gl2_texture_use(driver,14)>=0) {
    fmn_gl2_program_use(driver,&DRIVER->program_decal);
    int16_t w=128,h=48;
    int16_t dstx=(DRIVER->mainfb.texture.w>>1)-(w>>1);
    int16_t dsty=16;
    fmn_gl2_draw_decal(dstx,dsty,w,h,0,0,w,h);
  }
}

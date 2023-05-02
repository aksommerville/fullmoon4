int fmn_gl2_menu_dummy=0;
#if 0 /*XXX move to client*/
/* fmn_gl2_menu.c
 */
 
#include "../fmn_gl2_internal.h"
#include "opt/bigpc/bigpc_menu.h"

void fmn_gl2_render_menu_prompt(struct bigpc_render_driver *driver,struct bigpc_menu *menu);
void fmn_gl2_render_menu_pause(struct bigpc_render_driver *driver,struct bigpc_menu *menu);
void fmn_gl2_render_menu_chalk(struct bigpc_render_driver *driver,struct bigpc_menu *menu);
void fmn_gl2_render_menu_treasure(struct bigpc_render_driver *driver,struct bigpc_menu *menu);
void fmn_gl2_render_menu_gameover(struct bigpc_render_driver *driver,struct bigpc_menu *menu);
void fmn_gl2_render_menu_victory(struct bigpc_render_driver *driver,struct bigpc_menu *menu);
void fmn_gl2_render_menu_hello(struct bigpc_render_driver *driver,struct bigpc_menu *menu);

/* Render menu, main entry point for all menu types.
 */
 
void fmn_gl2_render_menu(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  // If the menu's controller hasn't seen it yet (ie framec zero, no updates), black out and quietly back away.
  if (!menu->framec) {
    fmn_gl2_program_use(driver,&DRIVER->program_raw);
    fmn_gl2_draw_raw_rect(0,0,DRIVER->mainfb.w,DRIVER->mainfb.h,0x000000ff);
    return;
  }
  
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw_rect(0,0,DRIVER->mainfb.w,DRIVER->mainfb.h,0x000000c0);
  if (menu->prompt>0) {
    fmn_gl2_render_menu_prompt(driver,menu);
  } else switch (menu->prompt) {
    case FMN_MENU_PAUSE: fmn_gl2_render_menu_pause(driver,menu); break;
    case FMN_MENU_CHALK: fmn_gl2_render_menu_chalk(driver,menu); break;
    case FMN_MENU_TREASURE: fmn_gl2_render_menu_treasure(driver,menu); break;
    case FMN_MENU_GAMEOVER: fmn_gl2_render_menu_gameover(driver,menu); break;
    case FMN_MENU_VICTORY: fmn_gl2_render_menu_victory(driver,menu); break;
    case FMN_MENU_HELLO: fmn_gl2_render_menu_hello(driver,menu); break;
  }
}
#endif

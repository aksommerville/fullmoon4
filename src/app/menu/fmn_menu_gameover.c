#include "fmn_menu_internal.h"

/* Dismiss.
 */
 
static void gameover_dismiss(struct fmn_menu *menu) {
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
    fmn_game_load_map(1);
  }
}

/* Update.
 */
 
static void _gameover_update(struct fmn_menu *menu,float elapsed,uint8_t input) {
  if (input!=menu->pvinput) {
    if ((input&FMN_INPUT_USE)&&!(menu->pvinput&FMN_INPUT_USE)) { gameover_dismiss(menu); return; }
    if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) { gameover_dismiss(menu); return; }
    menu->pvinput=input;
  }
}

/* Render.
 */
 
static void _gameover_render(struct fmn_menu *menu) {

  // Blackout.
  {
    struct fmn_draw_rect vtx={0,0,menu->fbw,menu->fbh,0x000000ff};
    fmn_draw_rect(&vtx,1);
  }
  
  // "Game Over".
  {
    int16_t w=192,h=32;
    int16_t dstx=(menu->fbw>>1)-(w>>1);
    int16_t dsty=(menu->fbh>>1)-(h>>1);
    int16_t srcx=0;
    int16_t srcy=48;
    struct fmn_draw_decal vtx={dstx,dsty,w,h,srcx,srcy,w,h};
    fmn_draw_decal(&vtx,1,14);
  }
}

/* Init.
 */
 
void fmn_menu_init_GAMEOVER(struct fmn_menu *menu) {
  menu->update=_gameover_update;
  menu->render=_gameover_render;
  menu->opaque=1;
  //TODO change music
}

#include "fmn_menu_internal.h"

#define bgcolor menu->argv[0]

/* Dismiss.
 */
 
static void settings_dismiss(struct fmn_menu *menu) {
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
  }
}

/* Activate selection.
 */
 
static void settings_activate(struct fmn_menu *menu) {
  //fprintf(stderr,"%s\n",__func__);//TODO
}

/* Adjust value of selection. (horz)
 */
 
static void settings_adjust(struct fmn_menu *menu,int8_t d) {
  //fprintf(stderr,"%s %+d\n",__func__,d);//TODO
}

/* Move selection. (vert)
 */
 
static void settings_move(struct fmn_menu *menu,int8_t d) {
  //fprintf(stderr,"%s %+d\n",__func__,d);//TODO
}

/* Update.
 */
 
static void _settings_update(struct fmn_menu *menu,float elapsed,uint8_t input) {
  if (input!=menu->pvinput) {
    if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) { settings_dismiss(menu); return; }
    if ((input&FMN_INPUT_USE)&&!(menu->pvinput&FMN_INPUT_USE)) settings_activate(menu);
    const uint8_t verts=FMN_INPUT_UP|FMN_INPUT_DOWN;
    const uint8_t horzs=FMN_INPUT_LEFT|FMN_INPUT_RIGHT;
    switch ((input&verts)&~menu->pvinput) {
      case FMN_INPUT_UP: settings_move(menu,-1); break;
      case FMN_INPUT_DOWN: settings_move(menu,1); break;
    }
    switch ((input&horzs)&~menu->pvinput) {
      case FMN_INPUT_LEFT: settings_adjust(menu,-1); break;
      case FMN_INPUT_RIGHT: settings_adjust(menu,1); break;
    }
    menu->pvinput=input;
  }
}

/* Render.
 */
 
static void _settings_render(struct fmn_menu *menu) {
  const int16_t tilesize=menu->fbw/FMN_COLC;
  
  // Blackout.
  {
    struct fmn_draw_rect vtx={0,0,menu->fbw,menu->fbh,bgcolor};
    fmn_draw_rect(&vtx,1);
  }
  
  //TODO render settings menu
}

/* Init.
 */
 
void fmn_menu_init_SETTINGS(struct fmn_menu *menu) {
  menu->update=_settings_update;
  menu->render=_settings_render;
  menu->opaque=1;
  bgcolor=fmn_video_pixel_from_rgba(0x100020ff);
}

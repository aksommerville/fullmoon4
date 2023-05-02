#include "fmn_menu_internal.h"

struct fmn_menu_global fmn_menu_global={0};

/* Begin menu, public entry point.
 */
 
struct fmn_menu *fmn_begin_menu(int menuid,int arg0) {
  if (fmn_menu_global.c>=FMN_MENU_LIMIT) return 0;
  struct fmn_menu *menu=fmn_menu_global.v+fmn_menu_global.c++;
  memset(menu,0,sizeof(struct fmn_menu));
  menu->menuid=menuid;
  menu->argv[0]=arg0;
  menu->pvinput=0xff;
  fmn_video_get_framebuffer_size(&menu->fbw,&menu->fbh);
  switch (menuid) {
    case FMN_MENU_PAUSE: fmn_menu_init_PAUSE(menu); break;
    case FMN_MENU_CHALK: fmn_menu_init_CHALK(menu); break;
    case FMN_MENU_TREASURE: fmn_menu_init_TREASURE(menu); break;
    case FMN_MENU_VICTORY: fmn_menu_init_VICTORY(menu); break;
    case FMN_MENU_GAMEOVER: fmn_menu_init_GAMEOVER(menu); break;
    case FMN_MENU_HELLO: fmn_menu_init_HELLO(menu); break;
  }
  return menu;
}

/* Top menu.
 */
 
struct fmn_menu *fmn_get_top_menu() {
  if (fmn_menu_global.c<1) return 0;
  return fmn_menu_global.v+fmn_menu_global.c-1;
}

/* Dismiss one menu.
 */

void fmn_dismiss_top_menu() {
  if (fmn_menu_global.c<1) return;
  // No cleanup, no callbacks... oh this is too easy
  fmn_menu_global.c--;
}

void fmn_dismiss_menu(struct fmn_menu *menu) {
  int p=menu-fmn_menu_global.v;
  if ((p<0)||(p>=fmn_menu_global.c)) return;
  fmn_menu_global.c--;
  memmove(menu,menu+1,sizeof(struct fmn_menu)*(fmn_menu_global.c-p));
}
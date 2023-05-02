#ifndef FMN_MENU_INTERNAL_H
#define FMN_MENU_INTERNAL_H

#include "app/fmn_platform.h"
#include "app/fmn_game.h"

/* I think there won't actually be more than one at a time, but not sure.
 */
#define FMN_MENU_LIMIT 8

extern struct fmn_menu_global {
  struct fmn_menu v[FMN_MENU_LIMIT];
  uint8_t c;
} fmn_menu_global;

void fmn_menu_init_PAUSE(struct fmn_menu *menu);
void fmn_menu_init_CHALK(struct fmn_menu *menu);
void fmn_menu_init_TREASURE(struct fmn_menu *menu);
void fmn_menu_init_VICTORY(struct fmn_menu *menu);
void fmn_menu_init_GAMEOVER(struct fmn_menu *menu);
void fmn_menu_init_HELLO(struct fmn_menu *menu);

#endif

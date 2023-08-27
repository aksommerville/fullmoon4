#ifndef FMN_MENU_INTERNAL_H
#define FMN_MENU_INTERNAL_H

#include "app/fmn_platform.h"
#include "app/fmn_game.h"
#include <stdio.h>

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
void fmn_menu_init_SETTINGS(struct fmn_menu *menu);
void fmn_menu_init_CREDITS(struct fmn_menu *menu);
void fmn_menu_init_INPUT(struct fmn_menu *menu);
void fmn_menu_init_SOUNDCHECK(struct fmn_menu *menu);
void fmn_menu_init_ARCADE(struct fmn_menu *menu);

/* Replace image content with a single line of text.
 * (forcew,forceh) (0,0) to use the minimum.
 */
void fmn_generate_text_image(uint16_t imageid,const char *src,int srcc,int16_t forcew,int16_t forceh);
void fmn_generate_string_image(uint16_t imageid,uint16_t stringid,int16_t forcew,int16_t forceh);

#endif

#if 0 /*XXX move to client*/
/* bigpc_menu_pause.c
 * The item-select menu.
 */
 
#include "../bigpc_internal.h"

/* Instance definition.
 */
 
struct bigpc_menu_pause {
  struct bigpc_menu hdr;
};

#define MENU ((struct bigpc_menu_pause*)menu)

/* Cleanup.
 */
 
static void _pause_del(struct bigpc_menu *menu) {
}

/* Ready.
 */
 
static int _pause_ready(struct bigpc_menu *menu) {
  return 0;
}

/* Update.
 */
 
static int _pause_update(struct bigpc_menu *menu,uint8_t new_input) {
  if (new_input) {
    if (new_input&(FMN_INPUT_USE|FMN_INPUT_MENU)) {
      bigpc_menu_callback_any(menu);
      return 0;
    }
    if (new_input&FMN_INPUT_LEFT) menu->selx=(menu->selx-1)&3;
    if (new_input&FMN_INPUT_RIGHT) menu->selx=(menu->selx+1)&3;
    if (new_input&FMN_INPUT_UP) menu->sely=(menu->sely-1)&3;
    if (new_input&FMN_INPUT_DOWN) menu->sely=(menu->sely+1)&3;
    fmn_global.selected_item=(menu->sely<<2)|menu->selx;
  }
  return 1;
}

/* New.
 */
 
struct bigpc_menu *bigpc_menu_new_PAUSE() {
  struct bigpc_menu *menu=calloc(1,sizeof(struct bigpc_menu_pause));
  if (!menu) return 0;
  menu->prompt=FMN_MENU_PAUSE;
  menu->pvinput=0xff;
  menu->selx=fmn_global.selected_item&3;
  menu->sely=fmn_global.selected_item>>2;
  menu->del=_pause_del;
  menu->ready=_pause_ready;
  menu->update=_pause_update;
  return menu;
}
#endif

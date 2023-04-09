/* bigpc_menu_victory.c
 * Final stats after killing the werewolf.
 * Eventually this will be the venue for the extended ending sequence.
 */
 
#include "../bigpc_internal.h"

/* Instance definition.
 */
 
struct bigpc_menu_victory {
  struct bigpc_menu hdr;
};

#define MENU ((struct bigpc_menu_victory*)menu)

/* Cleanup.
 */
 
static void _victory_del(struct bigpc_menu *menu) {
}

/* Ready.
 */
 
static int _victory_ready(struct bigpc_menu *menu) {
  return 0;
}

/* Update.
 */
 
static int _victory_update(struct bigpc_menu *menu,uint8_t new_input) {
  if (new_input&(FMN_INPUT_USE|FMN_INPUT_MENU)) {
    bigpc_menu_callback_any(menu);
    return 0;
  }
  return 1;
}

/* New.
 */
 
struct bigpc_menu *bigpc_menu_new_VICTORY() {
  struct bigpc_menu *menu=calloc(1,sizeof(struct bigpc_menu_victory));
  if (!menu) return 0;
  menu->prompt=FMN_MENU_VICTORY;
  menu->pvinput=0xff;
  menu->del=_victory_del;
  menu->ready=_victory_ready;
  menu->update=_victory_update;
  return menu;
}

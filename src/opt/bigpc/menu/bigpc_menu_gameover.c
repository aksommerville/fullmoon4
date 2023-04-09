/* bigpc_menu_gameover.c
 * Tells the user she is dead.
 */
 
#include "../bigpc_internal.h"

/* Instance definition.
 */
 
struct bigpc_menu_gameover {
  struct bigpc_menu hdr;
};

#define MENU ((struct bigpc_menu_gameover*)menu)

/* Cleanup.
 */
 
static void _gameover_del(struct bigpc_menu *menu) {
}

/* Ready.
 */
 
static int _gameover_ready(struct bigpc_menu *menu) {
  return 0;
}

/* Update.
 */
 
static int _gameover_update(struct bigpc_menu *menu,uint8_t new_input) {
  if (new_input&(FMN_INPUT_USE|FMN_INPUT_MENU)) {
    bigpc_menu_callback_any(menu);
    return 0;
  }
  return 1;
}

/* New.
 */
 
struct bigpc_menu *bigpc_menu_new_GAMEOVER() {
  struct bigpc_menu *menu=calloc(1,sizeof(struct bigpc_menu_gameover));
  if (!menu) return 0;
  menu->prompt=FMN_MENU_GAMEOVER;
  menu->pvinput=0xff;
  menu->del=_gameover_del;
  menu->ready=_gameover_ready;
  menu->update=_gameover_update;
  return menu;
}

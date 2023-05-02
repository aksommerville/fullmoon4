#if 0 /*XXX move to client*/
/* bigpc_menu_treasure.c
 * Shows off a new treasure.
 */
 
#include "../bigpc_internal.h"

/* Instance definition.
 */
 
struct bigpc_menu_treasure {
  struct bigpc_menu hdr;
  uint8_t itemid;
};

#define MENU ((struct bigpc_menu_treasure*)menu)

/* Cleanup.
 */
 
static void _treasure_del(struct bigpc_menu *menu) {
}

/* Ready.
 */
 
static int _treasure_ready(struct bigpc_menu *menu) {
  return 0;
}

/* Update.
 */
 
static int _treasure_update(struct bigpc_menu *menu,uint8_t new_input) {
  if (new_input&(FMN_INPUT_USE|FMN_INPUT_MENU)) {
    bigpc_menu_callback_any(menu);
    return 0;
  }
  return 1;
}

/* New.
 */
 
struct bigpc_menu *bigpc_menu_new_TREASURE() {
  struct bigpc_menu *menu=calloc(1,sizeof(struct bigpc_menu_treasure));
  if (!menu) return 0;
  menu->prompt=FMN_MENU_TREASURE;
  menu->pvinput=0xff;
  menu->del=_treasure_del;
  menu->ready=_treasure_ready;
  menu->update=_treasure_update;
  return menu;
}
#endif

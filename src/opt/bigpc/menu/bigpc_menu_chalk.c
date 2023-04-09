/* bigpc_menu_chalk.c
 * Interactive chalk-drawing menu.
 */
 
#include "../bigpc_internal.h"

/* Instance definition.
 */
 
struct bigpc_menu_chalk {
  struct bigpc_menu hdr;
  uint32_t bits;
};

#define MENU ((struct bigpc_menu_chalk*)menu)

/* Cleanup.
 */
 
static void _chalk_del(struct bigpc_menu *menu) {
}

/* Ready.
 */
 
static int _chalk_ready(struct bigpc_menu *menu) {
  return 0;
}

/* Update.
 */
 
static int _chalk_update(struct bigpc_menu *menu,uint8_t new_input) {
  if (new_input) {
    if (new_input&FMN_INPUT_MENU) {
      bigpc_menu_callback_any(menu);
      return 0;
    }
  }
  return 1;
}

/* New.
 */
 
struct bigpc_menu *bigpc_menu_new_CHALK(uint32_t bits) {
  struct bigpc_menu *menu=calloc(1,sizeof(struct bigpc_menu_chalk));
  if (!menu) return 0;
  menu->prompt=FMN_MENU_CHALK;
  menu->pvinput=0xff;
  menu->selx=1;
  menu->sely=1;
  menu->del=_chalk_del;
  menu->ready=_chalk_ready;
  menu->update=_chalk_update;
  MENU->bits=bits;
  return menu;
}

#if 0 /*XXX move to client*/
/* bigpc_menu_hello.c
 * First thing the user sees.
 */
 
#include "../bigpc_internal.h"

/* Instance definition.
 */
 
struct bigpc_menu_hello {
  struct bigpc_menu hdr;
};

#define MENU ((struct bigpc_menu_hello*)menu)

/* Cleanup.
 */
 
static void _hello_del(struct bigpc_menu *menu) {
}

/* Ready.
 */
 
static int _hello_ready(struct bigpc_menu *menu) {
  return 0;
}

/* Update.
 */
 
static int _hello_update(struct bigpc_menu *menu,uint8_t new_input) {
  if (new_input&(FMN_INPUT_USE|FMN_INPUT_MENU)) {
    bigpc_menu_callback_any(menu);
    return 0;
  }
  return 1;
}

/* New.
 */
 
struct bigpc_menu *bigpc_menu_new_HELLO() {
  struct bigpc_menu *menu=calloc(1,sizeof(struct bigpc_menu_hello));
  if (!menu) return 0;
  menu->prompt=FMN_MENU_HELLO;
  menu->pvinput=0xff;
  menu->del=_hello_del;
  menu->ready=_hello_ready;
  menu->update=_hello_update;
  return menu;
}
#endif

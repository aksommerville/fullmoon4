#include "bigpc_internal.h"

/* Delete.
 */
 
void bigpc_menu_del(struct bigpc_menu *menu) {
  if (!menu) return;
  if (menu->del) menu->del(menu);
  if (menu->optionv) free(menu->optionv);
  free(menu);
}

/* Add option.
 */
 
int bigpc_menu_add_option(struct bigpc_menu *menu,int stringid,void (*cb)()) {
  if (!menu||menu->is_ready) return -1;
  if (menu->optionc>=menu->optiona) {
    int na=menu->optiona+8;
    if (na>INT_MAX/sizeof(struct bigpc_menu_option)) return -1;
    void *nv=realloc(menu->optionv,sizeof(struct bigpc_menu_option)*na);
    if (!nv) return -1;
    menu->optionv=nv;
    menu->optiona=na;
  }
  struct bigpc_menu_option *option=menu->optionv+menu->optionc++;
  option->stringid=stringid;
  option->cb=cb;
  return 0;
}

/* Ready.
 */

int bigpc_menu_ready(struct bigpc_menu *menu) {
  if (!menu) return -1;
  if (menu->is_ready) return -1;
  if (menu->ready) {
    if (menu->ready(menu)<0) return -1;
  }
  menu->is_ready=1;
  return 0;
}

/* Update.
 */
 
int bigpc_menu_update(struct bigpc_menu *menu) {
  if (!menu||!menu->update) return 0;
  menu->framec++;
  uint8_t new_input=0;
  if (menu->pvinput!=bigpc.input_state) {
    new_input=bigpc.input_state&~menu->pvinput;
    menu->pvinput=bigpc.input_state;
  }
  return menu->update(menu,new_input);
}

/* Call any callback.
 */
 
void bigpc_menu_callback_any(struct bigpc_menu *menu) {
  if (!menu) return;
  struct bigpc_menu_option *option=menu->optionv;
  int i=menu->optionc;
  for (;i-->0;option++) {
    if (!option->cb) continue;
    option->cb();
    return;
  }
}

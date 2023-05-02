#if 0 /*XXX move to client*/
/* bigpc_menu_prompt.c
 * Generic prompt-and-options menu.
 * We're not actually using this, at the time of writing.
 */
 
#include "../bigpc_internal.h"

/* Instance definition.
 */
 
struct bigpc_menu_prompt {
  struct bigpc_menu hdr;
};

#define MENU ((struct bigpc_menu_prompt*)menu)

/* Cleanup.
 */
 
static void _prompt_del(struct bigpc_menu *menu) {
}

/* Ready.
 */
 
static int _prompt_ready(struct bigpc_menu *menu) {
  return 0;
}

/* Update.
 */
 
static int _prompt_update(struct bigpc_menu *menu,uint8_t new_input) {
  if (new_input&(FMN_INPUT_USE|FMN_INPUT_MENU)) return 0;
  return 1;
}

/* New.
 */
 
struct bigpc_menu *bigpc_menu_new_prompt(int stringid) {
  struct bigpc_menu *menu=calloc(1,sizeof(struct bigpc_menu_prompt));
  if (!menu) return 0;
  menu->prompt=stringid;
  menu->pvinput=0xff;
  menu->del=_prompt_del;
  menu->ready=_prompt_ready;
  menu->update=_prompt_update;
  return menu;
}
#endif

/* bigpc_menu.h
 * Generic representation of all modal menus.
 * Same as the web platform, menus do not render themselves. Look in the render units for that.
 */
 
#ifndef BIGPC_MENU_H
#define BIGPC_MENU_H

struct bigpc_menu {
  int prompt; // FMN_MENU_* from fmn_platform.h, or a positive string id.
  struct bigpc_menu_option {
    int stringid;
    void (*cb)();
  } *optionv;
  int optionc,optiona;
  int is_ready;
  uint8_t pvinput;
  int selx,sely; // types can do what they like with these
  int framec; // updated by wrapper
  uint32_t extra[4];
  
  // Not doing a "type" object for these, all the hooks live on each instance.
  void (*del)(struct bigpc_menu *menu);
  int (*ready)(struct bigpc_menu *menu);
  // (update) receives a mask of newly-pressed buttons, on the assumption that menus generally don't care about "hold" or "off" events.
  int (*update)(struct bigpc_menu *menu,uint8_t new_input);
};

void bigpc_menu_del(struct bigpc_menu *menu);

struct bigpc_menu *bigpc_menu_new_prompt(int stringid);
struct bigpc_menu *bigpc_menu_new_PAUSE();
struct bigpc_menu *bigpc_menu_new_CHALK();
struct bigpc_menu *bigpc_menu_new_TREASURE();
struct bigpc_menu *bigpc_menu_new_VICTORY();
struct bigpc_menu *bigpc_menu_new_GAMEOVER();
struct bigpc_menu *bigpc_menu_new_HELLO();

/* After a successful 'new', add zero or more options, then 'ready'.
 */
int bigpc_menu_add_option(struct bigpc_menu *menu,int stringid,void (*cb)());
int bigpc_menu_ready(struct bigpc_menu *menu);

/* 0 to dismiss, 1 to proceed, <0 for real errors.
 */
int bigpc_menu_update(struct bigpc_menu *menu);

/* For type implementations, as they dismiss.
 * Most of our menus don't have a sense of "selected option", just trigger any registered callback and go.
 */
void bigpc_menu_callback_any(struct bigpc_menu *menu);

#endif

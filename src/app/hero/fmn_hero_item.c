#include "fmn_hero_internal.h"

/* Dispatch on item type.
 */
 
void fmn_hero_item_begin() {
  //TODO
  fmn_log("TODO %s selected=%d",__func__,fmn_global.selected_item);
}

void fmn_hero_item_end() {
  //TODO
  fmn_log("TODO %s active=%d",__func__,fmn_global.active_item);
  fmn_global.active_item=0;
}

void fmn_hero_item_update(float elapsed) {
  switch (fmn_global.active_item) {
    //TODO
  }
}

/* Button state changed.
 */
 
void fmn_hero_item_event(uint8_t buttonvalue) {
  if (buttonvalue) fmn_hero_item_begin();
  else fmn_hero_item_end();
}

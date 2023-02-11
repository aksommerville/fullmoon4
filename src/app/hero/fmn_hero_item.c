#include "fmn_hero_internal.h"

/* Bell.
 */
 
static void fmn_hero_bell_begin() {
  fmn_sound_effect(FMN_SFX_BELL);
  fmn_hero.bell_count=1;
  //TODO whatever bells do
}

static void fmn_hero_bell_update(float elapsed) {
  // try to match animation; assume 60 Hz video.
  uint8_t current=(uint8_t)((fmn_hero.item_active_time*60.0f)/32.0f);
  if (current!=fmn_hero.bell_count) {
    fmn_sound_effect(FMN_SFX_BELL);
    fmn_hero.bell_count=current;
  }
}

/* Corn.
 */
 
static void fmn_hero_corn_begin() {
  fmn_global.active_item=0;
  if (!fmn_global.itemqv[FMN_ITEM_CORN]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_log("ok corn it");
  fmn_global.itemqv[FMN_ITEM_CORN]--;
  //TODO corn sprite
  //TODO attract a bird
}

/* Seed. TODO I'm pretty sure Corn and Seed are the same thing... play it out and consider combining to one item
 */
 
static void fmn_hero_seed_begin() {
  fmn_global.active_item=0;
  if (!fmn_global.itemqv[FMN_ITEM_SEED]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_log("ok plant seed");
  fmn_global.itemqv[FMN_ITEM_SEED]--;
  //TODO plant seed
}

/* Coin.
 */
 
static void fmn_hero_coin_begin() {
  fmn_global.active_item=0;
  if (!fmn_global.itemqv[FMN_ITEM_COIN]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_log("ok throw coin");
  fmn_global.itemqv[FMN_ITEM_COIN]--;
  //TODO throw coin
}

/* Cheese.
 */
 
static void fmn_hero_cheese_begin() {
  fmn_global.active_item=0;
  if (!fmn_global.itemqv[FMN_ITEM_CHEESE]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_sound_effect(FMN_SFX_CHEESE);
  fmn_global.itemqv[FMN_ITEM_CHEESE]--;
  fmn_hero.cheesetime+=FMN_HERO_CHEESE_TIME;
}

/* Pitcher.
 */
 
static uint8_t XXX_pitcher_next=FMN_PITCHER_CONTENT_WATER;
 
static uint8_t fmn_hero_pitcher_pickup_from_environment() {
  uint8_t result=XXX_pitcher_next;//XXX very temporary
  switch (XXX_pitcher_next) {
    case FMN_PITCHER_CONTENT_WATER: XXX_pitcher_next=FMN_PITCHER_CONTENT_MILK; break;
    case FMN_PITCHER_CONTENT_MILK: XXX_pitcher_next=FMN_PITCHER_CONTENT_HONEY; break;
    case FMN_PITCHER_CONTENT_HONEY: XXX_pitcher_next=FMN_PITCHER_CONTENT_SAP; break;
    case FMN_PITCHER_CONTENT_SAP: XXX_pitcher_next=FMN_PITCHER_CONTENT_BLOOD; break;
    default: XXX_pitcher_next=FMN_PITCHER_CONTENT_WATER; break;
  }
  return result;
}
 
static void fmn_hero_pitcher_begin() {
  // Item stays "active", for visual purposes only.
  if (fmn_global.itemqv[FMN_ITEM_PITCHER]==FMN_PITCHER_CONTENT_EMPTY) {
    if (fmn_global.itemqv[FMN_ITEM_PITCHER]=fmn_hero_pitcher_pickup_from_environment()) {
      fmn_sound_effect(FMN_SFX_PITCHER_PICKUP);
      fmn_log("pitcher pickup %d",fmn_global.itemqv[FMN_ITEM_PITCHER]);
      //TODO visual feedback. show what we picked up
    } else {
      fmn_sound_effect(FMN_SFX_PITCHER_NO_PICKUP);
    }
  } else {
    fmn_sound_effect(FMN_SFX_PITCHER_POUR);
    fmn_log("pitcher pour %d",fmn_global.itemqv[FMN_ITEM_PITCHER]);
    //TODO locate target, do the thing....
    fmn_global.itemqv[FMN_ITEM_PITCHER]=FMN_PITCHER_CONTENT_EMPTY;
  }
}

/* Match.
 */
 
static void fmn_hero_match_begin() {
  if (!fmn_global.itemqv[FMN_ITEM_MATCH]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_global.illumination_time+=FMN_HERO_MATCH_ILLUMINATION_TIME;
  fmn_global.itemqv[FMN_ITEM_MATCH]--;
  fmn_sound_effect(FMN_SFX_MATCH);
}

/* Umbrella.
 * Doesn't take much; the umbrella is mostly a matter of state and not impulse actions.
 * But when we release it, we must re-examine facedir.
 */
 
static void fmn_hero_umbrella_end() {
  // If current facedir agrees with current motion, keep it.
  switch (fmn_global.facedir) {
    case FMN_DIR_W: if (fmn_hero.walkdx<0) return; break;
    case FMN_DIR_E: if (fmn_hero.walkdx>0) return; break;
    case FMN_DIR_N: if (fmn_hero.walkdy<0) return; break;
    case FMN_DIR_S: if (fmn_hero.walkdy>0) return; break;
  }
  // Clear this flag early; fmn_hero_motion_event will noop if it's set.
  fmn_global.active_item=0;
  // Simulate input according to walking directions. Horizontal wins ties.
       if (fmn_hero.walkdx<0) fmn_hero_motion_event(FMN_INPUT_LEFT,1);
  else if (fmn_hero.walkdx>0) fmn_hero_motion_event(FMN_INPUT_RIGHT,1);
  else if (fmn_hero.walkdy<0) fmn_hero_motion_event(FMN_INPUT_UP,1);
  else if (fmn_hero.walkdy>0) fmn_hero_motion_event(FMN_INPUT_DOWN,1);
}

/* Dispatch on item type.
 */
 
void fmn_hero_item_begin() {

  //XXX highly temporary: Give us the full set of items when you try to use FMN_ITEM_NONE
  if (fmn_global.selected_item==FMN_ITEM_NONE) {
    fmn_log("*** giving all items a sensible quantity ***");
    memset(fmn_global.itemv,1,sizeof(fmn_global.itemv));
    fmn_global.itemqv[FMN_ITEM_CORN]=20;
    fmn_global.itemqv[FMN_ITEM_SEED]=20;
    fmn_global.itemqv[FMN_ITEM_MATCH]=3;
    fmn_global.itemqv[FMN_ITEM_CHEESE]=20;
    fmn_global.itemqv[FMN_ITEM_COIN]=20;
    fmn_global.itemqv[FMN_ITEM_PITCHER]=FMN_PITCHER_CONTENT_MILK;
    return;
  }
  
  // We do allow unpossessed items to be selected. Must verify first that we actually have it.
  if ((fmn_global.selected_item>=FMN_ITEM_COUNT)||!fmn_global.itemv[fmn_global.selected_item]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  
  fmn_global.active_item=fmn_global.selected_item;
  fmn_hero.item_active_time=0;
  switch (fmn_global.active_item) {
    case FMN_ITEM_BELL: fmn_hero_bell_begin(); break;
    case FMN_ITEM_CORN: fmn_hero_corn_begin(); break;
    case FMN_ITEM_SEED: fmn_hero_seed_begin(); break;
    case FMN_ITEM_COIN: fmn_hero_coin_begin(); break;
    case FMN_ITEM_CHEESE: fmn_hero_cheese_begin(); break;
    case FMN_ITEM_PITCHER: fmn_hero_pitcher_begin(); break;
    case FMN_ITEM_MATCH: fmn_hero_match_begin(); break;
  }
}

void fmn_hero_item_end() {
  switch (fmn_global.active_item) {
    case FMN_ITEM_UMBRELLA: fmn_hero_umbrella_end(); break;
  }
  fmn_global.active_item=0;
}

void fmn_hero_item_update(float elapsed) {
  fmn_hero.item_active_time+=elapsed;
  switch (fmn_global.active_item) {
    case FMN_ITEM_BELL: fmn_hero_bell_update(elapsed); break;
  }
}

/* Button state changed.
 */
 
void fmn_hero_item_event(uint8_t buttonvalue) {
  if (buttonvalue) fmn_hero_item_begin();
  else fmn_hero_item_end();
}

#include "fmn_game.h"

/* Rebuild secrets.
 */
 
void fmn_secrets_refresh_for_map() {

  /* TODO I honestly don't know how we're going to implement this. Or exactly what needs implemented.
   * The idea is you examine the entire world, find the secret closest to this map, and record its relative position in (fmn_global.compass[xy]).
   * Right now, it feeds the compass. Will probably also impact the bird, and maybe other things. Can we have a "Spell of Secrets"?
   */
   
  // very temporarily, point to somewhere a little north of the current screen.
  fmn_global.compassx=FMN_COLC>>1;
  fmn_global.compassy=-(FMN_ROWC>>1);
}

/* Decode spell.
 */
 
uint8_t fmn_spell_eval(const uint8_t *v,uint8_t c) {
  //TODO spells
  if ((c==5)&&(v[0]==FMN_DIR_W)&&(v[1]==FMN_DIR_E)&&(v[2]==FMN_DIR_W)&&(v[3]==FMN_DIR_N)&&(v[4]==FMN_DIR_N)) return 1;
  return 0;
}

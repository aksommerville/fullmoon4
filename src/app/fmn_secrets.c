#include "fmn_game.h"
#include <string.h>

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

/* Decode spells and songs.
 */
 
static const struct fmn_spell {
  uint8_t spellid;
  uint8_t input_method; // 0=spell, 1=song
  uint8_t v[FMN_VIOLIN_SONG_LENGTH];
  uint8_t c;
} fmn_spellv[]={
#define N FMN_DIR_N
#define S FMN_DIR_S
#define W FMN_DIR_W
#define E FMN_DIR_E
#define _ 0
#define SPELL(id,...) {id,0,{__VA_ARGS__},sizeof((uint8_t[]){__VA_ARGS__})},
#define SONG(id,...) {id,1,{__VA_ARGS__},sizeof((uint8_t[]){__VA_ARGS__})},

  SPELL(1,W,E,W,N,N)
  SONG(2,S,S,W,W,E,E,N,N,E,E,W,W,S)

#undef SPELL
#undef SONG
#undef N
#undef S
#undef W
#undef E
#undef _
};
 
uint8_t fmn_spell_eval(const uint8_t *v,uint8_t c) {
  const struct fmn_spell *spell=fmn_spellv;
  int i=sizeof(fmn_spellv)/sizeof(struct fmn_spell);
  for (;i-->0;spell++) {
    if (spell->input_method!=0) continue;
    if (spell->c!=c) continue;
    if (!memcmp(spell->v,v,c)) return spell->spellid;
  }
  return 0;
}

/* Decode song.
 */
 
uint8_t fmn_song_eval(const uint8_t *v,uint8_t c) {
  const struct fmn_spell *spell=fmn_spellv;
  int i=sizeof(fmn_spellv)/sizeof(struct fmn_spell);
  for (;i-->0;spell++) {
    if (spell->input_method!=1) continue;
    if (spell->c>c) continue;
    if (!memcmp(spell->v,v+c-spell->c,spell->c)) return spell->spellid;
  }
  return 0;
}

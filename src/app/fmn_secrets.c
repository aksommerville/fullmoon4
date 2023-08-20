#include "fmn_game.h"

/* Rebuild secrets.
 */
 
void fmn_secrets_refresh_for_map() {
  fmn_global.compassx=0;
  fmn_global.compassy=0;
  
  /* If we are a pumpkin, the only thing we will point to is the nearest depumpkinning transmogrifier.
   * If there isn't one in range, we go neutral -- even if there's some other secret nearby.
   */
  if (fmn_global.transmogrification==1) {
    const uint8_t v[]={0x44,0,0x41}; // transmogrify (anywhere) from pumpkin
    fmn_find_map_command(&fmn_global.compassx,0x05,v);
    return;
  }
  
  /* Buried door or treasure on this map is compassable if not discovered yet.
   * Prefer doors. Show buried treasure only if it has a gsbit and there are no buried doors.
   */
  const struct fmn_door *buried_treasure=0;
  const struct fmn_door *door=fmn_global.doorv;
  uint8_t i=fmn_global.doorc;
  for (;i-->0;door++) {
    if (!door->mapid&&(door->dstx==0x30)) { // buried_treasure
      if (buried_treasure) {
        // already have a buried treasure, ignore this one
      } else if (door->extra&&fmn_gs_get_bit(door->extra)) {
        // already have it (per gsbit), no real treasure here
      } else if (door->dsty>=FMN_ITEM_COUNT) {
        // invalid buried_treasure!
      } else if (fmn_global.itemv[door->dsty]&&!fmn_item_default_quantities[door->dsty]) {
        // already have it (non-quantity item), no real treasure here
      } else {
        buried_treasure=door;
      }
    } else if (door->mapid&&door->extra) { // buried_door
      if (!fmn_gs_get_bit(door->extra)) {
        fmn_global.compassx=door->x;
        fmn_global.compassy=door->y;
        return;
      }
    }
  }
  if (buried_treasure) {
    fmn_global.compassx=buried_treasure->x;
    fmn_global.compassy=buried_treasure->y;
    return;
  }
  
  /* TODO Decide what else constitutes a secret, implement some of those, and return here.
   */
}

/* Get the guidance direction, eg for crows.
 * This is very aware of the layout and design, we need to code it with the maps in mind.
 * Current logic is for the Demo. Will need to revisit for full version.
 * TODO: Can we encode these decisions in the data set somehow?
 */
 
uint8_t fmn_secrets_get_guide_dir_full() {
  uint8_t dir;
  //fmn_log("%s...",__func__);
  #define RESPOND(comment) { /*fmn_log("...%s 0x%04x",comment,dir);*/ return dir; }
  #define SIMPLE_ITEM(tag) if (!fmn_global.itemv[FMN_ITEM_##tag]) { if (dir=fmn_find_direction_to_item(FMN_ITEM_##tag)) RESPOND(#tag) }
  
  SIMPLE_ITEM(FEATHER) // no prereqs, intended as the very first thing.
  SIMPLE_ITEM(COMPASS) // only FEATHER required, and it's prereq to the swamp.
  SIMPLE_ITEM(WAND) // requires FEATHER,COMPASS
  SIMPLE_ITEM(UMBRELLA) // requires FEATHER,COMPASS. arguably required for SHOVEL (to learn the Spell of Slow Motion).
  SIMPLE_ITEM(SHOVEL) // requires WAND and Slow Motion.
  SIMPLE_ITEM(PITCHER) // no prereqs, but also not useful before you have SHOVEL.
  SIMPLE_ITEM(VIOLIN) // requires FEATHER probably, and it's a fair bit of travel so WAND is recommended too. Acquire before we start recommending farming.
  
  /* Chalk should come next. Don't recommend it if she's no COIN or MATCH.
   * COIN or MATCH missing, lead her to the Department of Agriculture.
   * ...on second thought, if it's COIN that's missing, send her to the Rabbit (beehive there).
   */
  if (!fmn_global.itemv[FMN_ITEM_MATCH]) {
    if (dir=fmn_find_direction_to_map_reference(1)) RESPOND("farmer(initial)")
  }
  if (!fmn_global.itemv[FMN_ITEM_COIN]) {
    if (dir=fmn_find_direction_to_map_reference(3)) RESPOND("rabbit")
  }
  SIMPLE_ITEM(HAT) // with PITCHER and SHOVEL, you can get COIN and buy this. needed for CHALK.
  SIMPLE_ITEM(CHALK)
  
  /* After chalk, SNOWGLOBE and BROOM become available.
   * SNOWGLOBE is very close to CHALK geographically, so recommend that first.
   */
  SIMPLE_ITEM(SNOWGLOBE)
  SIMPLE_ITEM(BROOM)
  
  SIMPLE_ITEM(BELL) // requires BROOM. Pretty much always the last item.
  
  // Not recommending SEED or CHEESE. SEED you would get along the way (at the farmer step). CHEESE is never required.
  
  /* If the castle's basement door is still locked (gs:ww_fire3), guide to the Church.
   */
  if (!fmn_gs_get_bit(29)) {
    if (dir=fmn_find_direction_to_map_reference(2)) RESPOND("church")
  }
  
  /* If she hasn't reached the werewolf yet (gs:ww_fire_3f as a proxy), ensure some quantity of COIN.
   * You need at least 2 coins, right at the end of the 3rd floor.
   */
  if (!fmn_gs_get_bit(63)&&(fmn_global.itemqv[FMN_ITEM_COIN]<2)) {
    if (dir=fmn_find_direction_to_map_reference(1)) RESPOND("farmer(coin)")
  }
  
  // OK! Guide to the castle.
  if (dir=fmn_find_direction_to_teleport(FMN_SPELLID_TELE4)) RESPOND("werewolf")
  
  
  // Welp that's all I got. Kaaaa, thanks for the corn.
  //fmn_log("...no response");
  return 0;
  #undef RESPOND
  #undef SIMPLE_ITEM
}

static uint8_t fmn_secrets_get_guide_dir_demo() {
  uint8_t dir;
  //fmn_log("%s...",__func__);
  #define RESPOND(comment) { /*fmn_log("...%s 0x%04x",comment,dir);*/ return dir; }
  #define SIMPLE_ITEM(tag) if (!fmn_global.itemv[FMN_ITEM_##tag]) { if (dir=fmn_find_direction_to_item(FMN_ITEM_##tag)) RESPOND(#tag) }
  
  // Feather required, and it has no prerequisites.
  SIMPLE_ITEM(FEATHER)
  
  /* Broom required, but you need feather or compass first.
   * We've already checked feather, so only check compass.
   */
  if (!fmn_global.itemv[FMN_ITEM_BROOM]) {
    if (!fmn_global.itemv[FMN_ITEM_FEATHER]&&!fmn_global.itemv[FMN_ITEM_COMPASS]) { // COMPASS required only if you don't have BROOM or FEATHER.
      if (dir=fmn_find_direction_to_item(FMN_ITEM_COMPASS)) RESPOND("compass")
    } else {
      if (dir=fmn_find_direction_to_item(FMN_ITEM_BROOM)) RESPOND("broom")
    }
  }
  
  /* Wand required.
   * Technically you only need the feather to get it.
   * Well, super technically, it's possible to get with nothing.
   * But that would mean waiting for raccoons to randomly step on treadles.
   * The feather-only solution, you need to understand raccoon mind control, and I don't count on users to figure that out.
   * So require VIOLIN first.
   * VIOLIN can be had for free.
   */
  if (!fmn_global.itemv[FMN_ITEM_WAND]) {
    if (!fmn_global.itemv[FMN_ITEM_VIOLIN]) {
      if (dir=fmn_find_direction_to_item(FMN_ITEM_VIOLIN)) RESPOND("violin")
    } else {
      if (dir=fmn_find_direction_to_item(FMN_ITEM_WAND)) RESPOND("wand")
    }
  }
  
  /* Umbrella required.
   * You need one of:
   *  - Feather, and knowledge of lambda blocks.
   *  - Wand, and the Spell of Opening.
   *  - Wand, and the Spell of Wind.
   * ...i'm only a bird, I don't get learning spells.
   */
  SIMPLE_ITEM(UMBRELLA)
  
  /* TODO Require Spell of Opening.
   * For now, use either of gsbit (3,4) as a proxy for knowing the spell.
   * Those are (moonsong,mns_switch); you've likely been in her basement if either of those is true.
   */
  if (!fmn_gs_get_bit(3)&&!fmn_gs_get_bit(4)) {
    if (dir=fmn_find_direction_to_map(30)) RESPOND("opening")
  }
  
  /* If the werewolf is alive, that's where you're going.
   */
  if (!fmn_global.werewolf_dead) {
    // Would prefer to use fmn_find_direction_to_teleport(FMN_SPELLID_TELE4), but in the Demo there is no castle teleport point.
    if (dir=fmn_find_direction_to_map(11)) RESPOND("werewolf")
  }
  
  /* Werewolf is dead? Hmm ok. Let's figure they're going for 100% item collection.
   * NB We are currently not allowing play to proceed after killing the werewolf, and werewolf_dead will always save false.
   */
  uint8_t itemid=2; for (;itemid<FMN_ITEM_COUNT;itemid++) {
    if (!fmn_global.itemv[itemid]) {
      if (dir=fmn_find_direction_to_item(itemid)) RESPOND("some item (impossible)")
    }
  }
  
  // Welp that's all I got. Kaaaa, thanks for the corn.
  //fmn_log("...no response");
  return 0;
  #undef RESPOND
  #undef SIMPLE_ITEM
}

uint8_t fmn_secrets_get_guide_dir() {
  if (fmn_is_demo()) return fmn_secrets_get_guide_dir_demo();
  return fmn_secrets_get_guide_dir_full();
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
#define SPELL(name,...) {FMN_SPELLID_##name,0,{__VA_ARGS__},sizeof((uint8_t[]){__VA_ARGS__})},
#define SONG(name,...) {FMN_SPELLID_##name,1,{__VA_ARGS__},sizeof((uint8_t[]){__VA_ARGS__})},

  SONG(BLOOM,      W,E,W,_,N,N,S)
  SONG(REVEILLE,   S,S,W,E,N,_,N)
  SONG(LULLABYE,   W,_,S,_,W,S,E)
  
  SPELL(RAIN,N,W,S,N,E,S)
  SPELL(WIND_W,E,N,W,S,E,W,S,W)
  SPELL(WIND_E,E,N,W,S,E,W,S,E)
  SPELL(WIND_N,E,N,W,S,E,W,S,N)
  SPELL(WIND_S,E,N,W,S,E,W,S,S)
  SPELL(SLOWMO,E,W,W,E,W,W,S,W)
  SPELL(OPEN,W,E,W,N,N)
  SPELL(INVISIBLE,W,N,S,E,W,W,E,E)
  SPELL(PUMPKIN,N,S,N,S,W,W,E)
  
  SPELL(HOME,S,S,S)
  SPELL(TELE1,W,S,E,W,S,E) /* beach (demo:Church) */
  SPELL(TELE2,E,W,E,S) /* swamp */
  SPELL(TELE3,W,N,E,W,N,E) /* mountains */
  SPELL(TELE4,W,E,W,E,S,N,N) /* castle */
  SPELL(TELE5,E,E,W,E,E) /* desert */
  SPELL(TELE6,N,N,S,N,N) /* steppe */

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

/* Get song or spell verbatim.
 */
 
uint8_t fmn_spell_get(uint8_t *dst,uint8_t dsta,uint8_t spellid) {
  const struct fmn_spell *spell=fmn_spellv;
  int i=sizeof(fmn_spellv)/sizeof(struct fmn_spell);
  for (;i-->0;spell++) {
    if (spell->spellid!=spellid) continue;
    if (spell->c<=dsta) memcpy(dst,spell->v,spell->c);
    return spell->c;
  }
  return 0;
}

/* Read sketches as text.
 */

extern const int fmn_chalk_glyphs[];
 
static int8_t fmn_sketch_cmp(const struct fmn_sketch *a,const struct fmn_sketch *b) {
  if (a->y<b->y) return -1;
  if (a->y>b->y) return 1;
  if (a->x<b->x) return -1;
  if (a->x>b->x) return 1;
  return 0;
}
 
static void fmn_sort_sketches(struct fmn_sketch *v,uint8_t c) {
  if (c<1) return;
  uint8_t lo=0,hi=c-1;
  int8_t d=1;
  while (lo<hi) {
    uint8_t first,last,i,done=1;
    if (d<0) { first=hi; last=lo; } else { first=lo; last=hi; }
    struct fmn_sketch *p=v+first;
    for (i=first;i!=last;i+=d,p+=d) {
      if (fmn_sketch_cmp(p,p+d)==d) {
        done=0;
        struct fmn_sketch tmp=*p;
        *p=p[d];
        p[d]=tmp;
      }
    }
    if (done) return;
    if (d<0) {
      lo++;
      d=1;
    } else {
      hi--;
      d=-1;
    }
  }
}

static char fmn_char_from_sketch(uint32_t bits) {
  if (!bits) return '?';
  const int *q=fmn_chalk_glyphs;
  for (;*q;q+=2) if (q[1]==bits) return q[0];
  return '?';
}
 
int8_t fmn_for_each_sketch_word(int8_t (*cb)(const char *src,uint8_t srcc,void *userdata),void *userdata) {
  if (!fmn_global.sketchc) return 0;

  // Copy sketches, then sort by position.
  struct fmn_sketch sketchv[FMN_SKETCH_LIMIT];
  memcpy(sketchv,fmn_global.sketchv,sizeof(struct fmn_sketch)*fmn_global.sketchc);
  fmn_sort_sketches(sketchv,fmn_global.sketchc);
  
  // Now connecting the strings is trivial.
  const struct fmn_sketch *sketch=sketchv;
  uint8_t c=fmn_global.sketchc;
  while (c>0) {
    char word[FMN_SKETCH_LIMIT+1];
    uint8_t wordc=0;
    word[wordc++]=fmn_char_from_sketch(sketch->bits);
    uint8_t nextx=sketch->x+1;
    uint8_t nexty=sketch->y;
    sketch++;
    c--;
    while (c&&(sketch->x==nextx)&&(sketch->y==nexty)) {
      word[wordc++]=fmn_char_from_sketch(sketch->bits);
      nextx++;
      sketch++;
      c--;
    }
    word[wordc]=0;
    int8_t err=cb(word,wordc,userdata);
    if (err) return err;
  }
  
  return 0;
}

#include "fmn_game.h"
#include <string.h>

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
  
  /* TODO Decide what constitutes a secret, implement some of those, and return here.
   */
}

/* Get the guidance direction, eg for crows.
 */
 
uint8_t fmn_secrets_get_guide_dir() {
  // This is not right, because the crow sees far, but not near.
  // We don't care about secrets on this map, but we do want to lead to far-away things.
  // Also this isn't really for "secrets"... It's more like "what is the next step, what should I be doing?"
  fmn_secrets_refresh_for_map();
  if (fmn_global.compassx||fmn_global.compassy) {
    if ((fmn_global.compassx<0)&&(fmn_global.compassx<fmn_global.compassy)) return FMN_DIR_W;
    if ((fmn_global.compassy<0)&&(fmn_global.compassy<fmn_global.compassx)) return FMN_DIR_N;
    if ((fmn_global.compassx>0)&&(fmn_global.compassx>fmn_global.compassy)) return FMN_DIR_E;
    if ((fmn_global.compassy>0)&&(fmn_global.compassy>fmn_global.compassx)) return FMN_DIR_S;
  }
  return FMN_DIR_S;
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

  SONG(BLOOM,W,E,W,_,W,E,W,_,N,_,N,_,S)
  SPELL(RAIN,N,S,S,S)
  SPELL(WIND_W,E,N,W,S,E,W,S,W)
  SPELL(WIND_E,E,N,W,S,E,W,S,E)
  SPELL(WIND_N,E,N,W,S,E,W,S,N)
  SPELL(WIND_S,E,N,W,S,E,W,S,S)
  SPELL(SLOWMO,E,W,W,E,W,W,S,W)
  SPELL(HOME,E,S,N,W,S,N)
  SPELL(TELE1,S,W,E,N,W,E)
  SPELL(TELE2,W,N,S,E,N,S)
  SPELL(TELE3,N,E,W,S,E,W)
  SPELL(TELE4,W,E,W,E,S,N,N)
  SPELL(OPEN,W,E,W,N,N)
  SPELL(INVISIBLE,W,N,S,E,W,W,E,E)
  SONG(REVEILLE,S,_,N,N,E,_,N,_,N,_,_,E,N)
  SONG(LULLABYE,W,_,_,_,S,_,_,_,W,_,S,_,E)
  SONG(HITHER,W,W,E,W,N,_,_,_,W,W,E,W,N,_,N)
  SONG(THITHER,N,E,W,S,E,E,W,S)
  SONG(REVELATIONS,S,_,W,_,S,W,E,W,S,_,N,_,N)
  SPELL(PUMPKIN,N,S,N,S,W,W,E)

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

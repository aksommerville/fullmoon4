#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define RABBIT_BEAT_TIME 0.250f

static const uint8_t rabbit_song[]={
  FMN_DIR_W,
  FMN_DIR_E,
  FMN_DIR_W,
  0,
  FMN_DIR_W,
  FMN_DIR_E,
  FMN_DIR_W,
  0,
  FMN_DIR_N,
  0,
  FMN_DIR_N,
  0,
  FMN_DIR_S,
  0,
  0,
  0,
};

#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define songp sprite->bv[2]
#define applaud sprite->bv[3]
#define clock sprite->fv[0]
#define songclock sprite->fv[1]

/* Init.
 */
 
static void _rabbit_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
}

/* Play a note.
 */
 
static void rabbit_play_note(struct fmn_sprite *sprite,uint8_t note) {
  if (note) {
    uint8_t tileid=tileid0+5;
    uint8_t xform=0; // the natural state points N
    switch (note) {
      case FMN_DIR_W: xform=FMN_XFORM_SWAP; break;
      case FMN_DIR_E: xform=FMN_XFORM_SWAP|FMN_XFORM_YREV; break;
      case FMN_DIR_S: xform=FMN_XFORM_YREV; break;
    }
    struct fmn_sprite *toast=fmn_sprite_generate_toast(sprite->x,sprite->y-0.5f,sprite->imageid,tileid,xform);
    if (toast) {
      toast->fv[2]=-2.0f; // dy
    }
  }
}

static void rabbit_play_decoration(struct fmn_sprite *sprite,uint8_t right) {
  struct fmn_sprite *toast=fmn_sprite_generate_toast(sprite->x,sprite->y-0.5f,sprite->imageid,tileid0+6,0);
  if (!toast) return;
  toast->fv[0]=0.75f;
  if (right) {
    toast->x+=0.25f;
    toast->fv[1]=0.3f;
    toast->fv[2]=-0.8f;
  } else {
    toast->x-=0.25f;
    toast->fv[1]=-0.3f;
    toast->fv[2]=-0.8f;
  }
}

/* Update.
 */
 
static void _rabbit_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    sprite->tileid=tileid0+4;
    clock=0.0f;
    songclock=0.0f;
    songp=0;
    return;
  }
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (herox<sprite->x) sprite->xform=0;
  else sprite->xform=FMN_XFORM_XREV;
  if (applaud) {
    clock+=elapsed;
    sprite->tileid=tileid0+0x55+(((int)(clock*8.0f))&1);
    return;
  }
  clock+=elapsed;
  sprite->tileid=tileid0+((int)(clock*4.0f))%4;
  songclock+=elapsed;
  if (songclock>=RABBIT_BEAT_TIME) {
    songclock-=RABBIT_BEAT_TIME;
    if (songp>=sizeof(rabbit_song)) songp=0;
    rabbit_play_note(sprite,rabbit_song[songp]);
    if (!(songp&3)) rabbit_play_decoration(sprite,songp&4);
    songp++;
  }
}

/* Interact.
 */
 
static int16_t _rabbit_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: if (!sleeping) { sleeping=1; fmn_sprite_generate_zzz(sprite); } break;
        case FMN_SPELLID_REVEILLE: sleeping=0; break;
        case FMN_SPELLID_BLOOM: applaud=1; break;
        case FMN_SPELLID_PUMPKIN: fmn_sprite_pumpkinize(sprite); break;
      } break;
    case FMN_ITEM_BELL: sleeping=0; break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_rabbit={
  .init=_rabbit_init,
  .update=_rabbit_update,
  .interact=_rabbit_interact,
};

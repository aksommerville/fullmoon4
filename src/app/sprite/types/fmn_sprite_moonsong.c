/* fmn_sprite_moonsong.c
 * Moon Song is a rival of Dot Vine's.
 * (It's a perfectly normal name, she's Korean).
 * Works kind of like the Farmer, in that this NPC does things that the hero can also do.
 * But unlike the Farmer, Moon Song does her thing artificially.
 * Basically, it's a long animation of casting a spell, then "cast" it manually by tweaking global state.
 */

#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define MOONSONG_OUVERTURE_TIME 3.0f
#define MOONSONG_STROKE_TIME 0.5f
#define MOONSONG_BETWEEN_TIME 0.25f

// Must match FMN_SPELLID_PUMPKIN, defined in src/app/fmn_secrets.c.
static const uint8_t moonsong_spell[]={
  FMN_DIR_N,FMN_DIR_S,FMN_DIR_N,FMN_DIR_S,FMN_DIR_W,FMN_DIR_W,FMN_DIR_E,
};

#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define pumpkinned sprite->bv[2]
#define spellp sprite->bv[3]
#define clock sprite->fv[0]
#define gsbit sprite->argv[0] /* nonzero=pumpkinned */

/* Init.
 */
 
static void _moonsong_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  if (gsbit&&fmn_gs_get_bit(gsbit)) {
    pumpkinned=1;
    sprite->invmass=128;
    sprite->tileid=tileid0+10;
  }
}

/* Cast my spell, turn the hero into a pumpkin.
 */
 
static void moonsong_cast_spell(struct fmn_sprite *sprite) {
  switch (fmn_global.transmogrification) {
    case 0: {
        float herox,heroy;
        fmn_hero_get_position(&herox,&heroy);
        fmn_global.transmogrification=1;
        fmn_sprite_generate_soulballs(herox,heroy,7);
        fmn_sound_effect(FMN_SFX_PUMPKIN);
        fmn_hero_cancel_item();
      } break;
  }
}

/* Tile from spell unit.
 */
 
static uint8_t moonsong_tile_offset_for_stroke(uint8_t dir) {
  switch (dir) {
    case FMN_DIR_E: return 5;
    case FMN_DIR_W: return 6;
    case FMN_DIR_N: return 7;
    case FMN_DIR_S: return 8;
  }
  return 4;
}

/* Update.
 */
 
static void _moonsong_update(struct fmn_sprite *sprite,float elapsed) {
  clock+=elapsed;
  if (pumpkinned) {
    if (clock>=4.0f) clock=0.0f;
    sprite->tileid=tileid0+((clock>=3.8f)?11:10);
    return;
  }
  if (sleeping) {
    sprite->tileid=tileid0+9;
    clock=0.0f;
    return;
  }
  if (clock<MOONSONG_OUVERTURE_TIME) {
    sprite->tileid=tileid0;
    spellp=0;
    if (fmn_global.transmogrification) clock=0.0f; // hero is already a pumpkin, do nothing.
    return;
  }
  uint8_t p=(uint8_t)((clock-MOONSONG_OUVERTURE_TIME)/(MOONSONG_STROKE_TIME+MOONSONG_BETWEEN_TIME));
  if (p>=sizeof(moonsong_spell)) {
    sprite->tileid=tileid0;
    spellp=0;
    clock=0.0f;
    moonsong_cast_spell(sprite);
    return;
  }
  spellp=p;
  float stroketime=MOONSONG_OUVERTURE_TIME+p*(MOONSONG_STROKE_TIME+MOONSONG_BETWEEN_TIME);
  float modtime=clock-stroketime;
  if (modtime<MOONSONG_STROKE_TIME) {
    sprite->tileid=tileid0+moonsong_tile_offset_for_stroke(moonsong_spell[spellp]);
  } else {
    sprite->tileid=tileid0+4;
  }
}

/* Toggle pumpkin state.
 */
 
static void moonsong_toggle_pumpkin(struct fmn_sprite *sprite) {
  if (sleeping) return; // only works while awake
  if (pumpkinned) {
    pumpkinned=0;
    fmn_sound_effect(FMN_SFX_UNPUMPKIN);
    fmn_sprite_generate_soulballs(sprite->x,sprite->y,3);
    clock=0.0f;
    sprite->invmass=0;
  } else {
    pumpkinned=1;
    fmn_sound_effect(FMN_SFX_PUMPKIN);
    fmn_sprite_generate_soulballs(sprite->x,sprite->y,3);
    sprite->invmass=128;
  }
  if (gsbit) fmn_gs_set_bit(gsbit,pumpkinned);
}

/* Interact.
 */
 
static int16_t _moonsong_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_PUMPKIN: moonsong_toggle_pumpkin(sprite); break;
        case FMN_SPELLID_LULLABYE: if (!sleeping&&!pumpkinned) { sleeping=1; fmn_sprite_generate_zzz(sprite); } break;
        case FMN_SPELLID_REVEILLE: if (!pumpkinned) sleeping=0; break;
      } break;
    case FMN_ITEM_BELL: if (!pumpkinned) sleeping=0; break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_moonsong={
  .init=_moonsong_init,
  .update=_moonsong_update,
  .interact=_moonsong_interact,
};

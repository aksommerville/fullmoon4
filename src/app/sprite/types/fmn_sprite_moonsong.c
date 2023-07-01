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
#define MOONSONG_WALK_SPEED 2.0f

/* emergency_exit: Hacky workaround for a trap possible in the demo.
 * Our setup is you have to turn Moon Song into a pumpkin and push her onto a treadle to open a gate and get into her basement.
 * If you empumpkin each other at the same time -- easy to do -- and proceed, she'll respawn off the treadle and you're trapped.
 * I guess that could also happen if you get to the other side and then depumpkin her. Why would you do that?
 * SOLUTION: Upon startup, if the hero is a pumpkin and on the other side of the gate, walk to the treadle.
 * That's out of character of course, the real Moon Song would keep you trapped until you rot.
 * We have to delay until the first update, to ensure that the treadle and gate both exist and hero is in the right place.
 * emergency_exit_state:
 *   0: No need.
 *   1: Waiting for first update.
 *   2: Walk to treadle.
 */

// Must match FMN_SPELLID_PUMPKIN, defined in src/app/fmn_secrets.c.
static const uint8_t moonsong_spell[]={
  FMN_DIR_N,FMN_DIR_S,FMN_DIR_N,FMN_DIR_S,FMN_DIR_W,FMN_DIR_W,FMN_DIR_E,
};

#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define pumpkinned sprite->bv[2]
#define spellp sprite->bv[3]
#define emergency_exit_state sprite->bv[4]
#define clock sprite->fv[0]
#define emergency_exit_dstx sprite->fv[1]
#define emergency_exit_dsty sprite->fv[2]
#define gsbit sprite->argv[0] /* nonzero=pumpkinned */

/* Init.
 */
 
static void _moonsong_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  emergency_exit_state=1;
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
        fmn_sprite_generate_soulballs(herox,heroy,7,0);
        fmn_sound_effect(FMN_SFX_PUMPKIN);
        fmn_hero_cancel_item();
        fmn_secrets_refresh_for_map();
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

/* Initial check for emergency_exit.
 */
 
struct moonsong_emergency_exit_players {
  struct fmn_sprite *sprite;
  struct fmn_sprite *treadle;
  struct fmn_sprite *gate;
};

static int moonsong_assess_emergency_exit_players(struct fmn_sprite *q,void *userdata) {
  struct moonsong_emergency_exit_players *players=userdata;
  // Assume that any treadle and gate are ok; there shouldn't be more than one of each.
  if (q->controller==FMN_SPRCTL_treadle) players->treadle=q;
  else if (q->controller==FMN_SPRCTL_gate) players->gate=q;
  else return 0;
  if (players->treadle&&players->gate) return 1;
  return 0;
}
 
static uint8_t moonsong_assess_emergency_exit(struct fmn_sprite *sprite) {

  // Only necessary if the hero is a pumpkin.
  if (fmn_global.transmogrification!=1) return 0;
  
  // Must have a treadle and gate.
  struct moonsong_emergency_exit_players players={
    .sprite=sprite,
  };
  if (!fmn_sprites_for_each(moonsong_assess_emergency_exit_players,&players)) return 0;
  
  // Hero must be on the other side of the gate horizontally.
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (sprite->x<players.gate->x) {
    if (herox<players.gate->x) return 0;
  } else {
    if (herox>players.gate->x) return 0;
  }
  
  // OK we are going to do the emergency exit thing.
  emergency_exit_dstx=players.treadle->x;
  emergency_exit_dsty=players.treadle->y;
  return 1;
}

/* Check for emergency exit behavior.
 * If we do something and return nonzero, all other update behavior is suspended.
 * (NB even if somehow we are sleeping, we do this).
 */
 
static uint8_t moonsong_update_emergency_exit(struct fmn_sprite *sprite,float elapsed) {
  switch (emergency_exit_state) {
    case 0: return 0;
    case 1: if (moonsong_assess_emergency_exit(sprite)) {
        emergency_exit_state=2;
        return 1;
      } else {
        emergency_exit_state=0;
        return 0;
      }
    case 2: {
        uint8_t xok=0,yok=0;
        if (sprite->x<emergency_exit_dstx) {
          if ((sprite->x+=MOONSONG_WALK_SPEED*elapsed)>=emergency_exit_dstx) sprite->x=emergency_exit_dstx;
        } else if (sprite->x>emergency_exit_dstx) {
          if ((sprite->x-=MOONSONG_WALK_SPEED*elapsed)<=emergency_exit_dstx) sprite->x=emergency_exit_dstx;
        } else xok=1;
        if (sprite->y<emergency_exit_dsty) {
          if ((sprite->y+=MOONSONG_WALK_SPEED*elapsed)>=emergency_exit_dsty) sprite->y=emergency_exit_dsty;
        } else if (sprite->y>emergency_exit_dsty) {
          if ((sprite->y-=MOONSONG_WALK_SPEED*elapsed)<=emergency_exit_dsty) sprite->y=emergency_exit_dsty;
        } else yok=1;
        if (xok&&yok) {
          emergency_exit_state=0;
          return 0;
        }
      } return 1;
  }
  return 0;
}

/* Update.
 */
 
static void _moonsong_update(struct fmn_sprite *sprite,float elapsed) {
  clock+=elapsed;
  if (moonsong_update_emergency_exit(sprite,elapsed)) return;
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
    fmn_sprite_generate_soulballs(sprite->x,sprite->y,3,0);
    clock=0.0f;
    sprite->invmass=0;
  } else {
    pumpkinned=1;
    fmn_sound_effect(FMN_SFX_PUMPKIN);
    fmn_sprite_generate_soulballs(sprite->x,sprite->y,3,0);
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

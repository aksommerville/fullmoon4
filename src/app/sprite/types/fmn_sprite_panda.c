/* fmn_sprite_panda.c
 * Large stationary sprite that monitors the hero's movement.
 * If she spends too long still or moving, we become enraged and spit fireballs at her.
 */

#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define rating sprite->fv[0] /* -1..1; <0=too still, >0=too movey */
#define ragetime sprite->fv[1]
#define fireangle sprite->fv[2] /* direction of next fireball, when enraged */
#define fireclock sprite->fv[3] /* counts down to next fireball */

#define PANDA_RATING_RATE (1.0f/2.0f) /* how many seconds to go from neutral to violation? */
#define PANDA_RATING_UNRATE (1.0f/0.5f)
#define PANDA_RAGE_TIME 0.7f
#define PANDA_RAGE_FIREBALL_COUNT 20
#define PANDA_RAGE_FIREBALL_PERIMETER 2.5f /* radians, centered on hero */
#define PANDA_FIREBALL_SPEED 10.0f

/* Generate one fireball and reset state in preparation for the next.
 */
 
static void panda_spit(struct fmn_sprite *sprite) {
  
  struct fmn_sprite *missile=fmn_sprite_generate_noparam(FMN_SPRCTL_missile,sprite->x+0.5f,sprite->y+0.25f);
  if (missile) {
    missile->imageid=sprite->imageid;
    missile->tileid=tileid0+0xf4;
    missile->style=FMN_SPRITE_STYLE_TWOFRAME;
    missile->fv[1]=PANDA_FIREBALL_SPEED*cosf(fireangle);
    missile->fv[2]=PANDA_FIREBALL_SPEED*sinf(fireangle);
    missile->bv[0]=1; // ready
  }
  
  fireclock+=PANDA_RAGE_TIME/PANDA_RAGE_FIREBALL_COUNT;
  fireangle+=PANDA_RAGE_FIREBALL_PERIMETER/PANDA_RAGE_FIREBALL_COUNT;
}

/* Become enraged and begin generating fireballs.
 */
 
static void panda_rage(struct fmn_sprite *sprite) {
  ragetime=PANDA_RAGE_TIME;
  fmn_sound_effect(FMN_SFX_PANDA_CRY);
  
  float basex=sprite->x+0.5f;
  float basey=sprite->y+0.25f;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  fireangle=atan2f(heroy-basey,herox-basex);
  fireangle-=PANDA_RAGE_FIREBALL_PERIMETER*0.5f;
  fireclock=0.0f;
  panda_spit(sprite);
}

/* Update.
 */
 
static void _panda_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    rating=0.0f;
    ragetime=0.0f;
    return;
  }
  if (ragetime>0.0f) {
    if (fireclock>0.0f) {
      fireclock-=elapsed;
      while (fireclock<=0.0f) panda_spit(sprite);
    }
    if ((ragetime-=elapsed)<=0.0f) {
      ragetime=0.0f;
    } else if (rating<0.0f) {
      if ((rating+=PANDA_RATING_UNRATE*elapsed)>=0.0f) {
        rating=0.0f;
      }
    } else if (rating>0.0f) {
      if ((rating-=PANDA_RATING_UNRATE*elapsed)<=0.0f) {
        rating=0.0f;
      }
    }
  } else if (fmn_global.walking) {
    rating+=PANDA_RATING_RATE*elapsed;
    if (rating>=1.0f) {
      rating=1.0f;
      panda_rage(sprite);
    }
  } else {
    rating-=PANDA_RATING_RATE*elapsed;
    if (rating<=-1.0f) {
      rating=-1.0f;
      panda_rage(sprite);
    }
  }
}

/* Interact.
 */
 
static int16_t _panda_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: if (!sleeping) {
            sleeping=1;
            fmn_sprite_generate_zzz(sprite);
          } break;
        case FMN_SPELLID_REVEILLE: sleeping=0; break;
        // Possibly too complicated for FMN_SPELLID_PUMPKIN?
      } break;
    case FMN_ITEM_BELL: sleeping=0; break;
  }
  return 0;
}

/* Init.
 */
 
static void _panda_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  sprite->hbw=0.5f;
  sprite->hbn=0.0f;
  sprite->hbe=1.5f;
  sprite->hbs=1.5f;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_panda={
  .init=_panda_init,
  .update=_panda_update,
  .interact=_panda_interact,
};

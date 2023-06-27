#include "app/sprite/fmn_sprite.h"

#define COW_BELL_TIME 0.75f
#define COW_MOO_TIME 0.5f

#define buttsprite ((struct fmn_sprite*)sprite->pv[0])

#define tileid0 sprite->bv[0] /* for both sprites */
#define sleeping sprite->bv[1] /* for both sprites */
#define ishead sprite->bv[2] /* head=1 butt=0 */
#define bellcount sprite->bv[3] /* head */
#define animclock sprite->fv[0] /* for both sprites */
#define belltimer sprite->fv[1] /* head */
#define mootime sprite->fv[2] /* head */

/* Moo.
 */
 
static void cow_moo(struct fmn_sprite *sprite,uint8_t bellc) {
  mootime=COW_MOO_TIME;
  fmn_sound_effect(FMN_SFX_MOO);
}

/* Update.
 */
 
static void _cow_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    sprite->tileid=tileid0+0x21;
  } else {
    if (mootime>=0.0f) {
      mootime-=elapsed;
      sprite->tileid=tileid0+0x96; // moo was a late add
      animclock=0.0f;
    } else {
      animclock+=elapsed;
      if (animclock>=6.0f) animclock-=10.0f; // total period
           if (animclock< 5.0f) sprite->tileid=tileid0; // staring into space
      else if (animclock< 5.5f) sprite->tileid=tileid0+0x20; // mouthful o grass
      else                      sprite->tileid=tileid0+0x10; // chomp
    }
    
    if (belltimer>0.0f) {
      if ((belltimer-=elapsed)<=0.0f) {
        belltimer=0.0f;
        uint8_t c=bellcount;
        bellcount=0;
        cow_moo(sprite,c);
      }
    }
  }
}

static void _cow_butt_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    sprite->tileid=tileid0+0x21;
  } else {
    animclock+=elapsed;
    if (((int)(animclock*2.0f))&1) sprite->tileid=tileid0+0x10;
    else sprite->tileid=tileid0;
  }
}

/* Init.
 */
 
static void _cow_init(struct fmn_sprite *sprite) {
  sprite->invmass=0.0f;
  sprite->pv[0]=fmn_sprite_spawn(sprite->x+1.0f,sprite->y,0,0,0,0,0);
  if (!buttsprite) {
    fmn_sprite_kill(sprite);
    return;
  }
  buttsprite->imageid=sprite->imageid;
  buttsprite->tileid=sprite->tileid+1;
  buttsprite->radius=0.5f;
  buttsprite->invmass=0.0f;
  buttsprite->physics=sprite->physics;
  buttsprite->interact=sprite->interact;
  buttsprite->update=_cow_butt_update;
  sprite->bv[0]=sprite->tileid;
  buttsprite->bv[0]=buttsprite->tileid;
  ishead=1;
}

/* Interact.
 * (sprite) can be the head or the butt.
 */
 
static int16_t _cow_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_PITCHER: {
        if (sleeping) return 0;
        if (qualifier==FMN_PITCHER_CONTENT_EMPTY) return FMN_PITCHER_CONTENT_MILK;
      } break;
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_REVEILLE: sleeping=0; break;
        case FMN_SPELLID_LULLABYE: if (!sleeping) { sleeping=1; if (ishead) fmn_sprite_generate_zzz(sprite); } break;
        case FMN_SPELLID_PUMPKIN: fmn_sprite_pumpkinize(sprite); break;
      } break;
    case FMN_ITEM_BELL: {
        if (sleeping) {
          sleeping=0;
          bellcount=0;
          belltimer=0.0f;
        } else if (ishead) {
          bellcount++;
          belltimer=COW_BELL_TIME;
        }
      } break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_cow={
  .init=_cow_init,
  .update=_cow_update,
  .interact=_cow_interact,
};

#include "app/sprite/fmn_sprite.h"

#define buttsprite ((struct fmn_sprite*)sprite->pv[0])

#define tileid0 sprite->bv[0] /* for both sprites */
#define sleeping sprite->bv[1] /* for both sprites */
#define ishead sprite->bv[2] /* head=1 butt=0 */
#define animclock sprite->fv[0] /* for both sprites */

/* Update.
 */
 
static void _cow_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    sprite->tileid=tileid0+0x21;
  } else {
    animclock+=elapsed;
    if (animclock>=6.0f) animclock-=10.0f; // total period
         if (animclock< 5.0f) sprite->tileid=tileid0; // staring into space
    else if (animclock< 5.5f) sprite->tileid=tileid0+0x20; // mouthful o grass
    else                      sprite->tileid=tileid0+0x10; // chomp
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
        case FMN_SPELLID_LULLABYE: sleeping=1; if (ishead) fmn_sprite_generate_noparam(FMN_SPRCTL_zzz,sprite->x,sprite->y-0.5f); break;
      } break;
    case FMN_ITEM_BELL: sleeping=0; break;
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

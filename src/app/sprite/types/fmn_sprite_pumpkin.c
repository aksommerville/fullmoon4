#include "app/sprite/fmn_sprite.h"
#include "app/sprite/fmn_physics.h"
#include "app/hero/fmn_hero.h"

#define tileid0 sprite->bv[0]
#define pass_thru_interact sprite->bv[1] /* caller may set directly */
#define animclock sprite->fv[0]
#define blinktime sprite->fv[1]
#define restore_sprite sprite->pv[0]

/* Init.
 */
 
static void _pumpkin_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
}

/* Update.
 */

static void _pumpkin_update(struct fmn_sprite *sprite,float elapsed) {

  if (restore_sprite) {
    struct fmn_sprite *other=restore_sprite;
    other->x=sprite->x;
    other->y=sprite->y;
  }

  if (tileid0==0xeb) { // bobbing in water
    animclock+=elapsed;
    if (animclock>=1.0f) animclock=0.0f;
    else if (animclock>=0.5f) sprite->tileid=tileid0+1;
    else sprite->tileid=tileid0;
  
  } else { // occasionally blink
    if (blinktime<=0.0f) {
      blinktime=2.5f+((rand()&0xffff)/20000.0f);
    }
    animclock+=elapsed;
    if (animclock>=blinktime) {
      animclock=0.0f;
      blinktime=0.0f;
      sprite->tileid=tileid0;
    } else if (animclock>=blinktime-0.25f) {
      sprite->tileid=tileid0+1;
    } else {
      sprite->tileid=tileid0;
    }
  }
}

/* Reanimate.
 */
 
static void pumpkin_reanimate(struct fmn_sprite *sprite) {
  if (!restore_sprite) return;
  struct fmn_sprite *original=restore_sprite;
  fmn_sound_effect(FMN_SFX_UNPUMPKIN);
  if (pass_thru_interact&&original->interact) {
    original->interact(original,FMN_ITEM_WAND|0x80,FMN_SPELLID_PUMPKIN);
  }
  fmn_sprite_refunct(original);
  fmn_sprite_kill(sprite);
}

/* Interact.
 */
 
static int16_t _pumpkin_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_PUMPKIN: pumpkin_reanimate(sprite); break;
      } return 0;
  }
  if (pass_thru_interact&&restore_sprite) {
    struct fmn_sprite *original=restore_sprite;
    if (original->interact) return original->interact(original,itemid,qualifier);
  }
  return 0;
}

/* Type definition.
 */

const struct fmn_sprite_controller fmn_sprite_controller_pumpkin={
  .init=_pumpkin_init,
  .update=_pumpkin_update,
  .interact=_pumpkin_interact,
};

/* Pumpkinize a sprite.
 */
 
struct fmn_sprite *fmn_sprite_pumpkinize(struct fmn_sprite *victim) {

  if (!victim) return 0;
  if (victim->style==FMN_SPRITE_STYLE_HIDDEN) return 0;
  
  struct fmn_sprite *sprite=fmn_sprite_generate_noparam(FMN_SPRCTL_pumpkin,victim->x,victim->y);
  if (!sprite) return 0;
  sprite->imageid=3;
  sprite->tileid=0xe9;
  tileid0=sprite->tileid;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->layer=0x80;
  sprite->physics=FMN_PHYSICS_EDGE|FMN_PHYSICS_SPRITES;
  sprite->physics|=victim->physics&(FMN_PHYSICS_SOLID|FMN_PHYSICS_HOLE|FMN_PHYSICS_BLOWABLE);
  sprite->radius=0.3f;
  sprite->invmass=victim->invmass;
  restore_sprite=victim;
  fmn_sprite_defunct(victim);
  
  // If we're over water, use the water sprite 0xeb.
  uint8_t col=sprite->x,row=sprite->y;
  if ((col<FMN_COLC)&&(row<FMN_ROWC)) {
    uint8_t maptile=fmn_global.map[row*FMN_COLC+col];
    switch (fmn_global.cellphysics[maptile]) {
      case FMN_CELLPHYSICS_WATER: {
          sprite->tileid=tileid0=0xeb;
        } break;
    }
  }
  
  fmn_sound_effect(FMN_SFX_PUMPKIN);
  return sprite;
}

/* Unpumpkinize a sprite, if its pumpkin can identify it.
 */
 
static int fmn_sprite_force_unpumpkin_cb(struct fmn_sprite *sprite,void *userdata) {
  struct fmn_sprite *victim=userdata;
  if (sprite->controller!=FMN_SPRCTL_pumpkin) return 0;
  if (restore_sprite!=userdata) return 0;
  fmn_sprite_refunct(victim);
  fmn_sprite_kill(sprite);
  return 1;
}
 
void fmn_sprite_force_unpumpkin(struct fmn_sprite *victim) {
  fmn_sprites_for_each(fmn_sprite_force_unpumpkin_cb,victim);
}

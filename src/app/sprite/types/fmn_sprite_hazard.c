#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define style0 sprite->bv[0]
#define ttl sprite->fv[0] /* infinite if <=0 */

static void _hazard_hero_collision(struct fmn_sprite *sprite,struct fmn_sprite *hero) {
  fmn_hero_injure(sprite->x,sprite->y,sprite);
}

static int16_t _hazard_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_PITCHER: {
        if (qualifier) {
          // If we're a bonfire, get extinguished.
          if ((sprite->imageid==3)&&(sprite->tileid==0x20)) {
            fmn_sound_effect(FMN_SFX_FIZZLE);
            struct fmn_sprite *smoke=fmn_sprite_generate_noparam(FMN_SPRCTL_toast,sprite->x,sprite->y);
            if (smoke) {
              smoke->layer=sprite->layer;
              smoke->imageid=sprite->imageid;
              smoke->tileid=0x8a;
              smoke->style=FMN_SPRITE_STYLE_FOURFRAME;
              smoke->fv[0]=2.0f; // ttl
              smoke->fv[1]=0.0f; // dx
              smoke->fv[2]=0.0f; // dy
            }
            fmn_sprite_kill(sprite);
            return 1;
          }
        } else {
          // Can we provide a pitcherful of fire? That sounds cool.
        }
      } break;
  }
  return 0;
}

static void _hazard_update(struct fmn_sprite *sprite,float elapsed) {
  if (ttl>0.0f) {
    if ((ttl-=elapsed)<=0.0f) {
      fmn_sprite_kill(sprite);
      return;
    }
    if (ttl<0.5f) {
      if (sprite->style==FMN_SPRITE_STYLE_HIDDEN) sprite->style=style0;
      else sprite->style=FMN_SPRITE_STYLE_HIDDEN;
    } else {
      style0=sprite->style;
    }
  }
}

const struct fmn_sprite_controller fmn_sprite_controller_hazard={
  .hero_collision=_hazard_hero_collision,
  .interact=_hazard_interact,
  .update=_hazard_update,
};

#include "app/sprite/fmn_sprite.h"

/* Init.
 */
 
static void _beehive_init(struct fmn_sprite *sprite) {
  uint8_t cmdv[]={
    0x20,sprite->imageid,
    0x21,sprite->tileid+0x10,
    0x42,FMN_SPRCTL_bee>>8,FMN_SPRCTL_bee,
  };
  struct fmn_sprite *ba=fmn_sprite_spawn(sprite->x,sprite->y,0,cmdv,sizeof(cmdv),0,0);
  struct fmn_sprite *bb=fmn_sprite_spawn(sprite->x,sprite->y,0,cmdv,sizeof(cmdv),0,0);
  if (ba) {
    ba->fv[0]=1.0f;
    ba->fv[1]=2.0f;
  }
  if (bb) {
    bb->fv[0]=1.0f+M_PI*0.25f;
    bb->fv[1]=0.5f;
  }
}

/* Interact.
 */
 
static int16_t _beehive_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_PITCHER: {
        if (qualifier==FMN_PITCHER_CONTENT_EMPTY) return FMN_PITCHER_CONTENT_HONEY;
      } break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_beehive={
  .init=_beehive_init,
  .interact=_beehive_interact,
};

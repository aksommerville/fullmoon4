#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

/* Song of Opening: OK, we'll open, but we're leaving a fire here too. Because fuck you.
 * It is still possible to sneak thru. You just need to extinguish or move the fire.
 * The lock itself should be earthquake- and wind-proof.
 */
 
static void bc_open_grudgingly(struct fmn_sprite *sprite) {
  struct fmn_sprite *fire=fmn_sprite_generate_noparam(FMN_SPRCTL_hazard,sprite->x,sprite->y);
  if (fire) {
    fire->imageid=3;
    fire->tileid=0x20;
    fire->style=FMN_SPRITE_STYLE_FOURFRAME;
    fire->radius=0.5f;
    fire->physics=FMN_PHYSICS_BLOWABLE;
  }
  fmn_sprite_kill(sprite);
}

/* Interact.
 */
 
static int16_t _bc_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_OPEN: bc_open_grudgingly(sprite); break;
      } break;
    case FMN_ITEM_COIN: fmn_sprite_kill(sprite); return 1; //TODO gsbit?
  }
  return 0;
}

/* Init.
 */
 
static void _bc_init(struct fmn_sprite *sprite) {
  // If the hero is very close, destroy self.
  // This is typical: bucklock is used over a door, and hero returns from the other side. Let it be unlocked in that case.
  float dx,dy;
  fmn_hero_get_position(&dx,&dy);
  dx-=sprite->x;
  dy-=sprite->y;
  if ((dx>-0.5f)&&(dx<0.5f)&&(dy>-0.5f)&&(dy<0.5f)) {
    fmn_sprite_kill(sprite);
    return;
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_bucklock={
  .init=_bc_init,
  .interact=_bc_interact,
};

#include "app/sprite/fmn_sprite.h"

#define bird_summoned sprite->bv[0]
#define bird_clock sprite->fv[0]

#define SEED_BIRD_TIME 5.0f

/* Init.
 */
 
static void _seed_init(struct fmn_sprite *sprite) {
  sprite->imageid=2;
  sprite->tileid=0x3a;
  sprite->style=FMN_SPRITE_STYLE_TILE;
}

/* Update.
 */
 
static void _seed_update(struct fmn_sprite *sprite,float elapsed) {
  bird_clock+=elapsed;
  if (bird_clock<SEED_BIRD_TIME) return;
  sprite->update=0;
  bird_summoned=1;
  fmn_log("TODO %s:%d summon bird",__FILE__,__LINE__);
}

/* Collision.
 */
 
static void _seed_hero_collision(struct fmn_sprite *sprite,struct fmn_sprite *hero) {
  if (fmn_global.itemqv[FMN_ITEM_SEED]>=0xff) return;
  fmn_sprite_kill(sprite);
  fmn_global.itemqv[FMN_ITEM_SEED]++;
  fmn_global.show_off_item=FMN_ITEM_SEED;
  fmn_global.show_off_item_time=0xff;
  fmn_sound_effect(FMN_SFX_ITEM_MINOR);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_seed={
  .init=_seed_init,
  .update=_seed_update,
  .hero_collision=_seed_hero_collision,
};

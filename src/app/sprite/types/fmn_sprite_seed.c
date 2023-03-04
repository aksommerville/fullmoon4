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
 
static int seed_search_crow(struct fmn_sprite *sprite,void *dummy) {
  if (sprite->controller==FMN_SPRCTL_crow) return 1;
  return 0;
}
 
static void _seed_update(struct fmn_sprite *sprite,float elapsed) {
  bird_clock+=elapsed;
  if (bird_clock<SEED_BIRD_TIME) return;
  sprite->update=0;
  bird_summoned=1;
  
  // If there is already a crow, forget it.
  if (fmn_sprites_for_each(seed_search_crow,0)) return;
  
  // Crow starts centered vertically, and just offscreen, whichever horizontal edge is further.
  // We don't introduce ourselves to the crow; let it find us on its own.
  fmn_sprite_generate_noparam(
    FMN_SPRCTL_crow,
    (sprite->x<(FMN_COLC>>1))?(FMN_COLC+1.0f):(-1.0f),
    FMN_ROWC>>1
  );
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

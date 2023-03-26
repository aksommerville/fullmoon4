#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define FLOORFIRE_LEAD_SPEED 8.0f
#define FLOORFIRE_END_RADIUS 30.0f
#define FLOORFIRE_RING_SPACING 0.750f
#define FLOORFIRE_LEAD_SAFE_RADIUS 2.0f
#define FLOORFIRE_DANGER_WIDTH 4.0f

#define clock sprite->fv[0]
#define lead_radius sprite->fv[1]
#define first_radius sprite->fv[2]
#define ringc sprite->bv[0]

static void _floorfire_init(struct fmn_sprite *sprite) {
  sprite->tileid=0xb0;
  sprite->radius=0.5f;
  sprite->layer=80;
  sprite->style=FMN_SPRITE_STYLE_FLOORFIRE;
  lead_radius=1.0f;
  first_radius=lead_radius;
}

static void floorfire_check_hero(struct fmn_sprite *sprite) {
  if (fmn_global.active_item==FMN_ITEM_BROOM) return; // can't burn her while flying
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float dx=herox-sprite->x;
  float dy=heroy-sprite->y;
  float distance=sqrtf(dx*dx+dy*dy);
  if (distance>lead_radius-FLOORFIRE_LEAD_SAFE_RADIUS) return; // out of range, outside
  if (distance<lead_radius-FLOORFIRE_LEAD_SAFE_RADIUS-FLOORFIRE_DANGER_WIDTH) return; // out of range, inside
  fmn_hero_kill(sprite);
}

static void _floorfire_update(struct fmn_sprite *sprite,float elapsed) {
  clock+=elapsed;
  lead_radius+=FLOORFIRE_LEAD_SPEED*elapsed;
  if (lead_radius>FLOORFIRE_END_RADIUS) {
    fmn_sprite_kill(sprite);
    return;
  }
  if (lead_radius>=first_radius+FLOORFIRE_RING_SPACING) {
    first_radius+=FLOORFIRE_RING_SPACING;
    ringc++;
  }
  floorfire_check_hero(sprite);
}

const struct fmn_sprite_controller fmn_sprite_controller_floorfire={
  .init=_floorfire_init,
  .update=_floorfire_update,
};

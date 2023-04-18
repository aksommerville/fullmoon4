#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"
#include <stdio.h>

#define pvitem sprite->bv[0]
#define tileid0 sprite->bv[1]
#define activated sprite->bv[2]
#define stage sprite->bv[3]
#define frame sprite->bv[4]
#define clock sprite->fv[0]
#define gsbit sprite->argv[0]

#define TREE_STAGE_PLANTED 0
#define TREE_STAGE_LEGGING 1
#define TREE_STAGE_MARCHING 2

#define TREE_MARCH_SPEED 1.0f

/* Hero stepped on the balcony or donned her hat.
 */
 
static void _tree_trigger(void *userdata,uint16_t eventid,void *payload) {
  struct fmn_sprite *sprite=userdata;
  if (fmn_global.selected_item!=FMN_ITEM_HAT) return;
  if (activated) return;
  activated=1;
  fmn_gs_set_bit(gsbit,1);
  stage=TREE_STAGE_LEGGING;
  clock=0.0f;
  frame=0;
}

/* Starting up and our bit is already set. Advance as far north as we can.
 * Since cellphysics might not be loaded, and that's a pain anyway, walk only on identical tiles.
 */
 
static void _tree_move_north_immediately(struct fmn_sprite *sprite) {
  int8_t x=sprite->x,y=sprite->y;
  if ((x<0)||(x>=FMN_COLC)) return;
  if (y>=FMN_ROWC) return;
  uint8_t tileid_home=fmn_global.map[y*FMN_COLC+x];
  while (y>0) {
    y--;
    uint8_t tileid=fmn_global.map[y*FMN_COLC+x];
    if (tileid!=tileid_home) break;
    sprite->y-=1.0f;
  }
}

/* Init.
 */
 
static void _tree_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  if (fmn_gs_get_bit(gsbit)) {
    activated=1;
    _tree_move_north_immediately(sprite);
  } else {
    fmn_game_event_listen(FMN_GAME_EVENT_BALCONY,_tree_trigger,sprite);
  }
}

/* Update.
 */
 
static void _tree_update(struct fmn_sprite *sprite,float elapsed) {
  if (fmn_global.selected_item!=pvitem) {
    pvitem=fmn_global.selected_item;
    if (pvitem==FMN_ITEM_HAT) {
      _tree_trigger(sprite,0,0);
    }
  }
  if (!activated) {
    sprite->tileid=tileid0;
    return;
  }
  clock+=elapsed;
  switch (stage) {
    case TREE_STAGE_PLANTED: sprite->tileid=tileid0; return;
    case TREE_STAGE_LEGGING: {
        if (clock>=0.25f) {
          clock=0.0f;
          frame++;
          if (frame>=3) {
            frame=0;
            stage=TREE_STAGE_MARCHING;
          } else {
            sprite->tileid=tileid0+frame;
          }
        }
      } break;
    case TREE_STAGE_MARCHING: {
        sprite->y-=elapsed*TREE_MARCH_SPEED;
        if (clock>=0.125f) {
          clock=0.0f;
          frame++;
          if (frame>=2) frame=0;
        }
        sprite->tileid=tileid0+3+frame;
      } break;
  }
}

/* Pressure.
 */
 
static void _tree_pressure(struct fmn_sprite *sprite,struct fmn_sprite *dummy,uint8_t dir) {
  if ((dir==FMN_DIR_S)&&(stage==TREE_STAGE_MARCHING)) {
    stage=TREE_STAGE_PLANTED;
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_tree={
  .init=_tree_init,
  .update=_tree_update,
  .static_pressure=_tree_pressure,
};

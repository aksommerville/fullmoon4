#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

#define tileid0 sprite->bv[0]
#define selection sprite->bv[1]
#define gsbit_selection sprite->argv[0]

/* Treadmill.
 */
 
static void _ms_treadmill(void *userdata,uint16_t eventid,void *payload) {
  if (!payload) return;
  struct fmn_sprite *sprite=userdata;
  selection+=*(int*)payload;
  selection&=0x03;
  switch (selection) {
    case 0: sprite->tileid=tileid0+0x00; break;
    case 1: sprite->tileid=tileid0+0x11; break;
    case 2: sprite->tileid=tileid0+0x02; break;
    case 3: sprite->tileid=tileid0+0x12; break;
  }
}

/* Init.
 */
 
static void _ms_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  sprite->hbw=0.5f;
  sprite->hbn=0.5f;
  sprite->hbe=1.5f;
  sprite->hbs=1.5f;
  fmn_game_event_listen(FMN_GAME_EVENT_TREADMILL,_ms_treadmill,sprite);
  switch (selection=fmn_gs_get_word(gsbit_selection,2)) {
    case 0: sprite->tileid=tileid0+0x00; break;
    case 1: sprite->tileid=tileid0+0x11; break;
    case 2: sprite->tileid=tileid0+0x02; break;
    case 3: sprite->tileid=tileid0+0x12; break;
  }
  struct fmn_sprite *dep;
  if (dep=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x+1.0f,sprite->y)) {
    dep->imageid=sprite->imageid;
    dep->tileid=tileid0+0x01;
    dep->style=FMN_SPRITE_STYLE_TILE;
  }
  if (dep=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x,sprite->y+1.0f)) {
    dep->imageid=sprite->imageid;
    dep->tileid=tileid0+0x10;
    dep->style=FMN_SPRITE_STYLE_TILE;
  }
  if (dep=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x+1.0f,sprite->y+1.0f)) {
    dep->imageid=sprite->imageid;
    dep->tileid=tileid0+0x10;
    dep->xform=FMN_XFORM_XREV;
    dep->style=FMN_SPRITE_STYLE_TILE;
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_musicstand={
  .init=_ms_init,
};

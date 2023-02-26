#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define radius 0.60f /* tracks hero's midpoint, so actually a little more than half */

#define pressed sprite->bv[0]
#define state sprite->bv[1]
#define tileid0 sprite->bv[2]
#define stompbox sprite->bv[3] /* set in template please */
#define gsbit sprite->argv[0] /* zero means detached, not bit zero */

/* Notify of change, for stompbox.
 */
 
static void _stompbox_cb_changed(void *userdata,uint16_t p,uint8_t v) {
  struct fmn_sprite *sprite=userdata;
  if (v) {
    if (state) return;
    state=1;
    sprite->tileid=tileid0+(pressed?3:2);
  } else {
    if (!state) return;
    state=0;
    sprite->tileid=tileid0+(pressed?1:0);
  }
}

/* Init.
 */
 
static void _treadle_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  if (stompbox) {
    if (gsbit) {
      if (fmn_gs_get_bit(gsbit)) {
        state=1;
        sprite->tileid=tileid0+2;
      }
      fmn_gs_listen_bit(gsbit,_stompbox_cb_changed,sprite);
    }
  } else {
    // Treadle plates clear their gs bit on init, since they start in the Off state.
    if (gsbit) {
      fmn_gs_set_bit(gsbit,0);
    }
  }
}

/* State changes.
 */
 
static void _treadle_press(struct fmn_sprite *sprite) {
  sprite->tileid=tileid0+1;
  state=1;
  if (gsbit) fmn_gs_set_bit(gsbit,1);
  fmn_sound_effect(FMN_SFX_TREADLE_PRESS);
}
 
static void _treadle_release(struct fmn_sprite *sprite) {
  sprite->tileid=tileid0;
  state=0;
  if (gsbit) fmn_gs_set_bit(gsbit,0);
  fmn_sound_effect(FMN_SFX_TREADLE_RELEASE);
}
 
static void _stompbox_press(struct fmn_sprite *sprite) {
  fmn_sound_effect(FMN_SFX_TREADLE_PRESS);
  if (state) {
    state=0;
    sprite->tileid=tileid0+1;
    if (gsbit) fmn_gs_set_bit(gsbit,0);
  } else {
    state=1;
    sprite->tileid=tileid0+3;
    if (gsbit) fmn_gs_set_bit(gsbit,1);
  }
}
 
static void _stompbox_release(struct fmn_sprite *sprite) {
  fmn_sound_effect(FMN_SFX_TREADLE_RELEASE);
  if (state) sprite->tileid=tileid0+2;
  else sprite->tileid=tileid0;
}

/* Read current state.
 */
 
struct treadle_bounds {
  float w,e,n,s;
};

static int _treadle_current_state_1(struct fmn_sprite *q,void *userdata) {
  const struct treadle_bounds *bounds=userdata;
  if (!(q->physics&FMN_PHYSICS_SPRITES)) return 0;
  if (q->x<bounds->w) return 0;
  if (q->x>bounds->e) return 0;
  if (q->y<bounds->n) return 0;
  if (q->y>bounds->s) return 0;
  if (q->style==FMN_SPRITE_STYLE_HERO) {
    if (!fmn_hero_feet_on_ground()) return 0;
  }
  return 1;
}
 
static uint8_t _treadle_current_state(const struct fmn_sprite *sprite) {
  struct treadle_bounds bounds={
    .w=sprite->x-radius,
    .e=sprite->x+radius,
    .n=sprite->y-radius,
    .s=sprite->y+radius,
  };
  return fmn_sprites_for_each(_treadle_current_state_1,&bounds);
}

/* Update.
 */
 
static void _treadle_update(struct fmn_sprite *sprite,float elapsed) {
  uint8_t foot=_treadle_current_state(sprite);
  if (foot==pressed) return;
  pressed=foot;
  if (stompbox) {
    if (pressed) _stompbox_press(sprite);
    else _stompbox_release(sprite);
  } else {
    if (pressed) _treadle_press(sprite);
    else _treadle_release(sprite);
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_treadle={
  .init=_treadle_init,
  .update=_treadle_update,
};

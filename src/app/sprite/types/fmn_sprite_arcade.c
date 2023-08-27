#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"
#include "app/eatwq/fmn_eatwq.h"

#define gsbit_coinc sprite->argv[0]
#define gsbit_hiscore sprite->argv[1]

#define playing sprite->bv[0]
#define blackout sprite->fv[1]

static struct fmn_eatwq_context arcade_context={0};

/* Power (create two dummy sprites showing a lit-up screen.
 */
 
static void arcade_power_on(struct fmn_sprite *sprite) {
  struct fmn_sprite *l=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x-0.5f,sprite->y-1.0f);
  struct fmn_sprite *r=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x+0.5f,sprite->y-1.0f);
  if (!l||!r) {
    fmn_sprite_kill(l);
    fmn_sprite_kill(r);
    return;
  }
  l->imageid=r->imageid=sprite->imageid;
  l->tileid=sprite->tileid+0x01;
  r->tileid=sprite->tileid+0x11;
  l->style=r->style=FMN_SPRITE_STYLE_TWOFRAME;
}

static int arcade_power_off_cb(struct fmn_sprite *q,void *userdata) {
  struct fmn_sprite *sprite=userdata;
  if (q->controller!=FMN_SPRCTL_dummy) return 0;
  if (q->imageid!=sprite->imageid) return 0;
  if ((q->tileid!=sprite->tileid+0x01)&&(q->tileid!=sprite->tileid+0x11)) return 0;
  fmn_sprite_kill(q);
  return 0;
}

static void arcade_power_off(struct fmn_sprite *sprite) {
  fmn_sprites_for_each(arcade_power_off_cb,sprite);
}

/* Update.
 */
 
static void _arcade_update(struct fmn_sprite *sprite,float elapsed) {
  if (playing) {
    // First update after quitting the game.
    playing=0;
    fmn_gs_set_word(gsbit_coinc,3,arcade_context.creditc);
    fmn_gs_set_word(gsbit_hiscore,8,arcade_context.hiscore);
    if (!arcade_context.creditc) arcade_power_off(sprite);
  }
  if ((blackout-=elapsed)<0.0f) blackout=0.0f;
}

/* Interact.
 */
 
static int16_t _arcade_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_COIN: {
        int coinc=fmn_gs_get_word(gsbit_coinc,3);
        if (coinc<7) {
          fmn_sound_effect(FMN_SFX_PAYMENT);
          coinc++;
          fmn_gs_set_word(gsbit_coinc,3,coinc);
          if (coinc==1) arcade_power_on(sprite);
          return 1;
        }
      } break;
  }
  return 0;
}

/* Hero collision.
 */
 
static void _arcade_pressure(struct fmn_sprite *sprite,struct fmn_sprite *pressure,uint8_t dir) {
  if (blackout>0.0f) return;
  arcade_context.creditc=fmn_gs_get_word(gsbit_coinc,3);
  if (!arcade_context.creditc) return;
  arcade_context.hiscore=fmn_gs_get_word(gsbit_hiscore,8);
  fmn_eatwq_begin(&arcade_context);
  blackout=0.5f;
  playing=1;
}

/* Init.
 */
 
static void _arcade_init(struct fmn_sprite *sprite) {
  sprite->x+=0.5f; // cabinet is 2 columns wide; we should be placed on the left.
  int coinc=fmn_gs_get_word(gsbit_coinc,3);
  if (coinc) arcade_power_on(sprite);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_arcade={
  .init=_arcade_init,
  .update=_arcade_update,
  .interact=_arcade_interact,
  .pressure=_arcade_pressure,
};

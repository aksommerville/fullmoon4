#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define range sprite->argv[0]
#define off_time_ds sprite->argv[1]
#define init_phase sprite->argv[2] /* gsbit if (off_time_ds==0) */

// Direction is knowable canonically from (xform), but we express more comprehensibly as (dx,dy).
#define dx sprite->sv[0]
#define dy sprite->sv[1]

#define period sprite->fv[0]
#define clock sprite->fv[1]
#define tileid0 sprite->bv[0]
#define status sprite->bv[1]
#define flamec sprite->bv[2]

#define on_time 1.0f
#define huff_time 0.80f
#define width 0.600f

// (status) is shared with the renderer
#define FIRENOZZLE_STATUS_IDLE 0
#define FIRENOZZLE_STATUS_HUFF 1
#define FIRENOZZLE_STATUS_PUFF 2

/* gsbit change.
 */
 
static void _firenozzle_gsbit(void *userdata,uint16_t p,uint8_t v) {
  struct fmn_sprite *sprite=userdata;
  if (v) {
    if (status==FIRENOZZLE_STATUS_IDLE) return;
    status=FIRENOZZLE_STATUS_IDLE;
  } else {
    if (status==FIRENOZZLE_STATUS_PUFF) return;
    status=FIRENOZZLE_STATUS_PUFF;
  }
}

/* Init.
 */
 
static void _firenozzle_init(struct fmn_sprite *sprite) {
  switch (sprite->xform) {
    case 0: { // facing east
        dx=1;
      } break;
    case FMN_XFORM_XREV: { // facing west
        dx=-1;
      } break;
    case FMN_XFORM_SWAP: { // facing south
        dy=1;
      } break;
    case FMN_XFORM_SWAP|FMN_XFORM_XREV: { // facing north
        dy=-1;
      } break;
    default: {
        fmn_log("Invalid xform 0x%02x for firenozzle. Dropping sprite.",sprite->xform);
        fmn_sprite_kill(sprite);
        return;
      }
  }
  sprite->x+=dx*0.20f;
  sprite->y+=dy*0.20f;
  if (range<2) {
    fmn_log("Invalid firenozzle range %d, using minimum 2 instead.",range);
    range=2;
  }
  flamec=range;
  tileid0=sprite->tileid;
  if (off_time_ds) {
    // Run on a cycle, no controller bit.
    period=off_time_ds/10.0f+on_time;
    clock=(init_phase*period)/256.0f;
  } else if (init_phase) {
    // Control via gsbit. (note that the sense is inverted: BIT OFF means FIRE ON)
    period=clock=0.0f;
    fmn_gs_listen_bit(init_phase,_firenozzle_gsbit,sprite);
    if (fmn_gs_get_bit(init_phase)) {
      status=FIRENOZZLE_STATUS_IDLE;
    } else {
      status=FIRENOZZLE_STATUS_PUFF;
    }
  } else {
    // Always on.
    status=FIRENOZZLE_STATUS_PUFF;
  }
}

/* Check hero.
 */
 
static void firenozzle_check_hero(struct fmn_sprite *sprite) {
  float herox=0.0f,heroy=0.0f;
  fmn_hero_get_position(&herox,&heroy);
  float harmx=herox,harmy=heroy;
  const float harmr=0.25f;
  float reach=range+0.50f; // our graphics put the nozzle right at the edge of the tile. reach out a bit.
  if (dy>0) reach+=0.50f;
  if (dx) { // horizontal orientation
    if (heroy<sprite->y-width) return;
    if (heroy>sprite->y+width) return;
    if (dx<0) {
      if (herox>=sprite->x) return;
      if (herox<sprite->x-reach) return;
    } else {
      if (herox<=sprite->x) return;
      if (herox>sprite->x+reach) return;
    }
    if (heroy<sprite->y) harmy+=harmr;
    else harmy-=harmr;
  } else { // vertical orientation
    if (herox<sprite->x-width) return;
    if (herox>sprite->x+width) return;
    if (dy<0) {
      if (heroy>=sprite->y) return;
      if (heroy<sprite->y-reach) return;
    } else {
      if (heroy<=sprite->y) return;
      if (heroy>sprite->y+reach) return;
    }
    if (herox<sprite->x) harmx+=harmr;
    else harmx-=harmr;
  }
  fmn_hero_injure(harmx,harmy,sprite);
}

/* Update.
 */
 
static void _firenozzle_update(struct fmn_sprite *sprite,float elapsed) {
  if (off_time_ds) { // timer in play
    clock+=elapsed;
    while (clock>=period) clock-=period;
    if (clock>=period-on_time) {
      if (status!=FIRENOZZLE_STATUS_PUFF) {
        fmn_sound_effect(FMN_SFX_FIRENOZZLE_ON);
        status=FIRENOZZLE_STATUS_PUFF;
      }
      firenozzle_check_hero(sprite);
    } else if (clock>=period-on_time-huff_time) status=FIRENOZZLE_STATUS_HUFF;
    else status=FIRENOZZLE_STATUS_IDLE;
  } else if (status==FIRENOZZLE_STATUS_PUFF) { // controlled externally
    firenozzle_check_hero(sprite);
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_firenozzle={
  .init=_firenozzle_init,
  .update=_firenozzle_update,
};

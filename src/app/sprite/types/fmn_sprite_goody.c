#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

#define tileid0 sprite->bv[0]
#define spellp sprite->bv[1]
#define stage sprite->bv[2]
#define style0 sprite->bv[3]
#define clock sprite->fv[0]

#define GOODY_STAGE_LEAD 0
#define GOODY_STAGE_STROKE 1
#define GOODY_STAGE_BETWEEN 2
#define GOODY_STAGE_INVISIBLE 3

#define GOODY_LEAD_TIME      2.000f
#define GOODY_STROKE_TIME    0.500f
#define GOODY_BETWEEN_TIME   0.500f
#define GOODY_INVISIBLE_TIME 5.000f

static uint8_t goody_spell[16];
static uint8_t goody_spellc=0;

/* Enter STROKE stage if there's any remaining, otherwise INVISIBLE (or LEAD if empty).
 */
 
static void goody_begin_stroke(struct fmn_sprite *sprite) {
  if (spellp<goody_spellc) {
    stage=GOODY_STAGE_STROKE;
    switch (goody_spell[spellp]) {
      case FMN_DIR_W: sprite->tileid=1; break;
      case FMN_DIR_E: sprite->tileid=2; break;
      case FMN_DIR_N: sprite->tileid=3; break;
      case FMN_DIR_S: sprite->tileid=4; break;
      default: sprite->tileid=0;
    }
    sprite->tileid+=tileid0;
    clock=GOODY_STROKE_TIME;
  } else if (goody_spellc) {
    stage=GOODY_STAGE_INVISIBLE;
    sprite->style=FMN_SPRITE_STYLE_HIDDEN;
    sprite->tileid=tileid0;
    clock=GOODY_INVISIBLE_TIME;
  } else {
    stage=GOODY_STAGE_LEAD;
    sprite->tileid=tileid0;
    clock=GOODY_LEAD_TIME;
  }
}

/* Update.
 */
 
static void _goody_update(struct fmn_sprite *sprite,float elapsed) {
  clock-=elapsed;
  if (clock<=0.0f) switch (stage) {
  
    case GOODY_STAGE_LEAD: {
        spellp=0;
        goody_begin_stroke(sprite);
      } break;
      
    case GOODY_STAGE_STROKE: {
        spellp++;
        stage=GOODY_STAGE_BETWEEN;
        clock=GOODY_BETWEEN_TIME;
        sprite->tileid=tileid0;
      } break;
      
    case GOODY_STAGE_BETWEEN: {
        goody_begin_stroke(sprite);
      } break;
      
    case GOODY_STAGE_INVISIBLE: {
        stage=GOODY_STAGE_LEAD;
        clock=GOODY_LEAD_TIME;
        sprite->style=style0;
      } break;
      
  }
}

/* Init.
 */
 
static void _goody_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  style0=sprite->style;
  sprite->hbn=0.3f;
  sprite->hbs=1.25f;
  sprite->hbw=0.3f;
  sprite->hbe=0.3f;
  goody_spellc=fmn_spell_get(goody_spell,sizeof(goody_spell),FMN_SPELLID_INVISIBLE);
  if (goody_spellc>sizeof(goody_spell)) {
    goody_spellc=0;
  }
  clock=GOODY_LEAD_TIME;
}

/* Init.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_goody={
  .init=_goody_init,
  .update=_goody_update,
};

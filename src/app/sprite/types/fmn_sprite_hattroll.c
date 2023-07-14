#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define tileid0 sprite->bv[0]
#define state sprite->bv[1]
#define substate sprite->bv[2]
#define clock sprite->fv[0]

#define HATTROLL_STATE_ANGRY 0
#define HATTROLL_STATE_SOCIAL 1
#define HATTROLL_STATE_CHARM 2
#define HATTROLL_STATE_SLEEP 3

#define HATTROLL_SUBSTATE_IDLE 0 /* ANGRY... */
#define HATTROLL_SUBSTATE_BRANDISH 1
#define HATTROLL_SUBSTATE_FOLLOWTHRU 2
#define HATTROLL_SUBSTATE_WAVE1 3 /* SOCIAL... (IDLE also valid) */
#define HATTROLL_SUBSTATE_WAVE2 4
#define HATTROLL_SUBSTATE_BLINK 5
#define HATTROLL_SUBSTATE_SCRATCH1 6
#define HATTROLL_SUBSTATE_SCRATCH2 7

#define HATTROLL_THROW_TIME_MIN 0.5f
#define HATTROLL_THROW_TIME_MAX 2.0f
#define HATTROLL_BRANDISH_TIME 0.5f
#define HATTROLL_FOLLOWTHRU_TIME 0.5f
#define HATTROLL_BLINK_TIME 0.25f
#define HATTROLL_SCRATCH_TIME 0.25f
#define HATTROLL_WAVE_TIME 0.5f
#define HATTROLL_RELAX_TIME_MIN 1.0f
#define HATTROLL_RELAX_TIME_MAX 3.0f
#define HATTROLL_WAVE_RADIUS 0.75f

static void hattroll_begin_angry(struct fmn_sprite *sprite);
static void hattroll_begin_social(struct fmn_sprite *sprite);

/* Rock management.
 */

struct hattroll_find_rock_context {
  struct fmn_sprite *sprite;
  struct fmn_sprite *rock;
};

static int hattroll_find_rock_cb(struct fmn_sprite *q,void *userdata) {
  struct hattroll_find_rock_context *ctx=userdata;
  struct fmn_sprite *sprite=ctx->sprite;
  if (q->controller!=FMN_SPRCTL_missile) return 0;
  if (q->pv[0]!=sprite) return 0;
  ctx->rock=q;
  return 1;
}
 
static struct fmn_sprite *hattroll_find_rock(struct fmn_sprite *sprite) {
  struct hattroll_find_rock_context ctx={
    .sprite=sprite,
  };
  fmn_sprites_for_each(hattroll_find_rock_cb,&ctx);
  return ctx.rock;
}
 
static void hattroll_drop_rock(struct fmn_sprite *sprite) {
  struct fmn_sprite *rock=hattroll_find_rock(sprite);
  if (!rock) return;
  fmn_sprite_kill(rock);
  if ((state==HATTROLL_STATE_ANGRY)&&(substate==HATTROLL_SUBSTATE_BRANDISH)) {
    substate=HATTROLL_SUBSTATE_IDLE;
    sprite->tileid=tileid0;
  } else if ((state==HATTROLL_STATE_ANGRY)&&(substate==HATTROLL_SUBSTATE_FOLLOWTHRU)) {
    substate=HATTROLL_SUBSTATE_IDLE;
    sprite->tileid=tileid0;
  }
}
 
static void hattroll_throw_rock(struct fmn_sprite *sprite) {
  struct fmn_sprite *rock=hattroll_find_rock(sprite);
  if (rock) {
    rock->pv[0]=0;
  }
  substate=HATTROLL_SUBSTATE_FOLLOWTHRU;
  sprite->tileid=tileid0+2;
  clock=HATTROLL_FOLLOWTHRU_TIME;
}

/* Create a rock and start brandishing it.
 */
 
static void hattroll_brandish_rock(struct fmn_sprite *sprite) {
  struct fmn_sprite *rock=fmn_sprite_generate_noparam(FMN_SPRCTL_missile,sprite->x,sprite->y-0.5f);
  if (!rock) {
    hattroll_begin_angry(sprite);
    return;
  }
  rock->pv[0]=sprite; // holder
  rock->fv[3]=(sprite->xform&FMN_XFORM_XREV)?0.375f:-0.375f; // xoffset_held
  rock->fv[4]=-0.125f; // yoffset_held
  rock->imageid=sprite->imageid;
  rock->tileid=tileid0-1;
  rock->style=FMN_SPRITE_STYLE_TILE;
  substate=HATTROLL_SUBSTATE_BRANDISH;
  sprite->tileid=tileid0+1;
  clock=HATTROLL_BRANDISH_TIME;
}

/* Blink or scratch my butt.
 */
 
static void hattroll_do_social_thing(struct fmn_sprite *sprite) {
  switch (substate) {
    case HATTROLL_SUBSTATE_BLINK: hattroll_begin_social(sprite); break;
    case HATTROLL_SUBSTATE_WAVE1: sprite->tileid=tileid0+5; substate=HATTROLL_SUBSTATE_WAVE2; clock=HATTROLL_WAVE_TIME; break;
    case HATTROLL_SUBSTATE_WAVE2: sprite->tileid=tileid0+4; substate=HATTROLL_SUBSTATE_WAVE1; clock=HATTROLL_WAVE_TIME; break;
    case HATTROLL_SUBSTATE_SCRATCH2: hattroll_begin_social(sprite); break;
    case HATTROLL_SUBSTATE_SCRATCH1: {
        substate=HATTROLL_SUBSTATE_SCRATCH2;
        clock=HATTROLL_SCRATCH_TIME;
        sprite->tileid=tileid0+8;
      } break;
    default: {
        if (rand()&1) {
          substate=HATTROLL_SUBSTATE_BLINK;
          clock=HATTROLL_BLINK_TIME;
          sprite->tileid=tileid0+6;
        } else {
          substate=HATTROLL_SUBSTATE_SCRATCH1;
          clock=HATTROLL_SCRATCH_TIME;
          sprite->tileid=tileid0+7;
        }
      }
  }
}

/* Begin social state.
 */
 
static void hattroll_begin_social(struct fmn_sprite *sprite) {
  hattroll_drop_rock(sprite);
  state=HATTROLL_STATE_SOCIAL;
  substate=HATTROLL_SUBSTATE_IDLE;
  sprite->tileid=tileid0+3;
  clock=HATTROLL_RELAX_TIME_MIN+((rand()&0xffff)*(HATTROLL_RELAX_TIME_MAX-HATTROLL_RELAX_TIME_MIN))/65536.0f;
}

/* Begin ANGRY state, or if the hat is on, begin social instead.
 */
 
static void hattroll_begin_angry(struct fmn_sprite *sprite) {
  if (fmn_global.selected_item==FMN_ITEM_HAT) {
    hattroll_begin_social(sprite);
    return;
  }
  state=HATTROLL_STATE_ANGRY;
  substate=HATTROLL_SUBSTATE_IDLE;
  sprite->tileid=tileid0;
  clock=HATTROLL_THROW_TIME_MIN+((rand()&0xffff)*(HATTROLL_THROW_TIME_MAX-HATTROLL_THROW_TIME_MIN))/65536.0f;
}

/* Turn to face the hero. Nonzero if she's in range vertically.
 */
 
static uint8_t hattroll_face_hero(struct fmn_sprite *sprite) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  uint8_t result=(heroy>=sprite->y-HATTROLL_WAVE_RADIUS)&&(heroy<=sprite->y+HATTROLL_WAVE_RADIUS);
  if (herox<sprite->x) {
    if (sprite->xform&FMN_XFORM_XREV) return result;
    sprite->xform|=FMN_XFORM_XREV;
  } else {
    if (!(sprite->xform&FMN_XFORM_XREV)) return result;
    sprite->xform&=~FMN_XFORM_XREV;
  }
  if (state==HATTROLL_STATE_ANGRY) {
    struct fmn_sprite *rock=hattroll_find_rock(sprite);
    if (rock) {
      rock->fv[3]*=-1.0f;
      rock->xform^=FMN_XFORM_XREV;
    }
  }
  return result;
}

/* Update.
 */
 
static void _hattroll_update(struct fmn_sprite *sprite,float elapsed) {
  switch (state) {
  
    case HATTROLL_STATE_ANGRY: {
        hattroll_face_hero(sprite);
        if (fmn_global.selected_item==FMN_ITEM_HAT) {
          hattroll_begin_social(sprite);
          return;
        }
        if ((clock-=elapsed)<=0.0f) {
          if (substate==HATTROLL_SUBSTATE_BRANDISH) {
            hattroll_throw_rock(sprite);
          } else if (substate==HATTROLL_SUBSTATE_FOLLOWTHRU) {
            hattroll_begin_angry(sprite);
          } else {
            hattroll_brandish_rock(sprite);
          }
        }
      } break;
      
    case HATTROLL_STATE_SOCIAL: {
        uint8_t line_of_sight=hattroll_face_hero(sprite);
        if (fmn_global.selected_item!=FMN_ITEM_HAT) {
          hattroll_begin_angry(sprite);
          return;
        }
        if (line_of_sight) {
          if ((substate==HATTROLL_SUBSTATE_WAVE1)||(substate==HATTROLL_SUBSTATE_WAVE2)) {
          } else {
            substate=HATTROLL_SUBSTATE_WAVE1;
            sprite->tileid=tileid0+4;
            clock=HATTROLL_WAVE_TIME;
          }
        } else if ((substate==HATTROLL_SUBSTATE_WAVE1)||(substate==HATTROLL_SUBSTATE_WAVE2)) {
          hattroll_begin_social(sprite);
        }
        if ((clock-=elapsed)<=0.0f) {
          hattroll_do_social_thing(sprite);//TODO or finish social thing
        }
      } break;
      
    case HATTROLL_STATE_CHARM: {
        hattroll_face_hero(sprite);
      } break;
  }
}

/* Sleep or wake.
 */
 
static void hattroll_sleep(struct fmn_sprite *sprite,uint8_t sleep) {
  hattroll_drop_rock(sprite);
  if (sleep) {
    if (state==HATTROLL_STATE_SLEEP) return;
    if (state==HATTROLL_STATE_CHARM) fmn_sprite_kill_enchantment(sprite);
    state=HATTROLL_STATE_SLEEP;
    sprite->tileid=tileid0+9;
    fmn_sprite_generate_zzz(sprite);
  } else {
    if (state!=HATTROLL_STATE_SLEEP) return;
    hattroll_begin_angry(sprite);
  }
}

/* Charm.
 */
 
static void hattroll_charm(struct fmn_sprite *sprite) {
  hattroll_drop_rock(sprite);
  if (state==HATTROLL_STATE_SLEEP) return;
  if (state==HATTROLL_STATE_CHARM) return;
  state=HATTROLL_STATE_CHARM;
  sprite->tileid=tileid0+3;
  fmn_sprite_generate_enchantment(sprite,1);
  fmn_sound_effect(FMN_SFX_ENCHANT_ANIMAL);
}

/* Interact.
 */
 
static int16_t _hattroll_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: hattroll_sleep(sprite,1); break;
        case FMN_SPELLID_REVEILLE: hattroll_sleep(sprite,0); break;
        case FMN_SPELLID_PUMPKIN: hattroll_drop_rock(sprite); fmn_sprite_pumpkinize(sprite); break;
      } break;
    case FMN_ITEM_BELL: hattroll_sleep(sprite,0); break;
    case FMN_ITEM_FEATHER: hattroll_charm(sprite); break;
  }
  return 0;
}

/* Init.
 */
 
static void _hattroll_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  hattroll_begin_angry(sprite);
}

/* Type definition.
 */

const struct fmn_sprite_controller fmn_sprite_controller_hattroll={
  .init=_hattroll_init,
  .update=_hattroll_update,
  .interact=_hattroll_interact,
};

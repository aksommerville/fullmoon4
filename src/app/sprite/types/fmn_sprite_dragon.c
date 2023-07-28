#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define tileid0 sprite->bv[0]
#define stage sprite->bv[1]
#define clock sprite->fv[0]
#define animclock sprite->fv[1]
#define dir sprite->argv[0]
#define phase sprite->argv[1]

// We abuse (tileid) as dragon-specific frames 0..5. Our renderer knows what to do with them.
#define DRAGON_TILE_IDLE 0
#define DRAGON_TILE_BLINK 1
#define DRAGON_TILE_FIRE1 2
#define DRAGON_TILE_FIRE2 3
#define DRAGON_TILE_PUMPKIN1 4
#define DRAGON_TILE_PUMPKIN2 5

#define DRAGON_STAGE_IDLE 0
#define DRAGON_STAGE_FLOOM 1
#define DRAGON_STAGE_SLEEP 2
#define DRAGON_STAGE_PUMPKIN 3
#define DRAGON_STAGE_CHARM 4

#define DRAGON_IDLE_TIME 2.0f

#define DRAGON_BLINK_TIME_MIN 2.0f
#define DRAGON_BLINK_TIME_MAX 5.0f
#define DRAGON_BLINK_DOWN_TIME 0.25f

#define DRAGON_FLOOM_TIME 2.25f
#define DRAGON_FLOOM_FRAME_TIME 0.125f

#define DRAGON_PUMPKIN_FRAME_TIME 0.125f

/* Begin FLOOM.
 */
 
static void dragon_begin_FLOOM(struct fmn_sprite *sprite) {
  stage=DRAGON_STAGE_FLOOM;
  sprite->tileid=DRAGON_TILE_FIRE1;
  clock=DRAGON_FLOOM_TIME;
  animclock=DRAGON_FLOOM_FRAME_TIME;
}

/* Begin IDLE or CHARM stage.
 */
 
static void dragon_begin_IDLE(struct fmn_sprite *sprite) {
  stage=DRAGON_STAGE_IDLE;
  clock=DRAGON_IDLE_TIME;
  animclock=DRAGON_BLINK_TIME_MIN+((rand()&0xffff)*(DRAGON_BLINK_TIME_MAX-DRAGON_BLINK_TIME_MIN))/65536.0f;
  sprite->tileid=DRAGON_TILE_IDLE;
}

static void dragon_begin_initial_state(struct fmn_sprite *sprite) {
  if (phase) {
    dragon_begin_FLOOM(sprite);
    clock-=(DRAGON_FLOOM_TIME-DRAGON_IDLE_TIME)*0.5f;
  } else {
    dragon_begin_IDLE(sprite);
  }
}

static void dragon_begin_CHARM(struct fmn_sprite *sprite) {
  if (stage!=DRAGON_STAGE_CHARM) {
    fmn_sound_effect(FMN_SFX_ENCHANT_ANIMAL);
    struct fmn_sprite *heart=fmn_sprite_generate_enchantment(sprite,1);
    if (heart) {
      heart->fv[5]=-0.5f;
      if (sprite->xform&FMN_XFORM_XREV) heart->fv[4]=1.0f;
      else heart->fv[4]=-1.0f;
      heart->x+=heart->fv[4];
      heart->y+=heart->fv[5];
    }
  }
  dragon_begin_IDLE(sprite);
  stage=DRAGON_STAGE_CHARM;
}

/* Begin SLEEP or PUMPKIN.
 */
 
static void dragon_begin_SLEEP(struct fmn_sprite *sprite) {
  stage=DRAGON_STAGE_SLEEP;
  sprite->tileid=DRAGON_TILE_BLINK;
  fmn_sprite_kill_enchantment(sprite);
}

static void dragon_begin_PUMPKIN(struct fmn_sprite *sprite) {
  stage=DRAGON_STAGE_PUMPKIN;
  sprite->tileid=DRAGON_TILE_PUMPKIN1;
  animclock=DRAGON_PUMPKIN_FRAME_TIME;
  fmn_sprite_kill_enchantment(sprite);
  fmn_sprite_kill_zzz(sprite);
}

/* Get charmed if the hero is tickling us.
 * We can't use the usual 'interact' method because this sprite is too big.
 */
 
static void dragon_check_charm(struct fmn_sprite *sprite) {
  if (fmn_global.active_item!=FMN_ITEM_FEATHER) return;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  switch (fmn_global.facedir) {
    case FMN_DIR_W: herox-=0.5f; break;
    case FMN_DIR_E: herox+=0.5f; break;
    case FMN_DIR_N: heroy-=0.5f; break;
    case FMN_DIR_S: heroy+=0.5f; break;
  }
  float hbt=sprite->y-1.5f;
  float hbb=sprite->y+0.5f;
  float hbl,hbr;
  if (sprite->xform&FMN_XFORM_XREV) {
    hbl=sprite->x-0.5f;
    hbr=sprite->x+1.5f;
  } else {
    hbl=sprite->x-1.5f;
    hbr=sprite->x+0.5f;
  }
  if (herox<hbl) return;
  if (herox>hbr) return;
  if (heroy<hbt) return;
  if (heroy>hbb) return;
  dragon_begin_CHARM(sprite);
}

/* Toast the hero if she's standing in our fire.
 */
 
static void dragon_check_fire(struct fmn_sprite *sprite) {
  float hbt=sprite->y-1.5f;
  float hbb=sprite->y+0.5f;
  float hbl,hbr;
  if (sprite->xform&FMN_XFORM_XREV) {
    hbl=sprite->x+0.5f;
    hbr=sprite->x+5.5f;
  } else {
    hbl=sprite->x-5.5f;
    hbr=sprite->x-0.5f;
  }
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (herox<hbl) return;
  if (herox>hbr) return;
  if (heroy<hbt) return;
  if (heroy>hbb) return;
  fmn_hero_injure(sprite->x,sprite->y,sprite);
}

/* Update, IDLE or CHARM.
 */
 
static void dragon_update_IDLE(struct fmn_sprite *sprite,float elapsed) {
  if ((clock-=elapsed)<=0.0f) {
    if (stage==DRAGON_STAGE_CHARM) dragon_begin_CHARM(sprite);
    else dragon_begin_FLOOM(sprite);
    return;
  }
  if ((animclock-=elapsed)<=0.0f) {
    animclock=DRAGON_BLINK_TIME_MIN+((rand()&0xffff)*(DRAGON_BLINK_TIME_MAX-DRAGON_BLINK_TIME_MIN))/65536.0f;
  }
  if (animclock<DRAGON_BLINK_DOWN_TIME) sprite->tileid=DRAGON_TILE_BLINK;
  else sprite->tileid=DRAGON_TILE_IDLE;
  if (stage==DRAGON_STAGE_IDLE) {
    dragon_check_charm(sprite);
  }
}

/* Update, FLOOM.
 */
 
static void dragon_update_FLOOM(struct fmn_sprite *sprite,float elapsed) {
  if ((clock-=elapsed)<=0.0f) {
    dragon_begin_IDLE(sprite);
    return;
  }
  if ((animclock-=elapsed)<=0.0f) {
    animclock=DRAGON_FLOOM_FRAME_TIME;
    if (sprite->tileid==DRAGON_TILE_FIRE1) sprite->tileid=DRAGON_TILE_FIRE2;
    else sprite->tileid=DRAGON_TILE_FIRE1;
  }
  dragon_check_fire(sprite);
  dragon_check_charm(sprite);
}

/* Update, PUMPKIN.
 */
 
static void dragon_update_PUMPKIN(struct fmn_sprite *sprite,float elapsed) {
  if ((animclock-=elapsed)<=0.0f) {
    animclock=DRAGON_PUMPKIN_FRAME_TIME;
    if (sprite->tileid==DRAGON_TILE_PUMPKIN1) sprite->tileid=DRAGON_TILE_PUMPKIN2;
    else sprite->tileid=DRAGON_TILE_PUMPKIN1;
  }
}

/* Update.
 */
 
static void _dragon_update(struct fmn_sprite *sprite,float elapsed) {
  switch (stage) {
    case DRAGON_STAGE_IDLE: dragon_update_IDLE(sprite,elapsed); break;
    case DRAGON_STAGE_FLOOM: dragon_update_FLOOM(sprite,elapsed); break;
    case DRAGON_STAGE_SLEEP: break;
    case DRAGON_STAGE_PUMPKIN: dragon_update_PUMPKIN(sprite,elapsed); break;
    case DRAGON_STAGE_CHARM: dragon_update_IDLE(sprite,elapsed); break;
  }
}

/* Sleep.
 */
 
static void dragon_sleep(struct fmn_sprite *sprite,int sleep) {
  if (stage==DRAGON_STAGE_PUMPKIN) return;
  if (sleep) {
    if (stage==DRAGON_STAGE_SLEEP) return;
    dragon_begin_SLEEP(sprite);
    struct fmn_sprite *zzz=fmn_sprite_generate_zzz(sprite);
    if (zzz) {
      zzz->fv[4]=-0.5f;
      if (sprite->xform&FMN_XFORM_XREV) zzz->fv[3]=1.0f;
      else zzz->fv[3]=-1.0f;
      zzz->x+=zzz->fv[3];
      zzz->y+=zzz->fv[4];
    }
  } else {
    if (stage!=DRAGON_STAGE_SLEEP) return;
    dragon_begin_initial_state(sprite);
  }
}

/* Pumpkin.
 */
 
static void dragon_toggle_pumpkin(struct fmn_sprite *sprite) {
  if (stage==DRAGON_STAGE_PUMPKIN) dragon_begin_initial_state(sprite);
  else dragon_begin_PUMPKIN(sprite);
}

/* Interact.
 */
 
static int16_t _dragon_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: dragon_sleep(sprite,1); break;
        case FMN_SPELLID_REVEILLE: dragon_sleep(sprite,0); break;
        case FMN_SPELLID_PUMPKIN: dragon_toggle_pumpkin(sprite); break;
      } break;
    case FMN_ITEM_BELL: dragon_sleep(sprite,0); break;
  }
  return 0;
}

/* Init.
 */
 
static void _dragon_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  sprite->tileid=DRAGON_TILE_IDLE;
  if (dir==FMN_DIR_E) sprite->xform=FMN_XFORM_XREV;
  else dir=0;
  dragon_begin_initial_state(sprite);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_dragon={
  .init=_dragon_init,
  .interact=_dragon_interact,
  .update=_dragon_update,
};

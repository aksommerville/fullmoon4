#include "app/fmn_platform.h"
#include "app/fmn_game.h"
#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define WATCHDUCK_SPEAK_RANGE 2.0f
#define WATCHDUCK_WALK_SPEED 2.0f

#define needitem sprite->argv[0]

#define tileid0 sprite->bv[0]
#define dialogueid sprite->bv[1] /* bv[1] in our dialogue sprites */

/* Speech bubble.
 */
 
struct watchduck_destroy_context {
  struct fmn_sprite *sprite;
  struct fmn_sprite *dialogue;
};
 
static int watchduck_destroy_speech_bubble_cb(struct fmn_sprite *q,void *userdata) {
  struct watchduck_destroy_context *ctx=userdata;
  struct fmn_sprite *sprite=ctx->sprite;
  if (q->layer<150) return 0;
  if (q->imageid!=sprite->imageid) return 0;
  if (q->bv[0]!=0xdc) return 0;
  if (q->bv[1]!=dialogueid) return 0;
  ctx->dialogue=q;
  return 1;
}
 
static void watchduck_destroy_speech_bubble(struct fmn_sprite *sprite) {
  // We have to restart iteration for each bubble bit we find; deleting them can change the addresses.
  struct watchduck_destroy_context ctx={.sprite=sprite};
  while (fmn_sprites_for_each(watchduck_destroy_speech_bubble_cb,&ctx)) fmn_sprite_kill(ctx.dialogue);
  dialogueid=0;
}
 
static void watchduck_create_speech_bubble(struct fmn_sprite *sprite) {
  dialogueid=(((uintptr_t)sprite)>>4)|1;
  struct fmn_sprite *bubblebit;
  #define DLGTILE(t,dx,dy,l) \
    if (bubblebit=fmn_sprite_generate_noparam(0,sprite->x+dx,sprite->y+dy)) { \
      bubblebit->imageid=sprite->imageid; \
      bubblebit->tileid=t; \
      bubblebit->style=FMN_SPRITE_STYLE_TILE; \
      bubblebit->bv[0]=0xdc; \
      bubblebit->bv[1]=dialogueid; \
      bubblebit->layer=150+l; \
    } else { \
      watchduck_destroy_speech_bubble(sprite); \
      return; \
    }
  DLGTILE(0x90,-0.5f,-2.0f,0)
  DLGTILE(0x91, 0.5f,-2.0f,0)
  DLGTILE(0xa0,-0.5f,-1.0f,0)
  DLGTILE(0xa1, 0.5f,-1.0f,0)
  DLGTILE(0xf0+needitem,0.0f,-1.60f,1)
  #undef DLGTILE
}

static int watchduck_posbub_cb(struct fmn_sprite *q,void *userdata) {
  struct fmn_sprite *sprite=userdata;
  if (q->layer<150) return 0;
  if (q->imageid!=sprite->imageid) return 0;
  if (q->bv[0]!=0xdc) return 0;
  if (q->bv[1]!=dialogueid) return 0;
  switch (q->tileid) {
    case 0x90: q->x=sprite->x-0.5f; q->y=sprite->y-2.0f; break;
    case 0x91: q->x=sprite->x+0.5f; q->y=sprite->y-2.0f; break;
    case 0xa0: q->x=sprite->x-0.5f; q->y=sprite->y-1.0f; break;
    case 0xa1: q->x=sprite->x+0.5f; q->y=sprite->y-1.0f; break;
    default: q->x=sprite->x; q->y=sprite->y-1.60f; break;
  }
  return 0;
}

static void watchduck_position_speech_bubble(struct fmn_sprite *sprite) {
  fmn_sprites_for_each(watchduck_posbub_cb,sprite);
}

/* Speaking.
 */
 
static void watchduck_update_speech(struct fmn_sprite *sprite,float elapsed) {
  if (dialogueid) {
    watchduck_position_speech_bubble(sprite);
  } else {
    watchduck_create_speech_bubble(sprite);
  }
  sprite->tileid=tileid0+2; // 2,3
  sprite->style=FMN_SPRITE_STYLE_TWOFRAME;
}

/* Walking.
 */
 
static void watchduck_update_walk(struct fmn_sprite *sprite,float elapsed,float herox) {
  if (dialogueid) {
    watchduck_destroy_speech_bubble(sprite);
  }
  sprite->tileid=tileid0+0;
  const float speedlimit=WATCHDUCK_WALK_SPEED*elapsed;
  float dx=herox-sprite->x;
  if (dx>0.0f) {
    sprite->style=FMN_SPRITE_STYLE_TWOFRAME;
    sprite->xform|=FMN_XFORM_XREV;
    if (dx>speedlimit) sprite->x+=speedlimit;
    else sprite->x+=dx;
  } else if (dx<0.0f) {
    sprite->style=FMN_SPRITE_STYLE_TWOFRAME;
    sprite->xform&=~FMN_XFORM_XREV;
    if (dx<-speedlimit) sprite->x-=speedlimit;
    else sprite->x+=dx;
  } else {
    sprite->style=FMN_SPRITE_STYLE_TILE;
  }
}

/* Update.
 */
 
static void _watchduck_update(struct fmn_sprite *sprite,float elapsed) {
  //TODO check sleep
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if ((heroy>sprite->y)&&(heroy<sprite->y+WATCHDUCK_SPEAK_RANGE)) {
    watchduck_update_speech(sprite,elapsed);
  } else {
    watchduck_update_walk(sprite,elapsed,herox);
  }
}

/* Interact.
 */
 
static int16_t _watchduck_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: break;
        case FMN_SPELLID_REVEILLE: break;
      } break;
    case FMN_ITEM_BELL: break;
  }
  return 0;
}

/* Init.
 */
 
static void _watchduck_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  
  // If hero already has the item, mission accomplished, we can disappear.
  if (needitem>=FMN_ITEM_COUNT) { fmn_sprite_kill(sprite); return; }
  if (fmn_global.itemv[needitem]) { // have the item...
    if (fmn_item_default_quantities[needitem]&&!fmn_global.itemqv[needitem]) ; // quantity zero, ok we actually don't have it
    else { fmn_sprite_kill(sprite); return; }
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_watchduck={
  .init=_watchduck_init,
  .update=_watchduck_update,
  .interact=_watchduck_interact,
};

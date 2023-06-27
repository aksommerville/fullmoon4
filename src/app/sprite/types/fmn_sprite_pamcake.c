#include "app/sprite/fmn_sprite.h"

#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define stage sprite->bv[2]
#define physics0 sprite->bv[3]
#define animclock sprite->fv[0]
#define doorx sprite->fv[1]
#define doory sprite->fv[2]
#define treex sprite->fv[3]
#define treey sprite->fv[4]
#define mode sprite->argv[0]

#define PAMCAKE_MODE_HARVEST 0
#define PAMCAKE_MODE_FEAST 1

#define PAMCAKE_STAGE_BIDE 1 /* HARVEST... */
#define PAMCAKE_STAGE_TO_TREE 2
#define PAMCAKE_STAGE_REAP 3
#define PAMCAKE_STAGE_TO_PUB 4
#define PAMCAKE_STAGE_REACH 5 /* FEAST... */
#define PAMCAKE_STAGE_CHEW 6

#define PAMCAKE_WALK_SPEED 1.0f

/* Sleep/wake.
 */
 
static void pamcake_sleep(struct fmn_sprite *sprite,uint8_t sleep) {
  if (sleep) {
    if (sleeping) return;
    if (stage==PAMCAKE_STAGE_BIDE) return;
    sleeping=1;
    fmn_sprite_generate_zzz(sprite);
  } else {
    if (!sleeping) return;
    sleeping=0;
    animclock=999.0f;
  }
}

/* Walk. Returns nonzero if continuing, or zero if we're there.
 * Designer must ensure that the tree and door points are reachable by an L path.
 */
 
static uint8_t pamcake_walk(struct fmn_sprite *sprite,float dstx,float dsty,float elapsed) {
  uint8_t result=0;
  if (dstx<sprite->x) {
    sprite->xform&=~FMN_XFORM_XREV;
    if ((sprite->x-=PAMCAKE_WALK_SPEED*elapsed)<=dstx) {
      sprite->x=dstx;
    } else result=1;
  } else if (dstx>sprite->x) {
    sprite->xform|=FMN_XFORM_XREV;
    if ((sprite->x+=PAMCAKE_WALK_SPEED*elapsed)>=dstx) {
      sprite->x=dstx;
    } else result=1;
  }
  if (dsty<sprite->y) {
    if ((sprite->y-=PAMCAKE_WALK_SPEED*elapsed)<=dsty) {
      sprite->y=dsty;
    } else result=1;
  } else if (dsty>sprite->y) {
    if ((sprite->y+=PAMCAKE_WALK_SPEED*elapsed)>=dsty) {
      sprite->y=dsty;
    } else result=1;
  }
  return result;
}

/* Update.
 */
 
static void _pamcake_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    sprite->tileid=tileid0+((mode==PAMCAKE_MODE_HARVEST)?0x04:0x13);
    animclock=0.0f;
    return;
  }
  animclock+=elapsed;
  switch (stage) {
    case PAMCAKE_STAGE_BIDE: {
        if (animclock>=4.0f) {
          animclock=0.0f;
          stage=PAMCAKE_STAGE_TO_TREE;
          sprite->style=FMN_SPRITE_STYLE_TILE;
          sprite->physics=physics0;
        }
      } break;
    case PAMCAKE_STAGE_TO_TREE: {
        if (animclock>=0.5f) animclock=0.0f;
        else if (animclock>=0.25f) sprite->tileid=tileid0+0x01;
        else sprite->tileid=tileid0;
        if (!pamcake_walk(sprite,treex,treey,elapsed)) {
          animclock=0.0f;
          stage=PAMCAKE_STAGE_REAP;
          //TODO Sound effect and pitcher content indicator?
        }
      } break;
    case PAMCAKE_STAGE_REAP: {
        if (animclock>=2.0f) {
          stage=PAMCAKE_STAGE_TO_PUB;
          animclock=0.0f;
        } else if (((int)(animclock*4.0f))&1) sprite->tileid=tileid0+0x03;
        else sprite->tileid=tileid0+0x02;
      } break;
    case PAMCAKE_STAGE_TO_PUB: {
        if (animclock>=0.5f) animclock=0.0f;
        else if (animclock>=0.25f) sprite->tileid=tileid0+0x01;
        else sprite->tileid=tileid0;
        if (!pamcake_walk(sprite,doorx,doory,elapsed)) {
          animclock=0.0f;
          stage=PAMCAKE_STAGE_BIDE;
          sprite->style=FMN_SPRITE_STYLE_HIDDEN;
          sprite->physics=0;
        }
      } break;
    case PAMCAKE_STAGE_REACH: {
        sprite->tileid=tileid0+0x10;
        if (animclock>=0.70f) {
          stage=PAMCAKE_STAGE_CHEW;
          animclock=0.0f;
        }
      } break;
    case PAMCAKE_STAGE_CHEW: {
        sprite->tileid=tileid0+((((int)(animclock*6.0f))&1)?0x11:0x12);
        if (animclock>=2.0f) {
          stage=PAMCAKE_STAGE_REACH;
          animclock=0.0f;
        }
      } break;
  }
}

/* Interact.
 */
 
static int16_t _pamcake_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: pamcake_sleep(sprite,1); break;
        case FMN_SPELLID_REVEILLE: pamcake_sleep(sprite,0); break;
        case FMN_SPELLID_PUMPKIN: fmn_sprite_pumpkinize(sprite); break;
      } break;
    case FMN_ITEM_BELL: pamcake_sleep(sprite,0); break;
  }
  return 0;
}

/* Find control points.
 * Whereever she starts initially is the tree.
 * Nearest door is the one we'll use.
 */
 
static void pamcake_find_control_points(struct fmn_sprite *sprite) {
  treex=sprite->x;
  treey=sprite->y;
  doorx=sprite->x;
  doory=sprite->y;
  float bestdistance=999.0f;
  const struct fmn_door *door=fmn_global.doorv;
  int i=fmn_global.doorc;
  for (;i-->0;door++) {
    if (!door->mapid||door->extra) continue; // plain old doors only
    float x=door->x+0.5f,y=door->y+0.5f;
    float dx=x-sprite->x; if (dx<0.0f) dx=-dx;
    float dy=y-sprite->y; if (dy<0.0f) dy=-dy;
    float distance=dx+dy;
    if (distance>=bestdistance) continue;
    doorx=x;
    doory=y;
    bestdistance=distance;
  }
}

/* Init.
 */
 
static void _pamcake_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  physics0=sprite->physics;
  switch (mode) {
    case PAMCAKE_MODE_HARVEST: {
        sprite->physics=0;
        sprite->style=FMN_SPRITE_STYLE_HIDDEN;
        pamcake_find_control_points(sprite);
        sprite->x=doorx;
        sprite->y=doory;
        stage=PAMCAKE_STAGE_BIDE;
      } break;
    case PAMCAKE_MODE_FEAST: {
        sprite->tileid=tileid0+0x10;
        sprite->x+=6.0f/16.0f;
        sprite->y-=2.0f/16.0f;
        sprite->physics&=~FMN_PHYSICS_SOLID; // she overlaps the table, it's fine.
        stage=PAMCAKE_STAGE_REACH;
      } break;
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_pamcake={
  .init=_pamcake_init,
  .update=_pamcake_update,
  .interact=_pamcake_interact,
};

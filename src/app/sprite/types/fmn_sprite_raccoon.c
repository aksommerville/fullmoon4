#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define RACCOON_STAGE_CHOOSE_DESTINATION 0
#define RACCOON_STAGE_TRAVEL 1
#define RACCOON_STAGE_WAIT 2
#define RACCOON_STAGE_BRANDISH 3
#define RACCOON_STAGE_TOSS 4 /* throw the acorn on stage start, and show arm horizontally while active */
#define RACCOON_STAGE_LOST 5 /* emergency stage, if we get pushed onto non-VACANT tiles */

#define RACCOON_WALK_SPEED 2.0f
#define RACCOON_PANIC_TIME 5.0f

#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define enchanted sprite->bv[2]
#define stage sprite->bv[3]
#define animclock sprite->fv[0]
#define dstx sprite->fv[1]
#define dsty sprite->fv[2]
#define panic_clock sprite->fv[3]

// Keep (w,e) symmetric, or account for xform at RACCOON_SET_HITBOX
#define RACCOON_HBW_UPRIGHT 0.5f
#define RACCOON_HBE_UPRIGHT 0.5f
#define RACCOON_HBN_UPRIGHT 0.5f
#define RACCOON_HBS_UPRIGHT 0.5f
#define RACCOON_HBW_SLEEP 0.5f
#define RACCOON_HBE_SLEEP 0.5f
#define RACCOON_HBN_SLEEP 0.0f
#define RACCOON_HBS_SLEEP 0.5f

#define RACCOON_SET_HITBOX(tag) { \
  sprite->hbw=RACCOON_HBW_##tag; \
  sprite->hbe=RACCOON_HBE_##tag; \
  sprite->hbn=RACCOON_HBN_##tag; \
  sprite->hbs=RACCOON_HBS_##tag; \
}

/* Init.
 */
 
static void _raccoon_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  stage=RACCOON_STAGE_CHOOSE_DESTINATION;
  sprite->radius=0.0f;
  RACCOON_SET_HITBOX(UPRIGHT)
}

/* Choose destination and enter TRAVEL stage.
 */
 
static void raccoon_choose_destination(struct fmn_sprite *sprite) {
  stage=RACCOON_STAGE_LOST;
  animclock=0.0f;
  panic_clock=0.0f;
  
  // We are not allowed offscreen. If that happened, all bets are off. Kill the sprite.
  int8_t col=(int8_t)sprite->x;
  int8_t row=(int8_t)sprite->y;
  if ((col<0)||(row<0)||(col>=FMN_COLC)||(row>=FMN_ROWC)) {
    fmn_sprite_kill(sprite);
    return;
  }
  
  // The cell we're on, and the path we follow, must be VACANT (not even UNSHOVELLABLE).
  // If our current cell is non-VACANT, or there is no VACANT neighbor, enter the panic "LOST" stage.
  #define WALKABLE(x,y) (fmn_global.cellphysics[fmn_global.map[(y)*FMN_COLC+(x)]]==FMN_CELLPHYSICS_VACANT)
  if (!WALKABLE(col,row)) return;
  uint8_t wc=0; while ((wc<col)&&WALKABLE(col-wc-1,row)) wc++;
  uint8_t nc=0; while ((nc<row)&&WALKABLE(col,row-nc-1)) nc++;
  uint8_t ec=0; while ((col+ec+1<FMN_COLC)&&WALKABLE(col+ec+1,row)) ec++;
  uint8_t sc=0; while ((row+sc+1<FMN_ROWC)&&WALKABLE(col,row+sc+1)) sc++;
  #undef WALKABLE
  uint8_t dirc=(wc?1:0)+(nc?1:0)+(ec?1:0)+(sc?1:0);
  if (!dirc) return;
  
  // When enchanted, if the hero is wagging her feather at the decision moment, go in the direction she's pointing.
  if (enchanted) {
    if (fmn_global.active_item==FMN_ITEM_FEATHER) {
      switch (fmn_global.facedir) {
        case FMN_DIR_W: ec=nc=sc=0; if (!wc) wc=1; break;
        case FMN_DIR_E: wc=nc=sc=0; if (!ec) ec=1; break;
        case FMN_DIR_N: wc=ec=sc=0; if (!nc) nc=1; break;
        case FMN_DIR_S: wc=nc=ec=0; if (!sc) sc=1; break;
      }
    }
  }
  
  // Pick an available direction at random, and walk a random distance along it.
  dstx=sprite->x;
  dsty=sprite->y;
  uint8_t dirp=rand()%dirc;
       if (wc&&!dirp--) dstx=sprite->x-(1+rand()%wc);
  else if (nc&&!dirp--) dsty=sprite->y-(1+rand()%nc);
  else if (ec&&!dirp--) dstx=sprite->x+(1+rand()%ec);
  else if (sc&&!dirp--) dsty=sprite->y+(1+rand()%sc);
  else return;
  
  stage=RACCOON_STAGE_TRAVEL;
  sprite->tileid=tileid0;
}

/* Move toward the destination.
 */
 
static void raccoon_update_TRAVEL(struct fmn_sprite *sprite,float elapsed) {

  // We don't account for solid sprites in choosing a destination, so we get stuck a lot.
  // It's not a big deal. Just, if we're walking for too long, pretend we made it.
  panic_clock+=elapsed;
  if (panic_clock>RACCOON_PANIC_TIME) {
    stage=RACCOON_STAGE_WAIT;
    animclock=0.0f;
    return;
  }

  animclock+=elapsed;
  if (animclock>=1.0f) animclock-=1.0f;
  uint8_t animframe=0;
  switch ((int)(animclock*4.0f)) {
    case 0: animframe=0; break;
    case 1: animframe=1; break;
    case 2: animframe=0; break;
    case 3: animframe=2; break;
  }
  sprite->tileid=tileid0+animframe;
  
  if (dstx<sprite->x) {
    if ((sprite->x-=RACCOON_WALK_SPEED*elapsed)<dstx) sprite->x=dstx;
  } else if (dstx>sprite->x) {
    if ((sprite->x+=RACCOON_WALK_SPEED*elapsed)>dstx) sprite->x=dstx;
  } else if (dsty<sprite->y) {
    if ((sprite->y-=RACCOON_WALK_SPEED*elapsed)<dsty) sprite->y=dsty;
  } else if (dsty>sprite->y) {
    if ((sprite->y+=RACCOON_WALK_SPEED*elapsed)>dsty) sprite->y=dsty;
    
  } else {
    stage=RACCOON_STAGE_WAIT;
    animclock=0.0f;
  }
}

/* Update, WAIT. Stall until the clock expires, then pick an acorn and BRANDISH it.
 */
 
static void raccoon_update_WAIT(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=tileid0;
  animclock+=elapsed;
  if (animclock>=1.5f) {
    // If enchanted, we don't throw things at our true love the witch. Just start walking again.
    // Same deal if she's invisible, albeit not due to good intentions.
    if (enchanted||(fmn_global.invisibility_time>0.0f)) {
      stage=RACCOON_STAGE_CHOOSE_DESTINATION;
      animclock=0.0f;
    } else {
      stage=RACCOON_STAGE_BRANDISH;
      animclock=0.0f;
      struct fmn_sprite *acorn=fmn_sprite_generate_noparam(FMN_SPRCTL_missile,sprite->x,sprite->y);
      if (acorn) {
        acorn->imageid=sprite->imageid;
        acorn->tileid=0x61;
        acorn->pv[0]=sprite;
      }
    }
  }
}

/* Update, BRANDISH.
 */
 
struct raccoon_find_acorn_context {
  struct fmn_sprite *raccoon;
  struct fmn_sprite *acorn;
};

static int raccoon_find_acorn_1(struct fmn_sprite *sprite,void *userdata) {
  struct raccoon_find_acorn_context *ctx=userdata;
  if (sprite->controller!=FMN_SPRCTL_missile) return 0;
  if (sprite->imageid!=ctx->raccoon->imageid) return 0;
  if (sprite->tileid!=0x61) return 0;
  if (sprite->pv[0]!=ctx->raccoon) return 0;
  ctx->acorn=sprite;
  return 1;
}
 
static struct fmn_sprite *raccoon_find_acorn(struct fmn_sprite *sprite) {
  struct raccoon_find_acorn_context ctx={.raccoon=sprite,.acorn=0,};
  fmn_sprites_for_each(raccoon_find_acorn_1,&ctx);
  return ctx.acorn;
}
 
static void raccoon_update_BRANDISH(struct fmn_sprite *sprite,float elapsed) {
  animclock+=elapsed;
  if (animclock>=1.0f) {
    stage=RACCOON_STAGE_TOSS;
    animclock=0.0f;
    struct fmn_sprite *acorn=raccoon_find_acorn(sprite);
    if (acorn) {
      if (enchanted||(fmn_global.invisibility_time>0.0f)) {
        fmn_sprite_kill(acorn);
        stage=RACCOON_STAGE_CHOOSE_DESTINATION;
        return;
      }
      acorn->pv[0]=0; // unset acorn's owner; it figures out the rest.
    }
  } else {
    sprite->tileid=tileid0+3;
  }
}

/* Update, TOSS. Just set the tile and track time.
 */
 
static void raccoon_update_TOSS(struct fmn_sprite *sprite,float elapsed) {
  animclock+=elapsed;
  if (animclock>=1.0f) {
    stage=RACCOON_STAGE_CHOOSE_DESTINATION;
    animclock=0.0f;
  } else {
    sprite->tileid=tileid0+4;
  }
}

/* Update, LOST. Stand around like a doofus and re-check every few seconds.
 */
 
static void raccoon_update_LOST(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=tileid0;
  animclock+=elapsed;
  if (animclock>4.0f) {
    stage=RACCOON_STAGE_CHOOSE_DESTINATION;
    animclock=0.0f;
  }
}

/* Look for missiles we can catch.
 */
 
static int raccoon_check_catch_1(struct fmn_sprite *missile,void *userdata) {
  struct fmn_sprite *sprite=userdata;
  if (missile->controller!=FMN_SPRCTL_missile) return 0;
  if (!missile->bv[1]) return 0; // reflected
  if (missile->pv[0]) return 0; // holder
  const float radius=0.75f;
  float dx=missile->x-sprite->x;
  if ((dx<-radius)||(dx>radius)) return 0;
  float dy=missile->y-sprite->y;
  if ((dy<-radius)||(dy>radius)) return 0;
  
  missile->pv[0]=sprite;
  missile->bv[1]=0;
  missile->bv[0]=0; // ready
  stage=RACCOON_STAGE_BRANDISH;
  animclock=0.0f;
  return 1;
}
 
static void raccoon_check_catch(struct fmn_sprite *sprite) {
  fmn_sprites_for_each(raccoon_check_catch_1,sprite);
}

/* Update.
 */
 
static void _raccoon_update(struct fmn_sprite *sprite,float elapsed) {

  if (sleeping) {
    sprite->tileid=tileid0+5;
    animclock=0.0f;
    return;
  }
  
  // Always face the hero, if we're awake.
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (herox<sprite->x) sprite->xform=0;
  else sprite->xform=FMN_XFORM_XREV;
  
  // In any stage except BRANDISH, try to catch flying acorns.
  if (stage!=RACCOON_STAGE_BRANDISH) {
    raccoon_check_catch(sprite);
  }

  switch (stage) {
    case RACCOON_STAGE_CHOOSE_DESTINATION: raccoon_choose_destination(sprite); break;
    case RACCOON_STAGE_TRAVEL: raccoon_update_TRAVEL(sprite,elapsed); break;
    case RACCOON_STAGE_WAIT: raccoon_update_WAIT(sprite,elapsed); break;
    case RACCOON_STAGE_BRANDISH: raccoon_update_BRANDISH(sprite,elapsed); break;
    case RACCOON_STAGE_TOSS: raccoon_update_TOSS(sprite,elapsed); break;
    case RACCOON_STAGE_LOST: raccoon_update_LOST(sprite,elapsed); break;
  }
}

/* Fall asleep.
 */
 
static void raccoon_sleep(struct fmn_sprite *sprite) {
  if (sleeping) return;
  RACCOON_SET_HITBOX(SLEEP)
  sleeping=1;
  fmn_sprite_generate_zzz(sprite);
  stage=RACCOON_STAGE_CHOOSE_DESTINATION;
  animclock=0.0f;
  struct fmn_sprite *acorn=raccoon_find_acorn(sprite);
  if (acorn) fmn_sprite_kill(acorn);
}

/* Interact.
 */
 
static int16_t _raccoon_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: raccoon_sleep(sprite); break;
        case FMN_SPELLID_REVEILLE: RACCOON_SET_HITBOX(UPRIGHT) sleeping=0; break;
      } break;
    case FMN_ITEM_BELL: RACCOON_SET_HITBOX(UPRIGHT) sleeping=0; break;
    case FMN_ITEM_FEATHER: if (!enchanted&&!sleeping) {
        enchanted=1;
        fmn_sound_effect(FMN_SFX_ENCHANT_ANIMAL);
      } break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_raccoon={
  .init=_raccoon_init,
  .update=_raccoon_update,
  .interact=_raccoon_interact,
};

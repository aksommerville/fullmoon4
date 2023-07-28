#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define pumpkin sprite->bv[2]
#define animframe sprite->bv[3]
#define ejected sprite->bv[4]
#define swimtime sprite->fv[0]
#define animclock sprite->fv[1]
#define dx sprite->fv[2]
#define limitlo sprite->fv[3]
#define limithi sprite->fv[4]

#define NFFISH_SWIM_SPEED 3.0f

/* Begin swimming.
 */
 
static void nffish_begin_swim(struct fmn_sprite *sprite) {
  swimtime=1.0f+((rand()&0xffff)*2.0f)/65536.0f;
  if (sprite->x<limitlo+1.0f) dx=1.0f;
  else if (sprite->x>limithi-1.0f) dx=-1.0f;
  else dx=(rand()&1)?1.0f:-1.0f;
  sprite->tileid=tileid0;
  animclock=0.5f;
  animframe=0;
}

/* Begin attack.
 */
 
static void nffish_begin_attack(struct fmn_sprite *sprite) {
  animclock=0.0f;
  sprite->tileid=tileid0+(pumpkin?2:1);
  ejected=0;
}

/* Generate fireball and send it on its way.
 */
 
static void nffish_generate_fireball(struct fmn_sprite *sprite) {
  struct fmn_sprite *missile=fmn_sprite_generate_noparam(FMN_SPRCTL_missile,sprite->x,sprite->y+0.25f);
  if (missile) {
    missile->imageid=3;
    missile->tileid=0xd7;
    missile->style=FMN_SPRITE_STYLE_TWOFRAME;
  }
}

/* Update.
 */
 
static void _nffish_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    if ((animclock-=elapsed)<=0.0f) {
      animclock=0.5f;
      animframe^=1;
      sprite->tileid=tileid0+(animframe?0x10:0);
    }
    return;
  }
  if (swimtime>0.0f) {
    sprite->x+=NFFISH_SWIM_SPEED*dx*elapsed;
    if (sprite->x<limitlo) { sprite->x=limitlo; swimtime=0.0f; }
    else if (sprite->x>limithi) { sprite->x=limithi; swimtime=0.0f; }
    if ((swimtime-=elapsed)<=0.0f) {
      nffish_begin_attack(sprite);
    } else {
      if ((animclock-=elapsed)<=0.0f) {
        animclock=0.5f;
        animframe^=1;
        sprite->tileid=tileid0+(animframe?0x10:0);
      }
    }
  } else {
    animclock+=elapsed;
    if (ejected) {
      if (animclock>=1.0f) {
        nffish_begin_swim(sprite);
      }
    } else if (animclock>=0.5f) {
      ejected=1;
      sprite->tileid+=0x10;
      nffish_generate_fireball(sprite);
    }
  }
}

/* Sleep.
 */
 
static void nffish_sleep(struct fmn_sprite *sprite,uint8_t sleep) {
  if (sleep) {
    if (sleeping) return;
    sleeping=1;
    animclock=0.0f;
    fmn_sprite_generate_zzz(sprite);
  } else {
    if (!sleeping) return;
    sleeping=0;
    nffish_begin_swim(sprite);
  }
}

/* Pumpkin.
 */
 
static void nffish_toggle_pumpkin(struct fmn_sprite *sprite) {
  if (pumpkin) {
    pumpkin=0;
  } else {
    pumpkin=1;
  }
}

/* Interact.
 */
 
static int16_t _nffish_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: nffish_sleep(sprite,1); break;
        case FMN_SPELLID_REVEILLE: nffish_sleep(sprite,0); break;
        case FMN_SPELLID_PUMPKIN: nffish_toggle_pumpkin(sprite); break;
      } break;
    case FMN_ITEM_BELL: nffish_sleep(sprite,0); break;
  }
  return 0;
}

/* Determine how far left and right we can swim.
 */
 
static void nffish_detect_limits(struct fmn_sprite *sprite) {
  int8_t row=(int8_t)sprite->y;
  if (row<0) row=0; else if (row>=FMN_ROWC) row=FMN_ROWC-1;
  #define CELLOK(x,y) (fmn_global.cellphysics[fmn_global.map[(y)*FMN_COLC+(x)]]==FMN_CELLPHYSICS_WATER)
  int8_t col=(int8_t)sprite->x;
  if (col<FMN_COLC) {
    while ((col>0)&&CELLOK(col-1,row)) col--;
    limitlo=col+0.5f;
  } else limitlo=0.5f;
  col=(int8_t)sprite->x;
  if (col>=0) {
    while ((col<FMN_COLC-1)&&CELLOK(col+1,row)) col++;
    limithi=col+0.5f;
  } else limithi=FMN_COLC-0.5f;
  #undef CELLOK
}

/* Init.
 */
 
static void _nffish_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  nffish_detect_limits(sprite);
  nffish_begin_swim(sprite);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_nffish={
  .init=_nffish_init,
  .interact=_nffish_interact,
  .update=_nffish_update,
};

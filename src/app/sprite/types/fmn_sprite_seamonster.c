#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define SEAMONSTER_STAGE_LURK 1
#define SEAMONSTER_STAGE_SWIM 2
#define SEAMONSTER_STAGE_SURFACE 3
#define SEAMONSTER_STAGE_SPIT 4

#define SEAMONSTER_LURK_TIME 1.0f
#define SEAMONSTER_SWIM_SPEED 2.0f
#define SEAMONSTER_SPIT_TIME 0.5f
#define SEAMONSTER_SURFACE_TIME 1.0f

#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define stage sprite->bv[2]
#define clock sprite->fv[0]
#define swimdx sprite->fv[1]
#define swimdy sprite->fv[2]

/* Choose swim destination.
 * Sets (clock,swimdx,swimdy).
 */
 
struct maprect {
  uint8_t x,y,w,h;
};

static uint8_t maprect_available(uint8_t x,uint8_t y,uint8_t w,uint8_t h) {
  const uint8_t *srcrow=fmn_global.map+y*FMN_COLC+x;
  for (;h-->0;srcrow+=FMN_COLC) {
    const uint8_t *srcp=srcrow;
    uint8_t xi=w;
    for (;xi-->0;srcp++) {
      switch (fmn_global.cellphysics[*srcp]) {
        // Arguably HOLE too. (we're not using that yet, i'm not sure how it will play out)
        case FMN_CELLPHYSICS_WATER:
          break;
        default: return 0;
      }
    }
  }
  return 1;
}

static void maprect_grow(struct maprect *mr,int8_t dx,int8_t dy) {
  if (dx<0) {
    while ((mr->x>0)&&maprect_available(mr->x-1,mr->y,1,mr->h)) { mr->x--; mr->w++; }
  } else if (dy<0) {
    while ((mr->y>0)&&maprect_available(mr->x,mr->y-1,mr->w,1)) { mr->y--; mr->h++; }
  } else if (dx>0) {
    while ((mr->x+mr->w<FMN_COLC)&&maprect_available(mr->x+mr->w,mr->y,1,mr->h)) mr->w++;
  } else if (dy>0) {
    while ((mr->y+mr->h<FMN_ROWC)&&maprect_available(mr->x,mr->y+mr->h,mr->w,1)) mr->h++;
  }
}

static void maprect_fill_bitmap(uint8_t *bitmap,const struct maprect *mr) {
  uint8_t *dstrow=bitmap+mr->y*FMN_COLC+mr->x;
  uint8_t yi=mr->h;
  for (;yi-->0;dstrow+=FMN_COLC) {
    memset(dstrow,1,mr->w);
  }
}
 
static void seamonster_choose_swim_destination(struct fmn_sprite *sprite) {

  // Prepare four rectangles in discrete grid space, all initially pointing to my cell.
  int8_t col=sprite->x,row=sprite->y;
  if ((col<0)||(row<0)||(col>=FMN_COLC)||(row>=FMN_ROWC)) {
    clock=swimdx=swimdy=0.0f;
    return;
  }
  struct maprect availw={col,row,1,1};
  struct maprect availe={col,row,1,1};
  struct maprect availn={col,row,1,1};
  struct maprect avails={col,row,1,1};
  
  // Grow each rectangle in its major direction, then in each of the perpendicular directions.
  // There can easily be reachable cells that we miss, but we should get a reasonable set of definitely-reachable ones.
  maprect_grow(&availw,-1, 0); maprect_grow(&availw, 0,-1); maprect_grow(&availw, 0, 1);
  maprect_grow(&availe, 1, 0); maprect_grow(&availe, 0,-1); maprect_grow(&availe, 0, 1);
  maprect_grow(&availn, 0,-1); maprect_grow(&availn,-1, 0); maprect_grow(&availn, 1, 0);
  maprect_grow(&avails, 0, 1); maprect_grow(&avails,-1, 0); maprect_grow(&avails, 1, 0);
  
  // The rectangles will likely have a lot of overlap around the current position.
  // I don't want that impacting our odds. (if anything, i'd want *further* cells to be more likely).
  // So now, flatten it into a bitmap of cells.
  uint8_t bitmap[FMN_COLC*FMN_ROWC]={0};
  maprect_fill_bitmap(bitmap,&availw);
  maprect_fill_bitmap(bitmap,&availe);
  maprect_fill_bitmap(bitmap,&availn);
  maprect_fill_bitmap(bitmap,&avails);
  bitmap[row*FMN_COLC+col]=0; // eliminate the one we're at, that wouldn't be an interesting voyage
  
  // Count the available cells, pick randomly in that range, and iterate them again to get the coordinates.
  uint8_t candidatec=0;
  { const uint8_t *v=bitmap;
    uint8_t i=FMN_COLC*FMN_ROWC;
    for (;i-->0;v++) if (*v) candidatec++;
  }
  if (!candidatec) {
    clock=swimdx=swimdy=0.0f;
    return;
  }
  uint8_t choice=rand()%candidatec;
  uint8_t x=0,y=0,ok=0;
  { const uint8_t *v=bitmap;
    for (;y<FMN_ROWC;y++) {
      for (x=0;x<FMN_COLC;x++,v++) {
        if (!*v) continue;
        if (!choice--) { ok=1; break; }
      }
      if (ok) break;
    }
  }
  if (!ok) {
    clock=swimdx=swimdy=0.0f;
    return;
  }
  
  float dstx=x+0.5f;
  float dsty=y+0.5f;
  swimdx=dstx-sprite->x;
  swimdy=dsty-sprite->y;
  float distance=sqrtf(swimdx*swimdx+swimdy*swimdy);
  swimdx=(swimdx*SEAMONSTER_SWIM_SPEED)/distance;
  swimdy=(swimdy*SEAMONSTER_SWIM_SPEED)/distance;
  clock=distance/SEAMONSTER_SWIM_SPEED;
}

/* Stage changes.
 */
 
static void seamonster_begin_LURK(struct fmn_sprite *sprite) {
  sprite->style=FMN_SPRITE_STYLE_TWOFRAME;
  sprite->tileid=tileid0;
  stage=SEAMONSTER_STAGE_LURK;
  clock=SEAMONSTER_LURK_TIME;
}

static void seamonster_begin_SWIM(struct fmn_sprite *sprite) {
  sprite->style=FMN_SPRITE_STYLE_TWOFRAME;
  sprite->tileid=tileid0;
  stage=SEAMONSTER_STAGE_SWIM;
  seamonster_choose_swim_destination(sprite);
  if (swimdx<0.0f) sprite->xform=FMN_XFORM_XREV;
  else sprite->xform=0;
}

static void seamonster_begin_SURFACE(struct fmn_sprite *sprite) {
  sprite->style=FMN_SPRITE_STYLE_TWOFRAME;
  sprite->tileid=tileid0+2;
  stage=SEAMONSTER_STAGE_SURFACE;
  clock=SEAMONSTER_SURFACE_TIME;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (herox<sprite->x) sprite->xform=FMN_XFORM_XREV;
  else sprite->xform=0;
}

static void seamonster_begin_SPIT(struct fmn_sprite *sprite) {

  // Proceed with spitting if hero is in the 90-degree cone. Don't change direction.
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (sprite->xform&FMN_XFORM_XREV) {
    if (herox>=sprite->x) { seamonster_begin_LURK(sprite); return; }
  } else {
    if (herox<=sprite->x) { seamonster_begin_LURK(sprite); return; }
  }
  float dx=herox-sprite->x; if (dx<0.0f) dx=-dx;
  float dy=heroy-sprite->y; if (dy<0.0f) dy=-dy;
  // If we want a wider or narrower cone, insert a multiplier here:
  if (dy>=dx) { seamonster_begin_LURK(sprite); return; }
  
  // OK! Make a missile, do the spit animation, and let the missile do its thing.
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->tileid=tileid0+4;
  stage=SEAMONSTER_STAGE_SPIT;
  clock=SEAMONSTER_SPIT_TIME;
  float x=sprite->x;
  if (sprite->xform&FMN_XFORM_XREV) x-=0.5f;
  else x+=0.5f;
  const uint8_t cmdv[]={
    0x42,FMN_SPRCTL_missile>>8,FMN_SPRCTL_missile,
    0x20,sprite->imageid,
    0x21,tileid0+5,
    0x23,FMN_SPRITE_STYLE_TILE,
  };
  const uint8_t argv[]={};
  struct fmn_sprite *star=fmn_sprite_spawn(x,sprite->y,0,cmdv,sizeof(cmdv),argv,sizeof(argv));
}

/* Init.
 */
 
static void _seamonster_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  seamonster_begin_LURK(sprite);
}

/* Update, per stage.
 */
 
static void seamonster_update_LURK(struct fmn_sprite *sprite,float elapsed) {
}
 
static void seamonster_update_SWIM(struct fmn_sprite *sprite,float elapsed) {
  sprite->x+=swimdx*elapsed;
  sprite->y+=swimdy*elapsed;
}
 
static void seamonster_update_SURFACE(struct fmn_sprite *sprite,float elapsed) {
}
 
static void seamonster_update_SPIT(struct fmn_sprite *sprite,float elapsed) {
}

/* Update.
 */
 
static void _seamonster_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) return;
  if ((clock-=elapsed)<=0.0f) switch (stage) {
    case SEAMONSTER_STAGE_LURK: seamonster_begin_SWIM(sprite); break;
    case SEAMONSTER_STAGE_SWIM: seamonster_begin_SURFACE(sprite); break;
    case SEAMONSTER_STAGE_SURFACE: seamonster_begin_SPIT(sprite); break;
    case SEAMONSTER_STAGE_SPIT: seamonster_begin_LURK(sprite); break;
  } else switch (stage) {
    case SEAMONSTER_STAGE_LURK: seamonster_update_LURK(sprite,elapsed); break;
    case SEAMONSTER_STAGE_SWIM: seamonster_update_SWIM(sprite,elapsed); break;
    case SEAMONSTER_STAGE_SURFACE: seamonster_update_SURFACE(sprite,elapsed); break;
    case SEAMONSTER_STAGE_SPIT: seamonster_update_SPIT(sprite,elapsed); break;
  }
}

/* Interact.
 */
 
static int16_t _seamonster_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: if (!sleeping) {
            sleeping=1;
            sprite->style=FMN_SPRITE_STYLE_HIDDEN;
            fmn_sprite_generate_zzz(sprite);
          } break;
        case FMN_SPELLID_REVEILLE: if (sleeping) {
            sleeping=0;
            seamonster_begin_LURK(sprite);
          } break;
      } break;
    case FMN_ITEM_BELL: if (sleeping) {
        sleeping=0;
        seamonster_begin_LURK(sprite);
      } break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_seamonster={
  .init=_seamonster_init,
  .update=_seamonster_update,
  .interact=_seamonster_interact,
};

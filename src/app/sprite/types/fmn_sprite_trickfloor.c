#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

/* Static globals.
 */
 
#define TRICKFLOOR_PATH_LIMIT 20
 
static uint8_t trickfloor_pathv[TRICKFLOOR_PATH_LIMIT];
static uint8_t trickfloor_pathc=0;
static struct trickfloor_bounds {
  int8_t x,y,w,h;
} trickfloor_bounds;
static uint8_t trickfloor_herox,trickfloor_heroy;
static uint8_t trickfloor_herop=0xff; // current position in path
static int8_t trickfloor_herod=0; // -1 or 1 if in the path

/* Manhattan distance between two cell positions.
 */
 
static uint8_t trickfloor_mdist(uint8_t a,uint8_t b) {
  uint8_t ax=a%FMN_COLC,ay=a/FMN_COLC;
  uint8_t bx=b%FMN_COLC,by=b/FMN_COLC;
  return (
    ((ax>bx)?(ax-bx):(bx-ax))+
    ((ay>by)?(ay-by):(by-ay))
  );
}

static uint8_t trickfloor_midpt(uint8_t a,uint8_t b) {
  uint8_t ax=a%FMN_COLC,ay=a/FMN_COLC;
  uint8_t bx=b%FMN_COLC,by=b/FMN_COLC;
  uint8_t x=(ax+bx)>>1;
  uint8_t y=(ay+by)>>1;
  return y*FMN_COLC+x;
}

static void trickfloor_splitdist(int8_t *dx,int8_t *dy,uint8_t a,uint8_t b) {
  uint8_t ax=a%FMN_COLC,ay=a/FMN_COLC;
  uint8_t bx=b%FMN_COLC,by=b/FMN_COLC;
  *dx=bx-ax;
  *dy=by-ay;
}

static uint8_t trickfloor_offset(uint8_t o,int8_t dx,int8_t dy) {
  uint8_t x=(o%FMN_COLC)+dx;
  uint8_t y=(o/FMN_COLC)+dy;
  return y*FMN_COLC+x;
}

static uint8_t trickfloor_offset_random(uint8_t o) {
  uint8_t x=(o%FMN_COLC);
  uint8_t y=(o/FMN_COLC);
  uint8_t candidatev[4];
  uint8_t candidatec=0;
  if (x>trickfloor_bounds.x) candidatev[candidatec++]=o-1;
  if (y>trickfloor_bounds.y) candidatev[candidatec++]=o-FMN_COLC;
  if (x<trickfloor_bounds.x+trickfloor_bounds.w-1) candidatev[candidatec++]=o+1;
  if (y<trickfloor_bounds.y+trickfloor_bounds.h-1) candidatev[candidatec++]=o+FMN_COLC;
  return candidatev[rand()%candidatec];
}

/* Nonzero if the given rectangle is entirely useable.
 */
 
static uint8_t trickfloor_can_grow(uint8_t focustile,int8_t x,int8_t y,int8_t w,int8_t h) {
  // We must not grow to the screen edge; leave a 1-tile margin on all sides.
  if (x<1) return 0;
  if (y<1) return 0;
  if (x+w>=FMN_COLC) return 0;
  if (y+h>=FMN_ROWC) return 0;
  const uint8_t *srcrow=fmn_global.map+y*FMN_COLC+x;
  uint8_t yi=h;
  for (;yi-->0;srcrow+=FMN_COLC) {
    const uint8_t *srcp=srcrow;
    uint8_t xi=w;
    for (;xi-->0;srcp++) {
      if (*srcp<focustile-1) return 0;
      if (*srcp>focustile+1) return 0;
      if (fmn_global.cellphysics[*srcp]!=FMN_CELLPHYSICS_UNSHOVELLABLE) return 0;
    }
  }
  return 1;
}

/* Build path.
 */
 
static void trickfloor_rebuild_path(uint8_t col,uint8_t row) {
  trickfloor_herop=0xff;
  trickfloor_herod=0;
  trickfloor_pathc=0;
  trickfloor_bounds.w=0;
  trickfloor_bounds.h=0;
  if ((col>=FMN_COLC)||(row>=FMN_ROWC)) return;
  trickfloor_bounds.x=col;
  trickfloor_bounds.y=row;
  
  // Confirm that the cell the sprite started on is unshovellable.
  uint8_t focustile=fmn_global.map[row*FMN_COLC+col];
  if (fmn_global.cellphysics[focustile]!=FMN_CELLPHYSICS_UNSHOVELLABLE) return;
  trickfloor_bounds.w=1;
  trickfloor_bounds.h=1;
  
  // Expand the bounds to consume all unshovellable tiles within 1 column of the focus on the tilesheet.
  struct trickfloor_bounds *b=&trickfloor_bounds;
  while (1) {
    uint8_t proceed=0;
    if (trickfloor_can_grow(focustile,b->x-1,b->y,1,b->h)) { proceed=1; b->w++; b->x--; }
    if (trickfloor_can_grow(focustile,b->x+b->w,b->y,1,b->h)) { proceed=1; b->w++; }
    if (trickfloor_can_grow(focustile,b->x,b->y-1,b->w,1)) { proceed=1; b->h++; b->y--; }
    if (trickfloor_can_grow(focustile,b->x,b->y+b->h,b->w,1)) { proceed=1; b->h++; }
    if (!proceed) break;
  }
  if (b->w<1) return;
  if (b->h<1) return;
  
  // Choose the endpoints. There must be exactly two edge cells with a VACANT or UNSHOVELLABLE neighbor outside.
  uint8_t startp=0xff,endp=0xff; // "start" and "end" are not really meaningful, you can run the path either direction.
  const uint8_t *srca,*srcb;
  uint8_t p,i;
  srca=fmn_global.map+b->y*FMN_COLC+b->x-1;
  srcb=srca+b->w+1;
  // beware tileid comes from a neighbor of (x,y), not (x,y) itself.
  #define CHECKCELL(tileid,x,y) switch (fmn_global.cellphysics[tileid]) { \
    case FMN_CELLPHYSICS_VACANT: \
    case FMN_CELLPHYSICS_UNSHOVELLABLE: { \
        if (startp==0xff) startp=(y)*FMN_COLC+(x); \
        else if (endp==0xff) endp=(y)*FMN_COLC+(x); \
        else { \
          fmn_log("trickfloor: Too many entry points."); \
          b->w=b->h=0; \
          return; \
        } \
      } break; \
  }
  for (p=b->y,i=b->h;i-->0;p++,srca+=FMN_COLC,srcb+=FMN_COLC) {
    CHECKCELL(*srca,b->x,p)
    CHECKCELL(*srcb,b->x+b->w-1,p)
  }
  srca=fmn_global.map+(b->y-1)*FMN_COLC+b->x;
  srcb=srca+(b->h+1)*FMN_COLC;
  for (p=b->x,i=b->w;i-->0;p++,srca++,srcb++) {
    CHECKCELL(*srca,p,b->y)
    CHECKCELL(*srcb,p,b->y+b->h-1)
  }
  #undef CHECKCELL
  if (endp==0xff) {
    fmn_log("trickfloor: Couldn't find two entry points.");
    b->w=b->h=0;
    return;
  }
  
  // Choose a length.
  // Hard limits: ManhattanDistance(startp,endp)+1 .. TRICKFLOOR_PATH_LIMIT
  // Aside from that, the length must be an odd number greater than the manhattan distance.
  uint8_t lenmin=trickfloor_mdist(startp,endp)+1;
  uint8_t lenmax=TRICKFLOOR_PATH_LIMIT;
  if ((lenmax-lenmin)&1) lenmax--;
  if (lenmin>lenmax) { // not possible
    b->w=b->h=0;
    return;
  }
  trickfloor_pathc=lenmax; //TODO provide in argv?
  uint8_t ap=0;
  uint8_t bp=trickfloor_pathc-1;
  trickfloor_pathv[ap]=startp;
  trickfloor_pathv[bp]=endp;
  
  // Drunk walk until we reach the deadline, then close the path.
  uint8_t toggle=0;
  while (1) {
    int8_t remaining=bp-ap-1;
    if (remaining<1) break;
    uint8_t mdist=trickfloor_mdist(trickfloor_pathv[ap],trickfloor_pathv[bp]);
    uint8_t p,pvcell,farcell;
    if (toggle^=1) {
      farcell=trickfloor_pathv[bp];
      pvcell=trickfloor_pathv[ap];
      ap++;
      p=ap;
    } else {
      farcell=trickfloor_pathv[ap];
      pvcell=trickfloor_pathv[bp];
      bp--;
      p=bp;
    }
    if (remaining<=mdist-1) { // must close path
      int8_t dx,dy;
      trickfloor_splitdist(&dx,&dy,pvcell,farcell);
           if (dx<0) trickfloor_pathv[p]=trickfloor_offset(pvcell,-1,0);
      else if (dx>0) trickfloor_pathv[p]=trickfloor_offset(pvcell,1,0);
      else if (dy<0) trickfloor_pathv[p]=trickfloor_offset(pvcell,0,-1);
      else           trickfloor_pathv[p]=trickfloor_offset(pvcell,0,1);
    } else { // go wherever
      trickfloor_pathv[p]=trickfloor_offset_random(pvcell);
    }
  }
}

/* Generate a fire on the cell the hero is currently touching.
 */
 
struct trickfloor_fpt {
  float x,y;
};

static int trickfloor_check_hazard(struct fmn_sprite *sprite,void *userdata) {
  struct trickfloor_fpt *pt=userdata;
  if (sprite->controller!=FMN_SPRCTL_hazard) return 0;
  float dx=sprite->x-pt->x;
  if ((dx<-0.5f)||(dx>0.5f)) return 0;
  float dy=sprite->y-pt->y;
  if ((dy<-0.5f)||(dy>0.5f)) return 0;
  return 1;
}
 
static void trickfloor_generate_hazard() {
  struct trickfloor_fpt pt={
    .x=trickfloor_herox+0.5f,
    .y=trickfloor_heroy+0.5f,
  };
  if (fmn_sprites_for_each(trickfloor_check_hazard,&pt)) return;
  struct fmn_sprite *hazard=fmn_sprite_generate_noparam(FMN_SPRCTL_hazard,pt.x,pt.y);
  if (!hazard) return;
  hazard->imageid=3;
  hazard->tileid=0x20;
  hazard->style=FMN_SPRITE_STYLE_FOURFRAME;
  hazard->layer=128;
  hazard->radius=0.5f;
}

/* EXPERIMENTAL. Nudge the hero gently toward the center of her cell.
 */
 
static void trickfloor_nudge_to_center(float herox,float heroy,float elapsed) {
  float dummy;
  float modx=modff(herox,&dummy);
  float mody=modff(heroy,&dummy);
  const float nudge_rate=0.5f;
  const float close_enough=0.02f;
  float dstx=herox,dsty=heroy;
  if (modx<0.5f-close_enough) dstx+=nudge_rate*elapsed;
  else if (modx>0.5f+close_enough) dstx-=nudge_rate*elapsed;
  if (mody<0.5f-close_enough) dsty+=nudge_rate*elapsed;
  else if (mody>0.5f+close_enough) dsty-=nudge_rate*elapsed;
  fmn_hero_set_position(dstx,dsty);
}

/* Track the hero's position, quantized to cells, and update things when she changes cell.
 */
 
static void trickfloor_check_hero(float elapsed) {

  // Drop any tracking state if she left the ground.
  if (!fmn_hero_feet_on_ground()) {
    trickfloor_herox=0xff;
    trickfloor_heroy=0xff;
    trickfloor_herod=0;
    trickfloor_herop=0xff;
    return;
  }

  // Quantize hero position. If unchanged, get out. Same if OOB.
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  uint8_t herocol=(uint8_t)herox;
  uint8_t herorow=(uint8_t)heroy;
  if ((herocol==trickfloor_herox)&&(herorow==trickfloor_heroy)) {
    if ((herocol>=trickfloor_bounds.x)&&(herorow>=trickfloor_bounds.y)&&(herocol<trickfloor_bounds.x+trickfloor_bounds.w)&&(herorow<trickfloor_bounds.y+trickfloor_bounds.h)) {
      trickfloor_nudge_to_center(herox,heroy,elapsed);
    }
    return;
  }
  if ((herocol>=FMN_COLC)||(herorow>=FMN_ROWC)) return;
  trickfloor_herox=herocol;
  trickfloor_heroy=herorow;
  uint8_t cellp=herorow*FMN_COLC+herocol;
  
  // If she left the floor, reset path position.
  if (
    (trickfloor_herox<trickfloor_bounds.x)||
    (trickfloor_heroy<trickfloor_bounds.y)||
    (trickfloor_herox>=trickfloor_bounds.x+trickfloor_bounds.w)||
    (trickfloor_heroy>=trickfloor_bounds.y+trickfloor_bounds.h)
  ) {
    trickfloor_herod=0;
    trickfloor_herop=0xff;
    return;
  }
  
  // If herod zero, we were not tracking before. She must be on one of the endpoints.
  if (!trickfloor_herod) {
    if (cellp==trickfloor_pathv[0]) {
      trickfloor_herop=0;
      trickfloor_herod=1;
    } else if (cellp==trickfloor_pathv[trickfloor_pathc-1]) {
      trickfloor_herop=trickfloor_pathc-1;
      trickfloor_herod=-1;
    } else {
      trickfloor_generate_hazard();
      trickfloor_herop=0xff;
      trickfloor_herod=0;
    }
    
  // herod nonzero. Advance to the next position if she guessed right.
  } else {
    if (trickfloor_herod<0) {
      if (trickfloor_herop) trickfloor_herop--;
    } else {
      if (trickfloor_herop<trickfloor_pathc-1) trickfloor_herop++;
    }
    if (cellp!=trickfloor_pathv[trickfloor_herop]) {
      trickfloor_generate_hazard();
      trickfloor_herop=0xff;
      trickfloor_herod=0;
    }
  }
}

/* Update the global compass target.
 */
 
static void trickfloor_update_compass() {
  if ((trickfloor_herod<0)&&(trickfloor_herop>0)) { // in the maze; point to the next direction.
    uint8_t nextcellp=trickfloor_pathv[trickfloor_herop-1];
    uint8_t nextx=nextcellp%FMN_COLC;
    uint8_t nexty=nextcellp/FMN_COLC;
    fmn_global.compassx=nextx;
    fmn_global.compassy=nexty;
  } else if ((trickfloor_herod>0)&&(trickfloor_herop<trickfloor_pathc-1)) {
    uint8_t nextcellp=trickfloor_pathv[trickfloor_herop+1];
    uint8_t nextx=nextcellp%FMN_COLC;
    uint8_t nexty=nextcellp/FMN_COLC;
    fmn_global.compassx=nextx;
    fmn_global.compassy=nexty;
  } else {
    fmn_global.compassx=0;
    fmn_global.compassy=0;
  }
}

/* Update.
 */
 
static void _trickfloor_class_update(void *userdata,float elapsed) {
  if (trickfloor_pathc<1) return;
  trickfloor_check_hero(elapsed);
  trickfloor_update_compass();
}

/* Init.
 */
 
static void _trickfloor_init(struct fmn_sprite *sprite) {
  if (!fmn_game_register_map_singleton(_trickfloor_init,_trickfloor_class_update,0,0)) return;
  trickfloor_rebuild_path((uint8_t)sprite->x,(uint8_t)sprite->y);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_trickfloor={
  .init=_trickfloor_init,
};

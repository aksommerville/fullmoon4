#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define FIREWALL_STATE_IDLE 0
#define FIREWALL_STATE_ADVANCE 1
#define FIREWALL_STATE_RETREAT 2

#define FIREWALL_ADVANCE_SPEED 7.0f
#define FIREWALL_RETREAT_SPEED 2.0f

#define dir sprite->bv[0] /* the edge i'm attached to, the direction hero faces to move me. i move opposite dir. */
#define cellp sprite->bv[1] /* first row or column */
#define cellc sprite->bv[2]
#define range sprite->bv[3]
#define state sprite->bv[4]
#define extent sprite->fv[0] /* 1..range */

/* Init.
 */
 
static void _firewall_init(struct fmn_sprite *sprite) {

  // Starting position must be on an edge, and not a corner.
  // Expand to fill the entire edge, up to the first solid cell on each side.
  // There must be at least two cells to occupy.
  #define INVALID { \
    fmn_log("firewall invalid!"); \
    fmn_sprite_kill(sprite); \
    return; \
  }
  #define OKCELL(xx,yy) ({ \
    uint8_t ok=0; \
    switch (fmn_global.cellphysics[fmn_global.map[(yy)*FMN_COLC+(xx)]]) { \
      case FMN_CELLPHYSICS_VACANT: \
      case FMN_CELLPHYSICS_HOLE: \
      case FMN_CELLPHYSICS_UNSHOVELLABLE: \
      case FMN_CELLPHYSICS_WATER: \
      case FMN_CELLPHYSICS_FOOTHAZARD: \
        ok=1; break; \
    } \
    ok; \
  })
  int8_t col=(int8_t)sprite->x;
  int8_t row=(int8_t)sprite->y;
  cellc=1;
  if (col==0) {
    if ((row<1)||(row>=FMN_ROWC-1)) INVALID
    dir=FMN_DIR_W;
    cellp=row;
    while (cellp&&OKCELL(col,cellp-1)) { cellp--; cellc++; }
    while ((cellp+cellc<FMN_ROWC)&&OKCELL(col,cellp+cellc)) cellc++;
  } else if (col==FMN_COLC-1) {
    if ((row<1)||(row>=FMN_ROWC-1)) INVALID
    dir=FMN_DIR_E;
    cellp=row;
    while (cellp&&(OKCELL(col,cellp-1))) { cellp--; cellc++; }
    while ((cellp+cellc<FMN_ROWC)&&OKCELL(col,cellp+cellc)) cellc++;
  } else if (row==0) {
    if ((col<1)||(col>=FMN_COLC)) INVALID
    dir=FMN_DIR_N;
    cellp=col;
    while (cellp&&OKCELL(cellp-1,row)) { cellp--; cellc++; }
    while ((cellp+cellc<FMN_COLC)&&OKCELL(cellp+cellc,row)) cellc++;
  } else if (row==FMN_ROWC-1) {
    if ((col<1)||(col>=FMN_COLC)) INVALID
    dir=FMN_DIR_S;
    cellp=col;
    while (cellp&&OKCELL(cellp-1,row)) { cellp--; cellc++; }
    while ((cellp+cellc<FMN_COLC)&&OKCELL(cellp+cellc,row)) cellc++;
  } else INVALID
  if (!OKCELL(col,row)) INVALID
  
  // Now that the direction and permanent area is established, measure how far we can extend.
  uint8_t colc=1,rowc=1;
  int8_t dcol=0,drow=0;
  switch (dir) {
    case FMN_DIR_W: row=cellp; rowc=cellc; dcol=1; break;
    case FMN_DIR_E: row=cellp; rowc=cellc; dcol=-1; break;
    case FMN_DIR_N: col=cellp; colc=cellc; drow=1;  break;
    case FMN_DIR_S: col=cellp; colc=cellc; drow=-1; break;
  }
  while ((col>=0)&&(col<FMN_COLC)&&(row>=0)&&(row<FMN_ROWC)) {
    uint8_t ok=1;
    if (dcol) {
      uint8_t suby=rowc; while (suby-->0) {
        if (!OKCELL(col,row+suby)) { ok=0; break; }
      }
    } else {
      uint8_t subx=colc; while (subx-->0) {
        if (!OKCELL(col+subx,row)) { ok=0; break; }
      }
    }
    if (!ok) {
      colc-=dcol?1:0;
      rowc-=drow?1:0;
      break;
    }
    colc+=dcol?1:0;
    rowc+=drow?1:0;
    col+=dcol;
    row+=drow;
  }
  range=(dcol?colc:rowc);
  if (!range) INVALID
  
  #undef OKCELL
  #undef INVALID
  
  extent=1.0f;
}

/* Are we being looked at?
 */
 
static uint8_t firewall_hero_is_peeking(const struct fmn_sprite *sprite,float herox,float heroy,uint8_t facedir) {

  // If she's looking away, easy, no peek.
  if (facedir!=dir) return 0;
  
  // If she's invisible it's irrelevant. Tree falling in the woods, etc, etc.
  if (fmn_global.invisibility_time>0.0f) return 0;
  
  // Check range on my long axis. The other axis doesn't matter; we are bound to one screen edge.
  if ((dir==FMN_DIR_W)||(dir==FMN_DIR_E)) {
    if (heroy<cellp) return 0;
    if (heroy>cellp+cellc) return 0;
  } else {
    if (herox<cellp) return 0;
    if (herox>cellp+cellc) return 0;
  }
  
  // Require a line of sight with no intervening solids. It's simpler to check all the way to the edge.
  // If she's offscreen, call it no-peek regardless. That's a weird case that shouldn't come up, or last long if it does.
  int8_t col=(int8_t)herox,row=(int8_t)heroy,colc=1,rowc=1;
  if ((col<0)||(row<0)||(col>=FMN_COLC)||(row>=FMN_ROWC)) return 0;
  switch (dir) {
    case FMN_DIR_W: colc=col; col=0; break;
    case FMN_DIR_E: colc=FMN_COLC-col; break;
    case FMN_DIR_N: rowc=row; row=0; break;
    case FMN_DIR_S: rowc=FMN_ROWC-row; break;
  }
  const uint8_t *cellrow=fmn_global.map+row*FMN_COLC+col;
  for (;rowc-->0;cellrow+=FMN_COLC) {
    uint8_t coli=colc;
    const uint8_t *cell=cellrow;
    for (;coli-->0;cell++) {
      switch (fmn_global.cellphysics[*cell]) {
        case FMN_CELLPHYSICS_VACANT:
        case FMN_CELLPHYSICS_HOLE:
        case FMN_CELLPHYSICS_UNSHOVELLABLE:
        case FMN_CELLPHYSICS_WATER:
        case FMN_CELLPHYSICS_FOOTHAZARD:
          break;
        default: return 0;
      }
    }
  }
  return 1;
}

/* Update.
 */
 
static void _firewall_update(struct fmn_sprite *sprite,float elapsed) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  
  // When advancing, our purpose is clear. Advance all the way.
  if (state==FIREWALL_STATE_ADVANCE) {
    extent+=FIREWALL_ADVANCE_SPEED*elapsed;
    if (extent>=range) {
      extent=range;
      state=FIREWALL_STATE_RETREAT;
    }
    
  // In RETREAT or IDLE state, we can be triggered by the hero looking at us.
  } else {
    if (firewall_hero_is_peeking(sprite,herox,heroy,fmn_global.facedir)) {
      state=FIREWALL_STATE_ADVANCE;
    }
    if (state==FIREWALL_STATE_RETREAT) {
      extent-=FIREWALL_RETREAT_SPEED*elapsed;
      if (extent<=1.0f) {
        extent=1.0f;
        state=FIREWALL_STATE_IDLE;
      }
    }
  }
  
  // Check for collisions in any state. (invisibility doesn't matter here)
  float x,y,w,h;
  switch (dir) {
    case FMN_DIR_W: x=0.0f; y=cellp; w=extent; h=cellc; break;
    case FMN_DIR_E: x=FMN_COLC-extent; y=cellp; w=extent; h=cellc; break;
    case FMN_DIR_N: x=cellp; y=0.0f; w=cellc; h=extent; break;
    case FMN_DIR_S: x=cellp; y=FMN_ROWC-extent; w=cellc; h=extent; break;
  }
  if ((herox>=x)&&(heroy>=y)&&(herox<x+w)&&(heroy<y+h)) {
    // Report the collision's location as cardinal to the hero.
    // If it would push her in our bound direction, reverse it.
    float hurtx=herox,hurty=heroy;
    float nx=(herox-x)/w;
    float ny=(heroy-y)/h;
    if (nx<ny) {
      if (nx<1.0f-ny) { // W
        if (dir==FMN_DIR_W) hurtx-=1.0f;
        else hurtx+=1.0f;
      } else { // S
        if (dir==FMN_DIR_S) hurty+=1.0f;
        else hurty-=1.0f;
      }
    } else if (nx<1.0f-ny) { // N
      if (dir==FMN_DIR_N) hurty-=1.0f;
      else hurty+=1.0f;
    } else { // E
      if (dir==FMN_DIR_E) hurtx+=1.0f;
      else hurtx-=1.0f;
    }
    fmn_hero_injure(hurtx,hurty,sprite);
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_firewall={
  .init=_firewall_init,
  .update=_firewall_update,
};

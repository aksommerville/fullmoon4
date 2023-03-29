#include "app/fmn_platform.h"
#include "fmn_physics.h"
#include "fmn_sprite.h"

#define HB(d,who) (((who)->radius>0.0f)?(who)->radius:(who)->hb##d)

/* Check edges.
 */
 
uint8_t fmn_physics_check_edges(float *cx,float *cy,const struct fmn_sprite *a) {
  uint8_t result=0;
  if (a->x<HB(w,a)) {
    if (cx) *cx=HB(w,a)-a->x;
    result=1;
  } else if (a->x>FMN_COLC-HB(e,a)) {
    if (cx) *cx=FMN_COLC-HB(e,a)-a->x;
    result=1;
  } else if (cx) *cx=0.0f;
  if (a->y<HB(n,a)) {
    if (cy) *cy=HB(n,a)-a->y;
    result=1;
  } else if (a->y>FMN_ROWC-HB(s,a)) {
    if (cy) *cy=FMN_ROWC-HB(s,a)-a->y;
    result=1;
  } else if (cy) *cy=0.0f;
  return result;
}

/* Check grid.
 */
 
uint8_t fmn_physics_check_grid(float *cx,float *cy,const struct fmn_sprite *a,uint8_t features) {

  // Important that we stop early and special, if the right or bottom edge is below zero.
  if (a->x+HB(e,a)<0.0f) return 0;
  if (a->y+HB(s,a)<0.0f) return 0;
  int8_t cola=a->x-HB(w,a);
  int8_t colz=a->x+HB(e,a);
  if (cola<0) cola=0;
  if (colz>=FMN_COLC) colz=FMN_COLC-1;
  if (cola>colz) return 0;
  int8_t rowa=a->y-HB(n,a);
  int8_t rowz=a->y+HB(s,a);
  if (rowa<0) rowa=0;
  if (rowz>=FMN_ROWC) rowz=FMN_ROWC-1;
  if (rowa>rowz) return 0;
  
  /* We will correct in cardinal directions only, whichever is closest.
   * However! It is very important that we examine all of the cells before attempting any correction.
   * This algorithm can fail if we correct out to an unchecked cell.
   * Hopefully the sprite is moving by small enough increments that that will never come up.
   */
  uint8_t result=0;
  float el,er,eu,ed; // Maximum escapement in each direction.
  int8_t vacantx=-1,vacanty=-1; // Any cell under consideration which is not collidable.
  
  const uint8_t *cellrow=fmn_global.map+rowa*FMN_COLC+cola;
  int8_t row=rowa; for (;row<=rowz;row++,cellrow+=FMN_COLC) {
    const uint8_t *cellp=cellrow;
    int8_t col=cola; for (;col<=colz;col++,cellp++) {
      uint8_t physics=fmn_global.cellphysics[*cellp];
      switch (physics) {
        case FMN_CELLPHYSICS_SOLID:
        case FMN_CELLPHYSICS_UNCHALKABLE:
        case FMN_CELLPHYSICS_SAP:
        case FMN_CELLPHYSICS_SAP_NOCHALK:
        case FMN_CELLPHYSICS_REVELABLE: {
            if (!(features&FMN_PHYSICS_SOLID)) goto _vacant_;
          } break;
        case FMN_CELLPHYSICS_HOLE:
        case FMN_CELLPHYSICS_WATER: {
            if (!(features&FMN_PHYSICS_HOLE)) goto _vacant_;
          } break;
        default: _vacant_: vacantx=col; vacanty=row; continue;
      }
      
      if (!cx||!cy) return 1;
      // Measure escapement for this cell alone.
      float el1=a->x+HB(e,a)-col;
      float er1=col+1.0f-a->x+HB(w,a);
      float eu1=a->y+HB(s,a)-row;
      float ed1=row+1.0f-a->y+HB(n,a);
      // If it's the first collision, that's it.
      if (!result) {
        result=1;
        el=el1;
        er=er1;
        eu=eu1;
        ed=ed1;
      // Otherwise, compare to prior escapements and keep the longer.
      } else {
        if (el1>el) el=el1;
        if (er1>er) er=er1;
        if (eu1>eu) eu=eu1;
        if (ed1>ed) ed=ed1;
      }
    }
  }
  if (!result) return 0;
  
  *cx=*cy=0.0f;
  if ((el<=er)&&(el<=eu)&&(el<=ed)) *cx=-el;
  else if ((er<=eu)&&(er<=ed)) *cx=er;
  else if (eu<=ed) *cy=-eu;
  else *cy=ed;
  
  /* There's one situation where we fail pretty bad: concave corners.
   * The obvious thing when you walk into a corner is to correct on both axes, and leave the sprite snug in the corner.
   * Our "one axis of correction" policy makes that impossible, and we decide instead to launch the sprite clear of all considered cells.
   * So if we end up with a correction longer than one tile, and a vacant tile was discovered during iteration, clamp into it.
   * It is expected that sprites be no larger than one tile. If they go larger than that, this might get weird.
   */
  if ((*cx<=-1.0f)||(*cx>=1.0f)||(*cy<=-1.0f)||(*cy>=1.0f)) {
    if (vacantx>=0) {
    
      // Double-check (vacant). If the cell containing the hero's center is vacant, that's the one we want.
      // Failing to do this means the hero can skip thru southern corners of a checkerboard.
      int8_t midx=(int8_t)a->x;
      int8_t midy=(int8_t)a->y;
      if (
        ((midx!=vacantx)||(midy!=vacanty))&&
        (midx>=0)&&(midy>=0)&&(midx<FMN_COLC)&&(midy<FMN_ROWC)
      ) {
        uint8_t physics=fmn_global.cellphysics[fmn_global.map[midy*FMN_COLC+midx]];
        switch (physics) {
          case FMN_CELLPHYSICS_SOLID:
          case FMN_CELLPHYSICS_UNCHALKABLE:
          case FMN_CELLPHYSICS_SAP:
          case FMN_CELLPHYSICS_SAP_NOCHALK:
          case FMN_CELLPHYSICS_REVELABLE: {
              if (!(features&FMN_PHYSICS_SOLID)) {
                vacantx=midx;
                vacanty=midy;
              }
            } break;
          case FMN_CELLPHYSICS_HOLE:
          case FMN_CELLPHYSICS_WATER: {
              if (!(features&FMN_PHYSICS_HOLE)) {
                vacantx=midx;
                vacanty=midy;
              }
            } break;
          default: vacantx=midx; vacanty=midy;
        }
      }
    
      float vmx=vacantx+0.5f,vmy=vacanty+0.5f;
      if (a->x>=vmx) *cx=vacantx+1.0f-a->x-HB(w,a);
      else *cx=vacantx-a->x+HB(e,a);
      if (a->y>=vmy) *cy=vacanty+1.0f-a->y-HB(s,a);
      else *cy=vacanty-a->y+HB(n,a);
    }
  }
  
  return 1;
}

/* Check two sprites.
 */
 
uint8_t fmn_physics_check_sprites(float *cx,float *cy,const struct fmn_sprite *a,const struct fmn_sprite *b) {

  // Check as rectangles.
  float ax0=a->x-HB(w,a);
  float bx1=b->x+HB(e,b);
  if (bx1<=ax0) return 0;
  float ax1=a->x+HB(e,a);
  float bx0=b->x-HB(w,b);
  if (ax1<=bx0) return 0;
  float ay0=a->y-HB(n,a);
  float by1=b->y+HB(s,b);
  if (by1<=ay0) return 0;
  float ay1=a->y+HB(s,a);
  float by0=b->y-HB(n,b);
  if (ay1<=by0) return 0;
  
  //TODO Consider colliding as circles instead of rectangles.
  
  if (!cx||!cy) return 1;
  float el=ax1-bx0;
  float er=bx1-ax0;
  float eu=ay1-by0;
  float ed=by1-ay0;
  *cx=*cy=0.0f;
  if ((el<=er)&&(el<=eu)&&(el<=ed)) *cx=-el;
  else if ((er<=eu)&&(er<=ed)) *cx=er;
  else if (eu<=ed) *cy=-eu;
  else *cy=ed;

  return 1;
}

/* Helper for our direction constants.
 */
 
uint8_t fmn_dir_from_vector(float x,float y) {
  float ax=(x<0.0f)?-x:x;
  float ay=(y<0.0f)?-y:y;
  // If the minor magnitude is more than half of the major, call it diagonal.
  // Note that (0,0) will not satisfy this. But I'm keeping the final 'if's just in case.
  if ((ax>ay/2.0f)&&(ay>ax/2.0f)) {
    if (x<0.0f) {
      if (y<0.0f) return FMN_DIR_NW;
      if (y>0.0f) return FMN_DIR_SW;
    } else if (x>0.0f) {
      if (y<0.0f) return FMN_DIR_NE;
      if (y>0.0f) return FMN_DIR_SE;
    }
  }
  // Cardinal...
  if (ax>ay) {
    if (x<0.0f) return FMN_DIR_W;
    if (x>0.0f) return FMN_DIR_E;
  } else {
    if (y<0.0f) return FMN_DIR_N;
    if (y>0.0f) return FMN_DIR_S;
  }
  return 0;
}

uint8_t fmn_dir_from_vector_cardinal(float x,float y) {
  float ax=(x<0.0f)?-x:x;
  float ay=(y<0.0f)?-y:y;
  if (ax>ay) {
    if (x<0.0f) return FMN_DIR_W;
    if (x>0.0f) return FMN_DIR_E;
  } else {
    if (y<0.0f) return FMN_DIR_N;
    if (y>0.0f) return FMN_DIR_S;
  }
  return 0;
}

uint8_t fmn_dir_reverse(uint8_t dir) {
  switch (dir) {
    case FMN_DIR_NW: return FMN_DIR_SE;
    case FMN_DIR_N:  return FMN_DIR_S;
    case FMN_DIR_NE: return FMN_DIR_SW;
    case FMN_DIR_W:  return FMN_DIR_E;
    case FMN_DIR_E:  return FMN_DIR_W;
    case FMN_DIR_SW: return FMN_DIR_NE;
    case FMN_DIR_S:  return FMN_DIR_N;
    case FMN_DIR_SE: return FMN_DIR_NW;
  }
  return dir;
}

void fmn_vector_from_dir(float *x,float *y,uint8_t dir) {
  switch (dir) {
    case FMN_DIR_NW: *x=-1.0f; *y=-1.0f; return;
    case FMN_DIR_N:  *x= 0.0f; *y=-1.0f; return;
    case FMN_DIR_NE: *x= 1.0f; *y=-1.0f; return;
    case FMN_DIR_W:  *x=-1.0f; *y= 0.0f; return;
    case FMN_DIR_E:  *x= 1.0f; *y= 0.0f; return;
    case FMN_DIR_SW: *x=-1.0f; *y= 1.0f; return;
    case FMN_DIR_S:  *x= 0.0f; *y= 1.0f; return;
    case FMN_DIR_SE: *x= 1.0f; *y= 1.0f; return;
  }
  *x=0.0f;
  *y=0.0f;
}

#include "app/fmn_platform.h"
#include "fmn_physics.h"
#include "fmn_sprite.h"

/* Check edges.
 */
 
uint8_t fmn_physics_check_edges(float *cx,float *cy,const struct fmn_sprite *a) {
  uint8_t result=0;
  if (a->x<a->radius) {
    if (cx) *cx=a->radius-a->x;
    result=1;
  } else if (a->x>FMN_COLC-a->radius) {
    if (cx) *cx=FMN_COLC-a->radius-a->x;
    result=1;
  } else if (cx) *cx=0.0f;
  if (a->y<a->radius) {
    if (cy) *cy=a->radius-a->y;
    result=1;
  } else if (a->y>FMN_ROWC-a->radius) {
    if (cy) *cy=FMN_ROWC-a->radius-a->y;
    result=1;
  } else if (cy) *cy=0.0f;
  return result;
}

/* Check grid.
 */
 
uint8_t fmn_physics_check_grid(float *cx,float *cy,const struct fmn_sprite *a,uint8_t features) {

  int8_t cola=a->x-a->radius; // truncates (not floor), but that's fine because we're clamping to zero anyway.
  int8_t colz=a->x+a->radius;
  if (cola<0) cola=0;
  if (colz>=FMN_COLC) colz=FMN_COLC-1;
  if (cola>colz) return 0;
  int8_t rowa=a->y-a->radius;
  int8_t rowz=a->y+a->radius;
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
        case FMN_CELLPHYSICS_SAP_NOCHALK: {
            if (!(features&FMN_PHYSICS_SOLID)) continue;
          } break;
        case FMN_CELLPHYSICS_HOLE:
        case FMN_CELLPHYSICS_WATER: {
            if (!(features&FMN_PHYSICS_HOLE)) continue;
          } break;
        // If we add new tile physics, they go here.
        default: vacantx=col; vacanty=row; continue;
      }
      
      if (!cx||!cy) return 1;
      // Measure escapement for this cell alone.
      float el1=a->x+a->radius-col;
      float er1=col+1.0f-a->x+a->radius;
      float eu1=a->y+a->radius-row;
      float ed1=row+1.0f-a->y+a->radius;
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
      float vmx=vacantx+0.5f,vmy=vacanty+0.5f;
      if (a->x>=vmx) *cx=vacantx+1.0f-a->x-a->radius;
      else *cx=vacantx-a->x+a->radius;
      if (a->y>=vmy) *cy=vacanty+1.0f-a->y-a->radius;
      else *cy=vacanty-a->y+a->radius;
    }
  }
  
  return 1;
}

/* Check two sprites.
 */
 
uint8_t fmn_physics_check_sprites(float *cx,float *cy,const struct fmn_sprite *a,const struct fmn_sprite *b) {

  // Check as rectangles.
  float ax0=a->x-a->radius;
  float bx1=b->x+b->radius;
  if (bx1<=ax0) return 0;
  float ax1=a->x+a->radius;
  float bx0=b->x-b->radius;
  if (ax1<=bx0) return 0;
  float ay0=a->y-a->radius;
  float by1=b->y+b->radius;
  if (by1<=ay0) return 0;
  float ay1=a->y+a->radius;
  float by0=b->y-b->radius;
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

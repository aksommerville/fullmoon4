#include "fmn_hero_internal.h"
#include "app/sprite/fmn_physics.h"
#include <math.h>

/* Begin walking.
 */
 
static void fmn_hero_walk_begin(int8_t dx,int8_t dy) {
  fmn_global.walking=1;
  fmn_hero.walkdx=dx;
  fmn_hero.walkdy=dy;
}

/* Change direction while walking.
 */
 
static void fmn_hero_walk_turn(int8_t dx,int8_t dy) {
  fmn_hero.walkdx=dx;
  fmn_hero.walkdy=dy;
}

/* End walking.
 */
 
static void fmn_hero_walk_end() {
  fmn_global.walking=0;
  fmn_hero.walkdx=0;
  fmn_hero.walkdy=0;
}

/* Input event.
 * Set facedir.
 */
 
void fmn_hero_motion_event(uint8_t bit,uint8_t value) {

  if (fmn_global.active_item==FMN_ITEM_UMBRELLA) return;
  
  if (value) switch (bit) {
    case FMN_INPUT_LEFT:  fmn_global.facedir=FMN_DIR_W; break;
    case FMN_INPUT_RIGHT: fmn_global.facedir=FMN_DIR_E; break;
    case FMN_INPUT_UP:    fmn_global.facedir=FMN_DIR_N; break;
    case FMN_INPUT_DOWN:  fmn_global.facedir=FMN_DIR_S; break;
  } else switch (bit) {
    case FMN_INPUT_LEFT:  if (fmn_hero.walkdy<0) fmn_global.facedir=FMN_DIR_N; else if (fmn_hero.walkdy>0) fmn_global.facedir=FMN_DIR_S; break;
    case FMN_INPUT_RIGHT: if (fmn_hero.walkdy<0) fmn_global.facedir=FMN_DIR_N; else if (fmn_hero.walkdy>0) fmn_global.facedir=FMN_DIR_S; break;
    case FMN_INPUT_UP:    if (fmn_hero.walkdx<0) fmn_global.facedir=FMN_DIR_W; else if (fmn_hero.walkdx>0) fmn_global.facedir=FMN_DIR_E; break;
    case FMN_INPUT_DOWN:  if (fmn_hero.walkdx<0) fmn_global.facedir=FMN_DIR_W; else if (fmn_hero.walkdx>0) fmn_global.facedir=FMN_DIR_E; break;
  }
  
  if (fmn_global.facedir&(FMN_DIR_E|FMN_DIR_W)) fmn_global.last_horz_dir=fmn_global.facedir;
}

/* Extra helpers around facedir.
 */
 
uint8_t fmn_hero_facedir_agrees() {
  switch (fmn_global.facedir) {
    case FMN_DIR_W: if (fmn_hero.walkdx<0) return 1; break;
    case FMN_DIR_E: if (fmn_hero.walkdx>0) return 1; break;
    case FMN_DIR_N: if (fmn_hero.walkdy<0) return 1; break;
    case FMN_DIR_S: if (fmn_hero.walkdy>0) return 1; break;
  }
  return 0;
}

void fmn_hero_reset_facedir() {
       if (fmn_hero.walkdx<0) fmn_hero_motion_event(FMN_INPUT_LEFT,1);
  else if (fmn_hero.walkdx>0) fmn_hero_motion_event(FMN_INPUT_RIGHT,1);
  else if (fmn_hero.walkdy<0) fmn_hero_motion_event(FMN_INPUT_UP,1);
  else if (fmn_hero.walkdy>0) fmn_hero_motion_event(FMN_INPUT_DOWN,1);
}

/* Input state changed.
 */
 
void fmn_hero_motion_input(uint8_t state) {

  int8_t dx=0,dy=0;
  switch (state&(FMN_INPUT_LEFT|FMN_INPUT_RIGHT)) {
    case FMN_INPUT_LEFT: dx=-1; break;
    case FMN_INPUT_RIGHT: dx=1; break;
  }
  switch (state&(FMN_INPUT_UP|FMN_INPUT_DOWN)) {
    case FMN_INPUT_UP: dy=-1; break;
    case FMN_INPUT_DOWN: dy=1; break;
  }
  
  if (dx||dy) {
    if (!fmn_hero.walkdx&&!fmn_hero.walkdy) {
      fmn_hero_walk_begin(dx,dy);
    } else if ((fmn_hero.walkdx!=dx)||(fmn_hero.walkdy!=dy)) {
      fmn_hero_walk_turn(dx,dy);
    }
  } else if (fmn_hero.walkdx||fmn_hero.walkdy) {
    fmn_hero_walk_end();
  }
}

/* Is motion suppressed due to item or something?
 */
 
static uint8_t fmn_hero_should_suppress_motion() {
  switch (fmn_global.active_item) {
    case FMN_ITEM_SHOVEL: return 1;
    case FMN_ITEM_WAND: return 1;
    case FMN_ITEM_PITCHER: return 1;
    case FMN_ITEM_VIOLIN: return 1;
  }
  return 0;
}

/* Update.
 */
 
void fmn_hero_motion_update(float elapsed) {

  // We may drop (fmn_global.walking) during transitions or pauses.
  // We're updating now, so ensure it is true.
  if (fmn_hero.walkdx||fmn_hero.walkdy) fmn_global.walking=1;

  // Determine target velocity (ignoring elapsed).
  float max=(fmn_global.active_item==FMN_ITEM_BROOM)?FMN_HERO_FLY_SPEED_MAX:FMN_HERO_WALK_SPEED_MAX;
  float tvx=fmn_hero.walkdx*max;
  float tvy=fmn_hero.walkdy*max;
  if (fmn_hero_should_suppress_motion()) {
    tvx=tvy=0.0f;
  } else {
    if (fmn_hero.cheesetime>0.0f) {
      if ((fmn_hero.cheesetime-=elapsed)<=0.0f) {
        fmn_sound_effect(FMN_SFX_UNCHEESE);
        fmn_hero.cheesetime=0.0f;
        fmn_global.cheesing=0;
      } else {
        tvx*=FMN_HERO_CHEESE_ADJUST;
        tvy*=FMN_HERO_CHEESE_ADJUST;
      }
    }
    if (fmn_hero.walkdx&&fmn_hero.walkdy) {
      const float halfroot2=M_SQRT2/2.0f;
      tvx*=halfroot2;
      tvy*=halfroot2;
    }
    if (fmn_global.curse_time>0.0f) {
      tvx*=FMN_HERO_CURSE_SPEED_PENALTY;
      tvy*=FMN_HERO_CURSE_SPEED_PENALTY;
    }
  }
  
  // Accelerate toward the target with a maximum rate per axis.
  //TODO Different acceleration constants eg when riding the broom, standing on ice, ...
  // Will we ever need to accelerate at different rates toward vs away from zero?
  float limit=FMN_HERO_ACCELERATION*elapsed;
  float dx=tvx-fmn_hero.sprite->velx;
  if (dx>limit) dx=limit; else if (dx<-limit) dx=-limit;
  float dy=tvy-fmn_hero.sprite->vely;
  if (dy>limit) dy=limit; else if (dy<-limit) dy=-limit;
  fmn_hero.sprite->velx+=dx;
  fmn_hero.sprite->vely+=dy;
}

/* At the start of injury processing, consider whether we should nix it (eg due to Umbrella).
 * If yes, move hero to escape the collision, do any other required feedback, and return nonzero.
 * If no, return zero and caller should proceed with the injury.
 */
 
static uint8_t fmn_hero_suppress_injury(
  struct fmn_sprite *hero,float x,float y,struct fmn_sprite *assailant,uint8_t injury_dir
) {
  
  // First escape the collision. If we find now that no collision exists, allow the injury to proceed.
  float cx,cy;
  if (!fmn_physics_check_sprites(&cx,&cy,hero,assailant)) {
    return 0;
  }
  hero->x+=cx;
  hero->y+=cy;
  
  // Normalize correction vector and clamp current velocity so we're not continuing to advance into the hazard.
  float d=sqrtf(cx*cx+cy*cy);
  float nx=0.0f,ny=0.0f;
  if (d>0.0f) {
    nx=cx/d;
    ny=cy/d;
    if (((nx<0.0f)&&(hero->velx>0.0f))||((nx>0.0f)&&(hero->velx<0.0f))) hero->velx=0.0f;
    if (((ny<0.0f)&&(hero->vely>0.0f))||((ny>0.0f)&&(hero->vely<0.0f))) hero->vely=0.0f;
  } else {
    // Correction is impossibly small, just nix all velocity.
    hero->velx=0.0f;
    hero->vely=0.0f;
  }
  
  // Add a little velocity in the corrective direction.
  hero->velx+=nx*FMN_HERO_INJURY_SUPPRESSION_BOUNCEBACK;
  hero->vely+=ny*FMN_HERO_INJURY_SUPPRESSION_BOUNCEBACK;
  
  // Let the assailant react too.
  if (assailant&&assailant->interact) assailant->interact(assailant,FMN_ITEM_UMBRELLA,injury_dir);
  
  fmn_sound_effect(FMN_SFX_INJURY_DEFLECTED);
  
  return injury_dir;
}
 
static uint8_t fmn_hero_suppress_injury_if_applicable(
  struct fmn_sprite *hero,float x,float y,struct fmn_sprite *assailant
) {
  if (!assailant) return 0;
  
  if (fmn_global.active_item==FMN_ITEM_UMBRELLA) {
    uint8_t injury_dir=fmn_dir_from_vector_cardinal(x-hero->x,y-hero->y);
    if (injury_dir==fmn_global.facedir) return fmn_hero_suppress_injury(hero,x,y,assailant,injury_dir);
  }

  return 0;
}

/* After resetting to the map entry, check whether our position is valid and try to make it so if not.
 * This is a serious problem if you entered the map on a broom over water, then get reset.
 */
 
static void fmn_hero_attempt_valid_position() {
  struct fmn_sprite *hero=fmn_hero.sprite;
  
  // Offscreen is OK; she'll immediately bump to the neighbor map.
  if ((hero->x<0.0f)||(hero->y<0.0f)) return;
  if ((hero->x>=FMN_COLC)||(hero->y>=FMN_ROWC)) return;
  
  // If the cell is vacant, call it OK.
  // We're not checking for solid sprites or hazards. (TODO should we?)
  uint8_t col=hero->x,row=hero->y;
  uint8_t tile=fmn_global.map[row*FMN_COLC+col];
  uint8_t physics=fmn_global.cellphysics[tile];
  switch (physics) {
    case FMN_CELLPHYSICS_SOLID:
    case FMN_CELLPHYSICS_HOLE:
    case FMN_CELLPHYSICS_UNCHALKABLE:
    case FMN_CELLPHYSICS_SAP:
    case FMN_CELLPHYSICS_SAP_NOCHALK:
    case FMN_CELLPHYSICS_WATER:
    case FMN_CELLPHYSICS_REVELABLE:
      break;
    default: return; // "unknown" means vacant
  }
  
  // Fan out from current position along the edge we're on.
  // Abort if we're not actually on an edge.
  // Stop at any solid cell, don't consider ones on the other side of it.
  // Abort if nothing is vacant on this edge.
  // Otherwise take the nearest vacant tile.
  // If we're on a corner, both edges are valid.
  #define CHECKCELL(_x,_y,okflag) { \
    switch (fmn_global.cellphysics[fmn_global.map[_y*FMN_COLC+_x]]) { \
      case FMN_CELLPHYSICS_SOLID: \
      case FMN_CELLPHYSICS_UNCHALKABLE: \
      case FMN_CELLPHYSICS_SAP: \
      case FMN_CELLPHYSICS_SAP_NOCHALK: \
      case FMN_CELLPHYSICS_REVELABLE: { \
          /* solid, stop searching in this direction */ \
          okflag=0; \
        } break; \
      case FMN_CELLPHYSICS_HOLE: break; \
      case FMN_CELLPHYSICS_WATER: break; \
      default: { /* vacant */ \
          hero->x=_x+0.5f; \
          hero->y=_y+0.5f; \
          return; \
        } \
    } \
  }
  if ((col==0)||(col==FMN_COLC-1)) {
    uint8_t upok=1,downok=1;
    int8_t d=1; for (;d<FMN_ROWC;d++) {
      int8_t qrow=row+d;
      if (upok&&(qrow<FMN_ROWC)) CHECKCELL(col,qrow,upok)
      qrow=row-d;
      if (downok&&(qrow>=0)) CHECKCELL(col,qrow,downok)
    }
  }
  if ((row==0)||(row==FMN_ROWC-1)) {
    uint8_t leftok=1,rightok=1;
    int8_t d=1; for (;d<FMN_COLC;d++) {
      int8_t qcol=col+d;
      if (leftok&&(qcol<FMN_COLC)) CHECKCELL(qcol,row,leftok)
      qcol=col-d;
      if (rightok&&(qcol>=0)) CHECKCELL(qcol,row,rightok)
    }
  }
  #undef CHECKCELL
  
  // Panic! Drop whatever is happening and put her back on the broom.
  // It's fair to assume that she has one; that's the only way she could get here.
  // But even if not, this will be less weird than getting stuck on an edge. The fake broom is gone once she lands.
  fmn_global.selected_item=FMN_ITEM_BROOM;
  fmn_global.active_item=FMN_ITEM_BROOM;
  fmn_hero.sprite->physics&=~FMN_PHYSICS_HOLE;
  fmn_hero.landing_pending=1;
}

/* Return to map entry.
 * Right now this will only happen due to multiple injuries.
 */
 
void fmn_hero_return_to_map_entry() {
  struct fmn_sprite *hero=fmn_hero.sprite;
  fmn_sprite_generate_soulballs(hero->x,hero->y,7);
  fmn_global.injury_time=FMN_HERO_INJURY_TIME;
  fmn_hero_item_end();
  fmn_sound_effect(FMN_SFX_GRIEVOUS_INJURY);
  fmn_hero.velx=hero->velx=0.0f;
  fmn_hero.vely=hero->vely=0.0f;
  hero->x=fmn_hero.enterx;
  hero->y=fmn_hero.entery;
  int8_t dumx,dumy;
  fmn_hero_get_quantized_position(&dumx,&dumy);
  fmn_hero_walk_end();
  fmn_hero_attempt_valid_position();
}

/* Begin injury.
 */
 
uint8_t fmn_hero_injure(float x,float y,struct fmn_sprite *assailant) {
  struct fmn_sprite *hero=fmn_hero.sprite;
  
  if (fmn_hero_suppress_injury_if_applicable(hero,x,y,assailant)) return 0;
  
  if (fmn_global.injury_time>0.0f) {
    float time_since=FMN_HERO_INJURY_TIME-fmn_global.injury_time;
    if (time_since<FMN_HERO_INJURY_BLANK_TIME) {
      return 0;
    } else if (time_since<FMN_HERO_DOUBLE_INJURY_TIME) {
      fmn_hero_return_to_map_entry();
      return 1;
    }
  }
  
  // Getting injured implicitly ends any action in flight (broom, in particular).
  // Player won't be able to restart it until the injury exposure is complete.
  fmn_hero_item_end();
  
  // First nice and simple, apply a constant force in the opposite direction of the assailant.
  float dx=hero->x-x;
  float dy=hero->y-y;
  float distance2=dx*dx+dy*dy;
  float distance=sqrtf(distance2);
  float nx,ny;
  if (distance<=0.0f) {
    // Exactly atop each other, the hero corrects upward, no particular reason.
    nx=0.0f;
    ny=-1.0f;
  } else {
    nx=dx/distance;
    ny=dy/distance;
  }
  hero->velx+=nx*FMN_HERO_INJURY_VELOCITY;
  hero->vely+=ny*FMN_HERO_INJURY_VELOCITY;
  
  /* If we're still pointing at the assailant, flip sign.
   * This is unusual, and it probably means we're ping-ponging between two hazards.
   * It is an important business requirement that the hero be forced away from any hazard, we'll use them like walls.
   */
  if (((hero->velx<0.0f)&&(dx>0.0f))||((hero->velx>0.0f)&&(dx<0.0f))) hero->velx=-hero->velx;
  if (((hero->vely<0.0f)&&(dy>0.0f))||((hero->vely>0.0f)&&(dy<0.0f))) hero->vely=-hero->vely;
  
  /* Calculate my velocity along the line thru assailant, and clamp it to a constant minimum.
   */
  float nvel=hero->velx*nx+hero->vely*ny;
  if (nvel<FMN_HERO_INJURY_MIN) {
    if (nvel<=0.0f) {
           if (dx<0.0f) { hero->velx=-FMN_HERO_INJURY_MIN; hero->vely=0.0f; }
      else if (dx>0.0f) { hero->velx= FMN_HERO_INJURY_MIN; hero->vely=0.0f; }
      else if (dy<0.0f) { hero->vely=-FMN_HERO_INJURY_MIN; hero->velx=0.0f; }
      else              { hero->vely= FMN_HERO_INJURY_MIN; hero->velx=0.0f; }
    } else {
      float scale=FMN_HERO_INJURY_MIN/nvel;
      hero->velx*=scale;
      hero->vely*=scale;
    }
  }
  
  /* Finally, clamp my true velocity to a constant maximum, in whatever direction it ended up.
   */
  float vel2=hero->velx*hero->velx+hero->vely*hero->vely;
  if (vel2>FMN_HERO_INJURY_MAX*FMN_HERO_INJURY_MAX) {
    nvel=sqrtf(vel2);
    float scale=FMN_HERO_INJURY_MAX/nvel;
    hero->velx*=scale;
    hero->vely*=scale;
  }
  
  fmn_global.injury_time=FMN_HERO_INJURY_TIME;

  fmn_sound_effect(FMN_SFX_HURT);
  return 1;
}

/* Curse.
 * Same idea as injury, but much simpler because no motion is involved.
 */
 
uint8_t fmn_hero_curse(struct fmn_sprite *assailant) {
  if (fmn_global.curse_time>0.0f) return 0;
  fmn_global.curse_time=FMN_HERO_CURSE_TIME;
  fmn_sound_effect(FMN_SFX_CURSE);
  fmn_hero_item_end();
  return 1;
}

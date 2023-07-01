#include "app/sprite/fmn_sprite.h"
#include "app/sprite/fmn_physics.h"
#include "app/fmn_game.h"

#define FARMER_STAGE_HOME 1 /* inside the house, hidden */
#define FARMER_STAGE_TRAVEL 2 /* walking to the plant, or back home */
#define FARMER_STAGE_HOLD 3 /* standing still at the job site, before taking action */
#define FARMER_STAGE_PRIDE 4 /* '' after taking action */
#define FARMER_STAGE_DIG 5 /* shovel animation */
#define FARMER_STAGE_WATER 6 /* pitcher animation */

#define FARMER_WALK_SPEED 1.0f

#define clock sprite->fv[0]
#define stagetime sprite->fv[1]
#define dy sprite->fv[2] /* FARMER_STAGE_TRAVEL; -1 or 1 */
#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define stage sprite->bv[2]
#define physics0 sprite->bv[3]
#define hole_distance sprite->argv[0]

/* If we were to resume normal physics at current position, would there be a collision?
 */
 
static int farmer_test_collisions_1(struct fmn_sprite *pumpkin,void *userdata) {
  struct fmn_sprite *sprite=userdata;
  if (sprite==pumpkin) return 0;
  if (!(pumpkin->physics&FMN_PHYSICS_SPRITES)) return 0;
  if (fmn_physics_check_sprites(0,0,pumpkin,sprite)) return 1;
  return 0;
}
 
static uint8_t farmer_test_collisions(struct fmn_sprite *sprite) {
  if (fmn_physics_check_edges(0,0,sprite)) return 1;
  float oradius=sprite->radius;
  sprite->radius=0.2f;
  if (fmn_physics_check_grid(0,0,sprite,physics0)) {
    sprite->radius=oradius;
    return 1;
  }
  sprite->radius=oradius;
  return fmn_sprites_for_each(farmer_test_collisions_1,sprite);
}

/* Jobs we can perform.
 * These modify world state but not sprite state.
 * Assume all are fallible; they return nonzero on success.
 */
 
static uint8_t farmer_dig_hole(struct fmn_sprite *sprite,int8_t col,int8_t row) {
  if ((col<0)||(col>=FMN_COLC)||(row<0)||(row>=FMN_ROWC)) return 0;
  if (fmn_global.cellphysics[fmn_global.map[row*FMN_COLC+col]]!=FMN_CELLPHYSICS_VACANT) return 0;
  fmn_sound_effect(FMN_SFX_DIG);
  fmn_global.map[row*FMN_COLC+col]=0x0f;
  fmn_map_dirty();
  return 1;
}

static uint8_t farmer_sow_plant(struct fmn_sprite *sprite,int8_t col,int8_t row) {
  if (fmn_add_plant(col,row)<0) {
    fmn_log_event("plant-rejected","%d,%d",col,row);
    return 0;
  } else {
    fmn_log_event("plant","%d,%d",col,row);
    fmn_sound_effect(FMN_SFX_PLANT);
    fmn_map_dirty();
    return 1;
  }
}

static uint8_t farmer_water_plant(struct fmn_sprite *sprite,struct fmn_plant *plant) {
  if (!plant||(plant->state!=FMN_PLANT_STATE_SEED)) return 0;
  fmn_sound_effect(FMN_SFX_SPROUT);
  plant->state=FMN_PLANT_STATE_GROW;
  plant->fruit=FMN_PLANT_FRUIT_SEED;
  plant->flower_time=fmn_game_get_platform_time_ms()+FMN_FLOWER_TIME_MS;
  fmn_map_dirty();
  return 1;
}

/* Specific stage transitions, no decision-making.
 */
 
static void farmer_begin_HOME(struct fmn_sprite *sprite) {
  clock=0.0f;
  stagetime=5.0f;
  stage=FARMER_STAGE_HOME;
  sprite->style=FMN_SPRITE_STYLE_HIDDEN;
  sprite->physics=0;
}

static void farmer_begin_TRAVEL(struct fmn_sprite *sprite,float _dy) {
  clock=0.0f;
  stagetime=(hole_distance-0.750f)/FARMER_WALK_SPEED;//TODO was originally -0.750 going out and -0.5 returning. tweak to taste
  stage=FARMER_STAGE_TRAVEL;
  dy=_dy;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  if (dy<0.0f) sprite->tileid=tileid0+6;
  else sprite->tileid=tileid0+0;
  sprite->physics=physics0;
}

static void farmer_begin_HOLD(struct fmn_sprite *sprite) {
  clock=0.0f;
  stagetime=0.5f;
  stage=FARMER_STAGE_HOLD;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->tileid=tileid0+0;
  sprite->physics=physics0;
}

static void farmer_begin_PRIDE(struct fmn_sprite *sprite) {
  clock=0.0f;
  stagetime=1.0f;
  stage=FARMER_STAGE_PRIDE;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->tileid=tileid0+0;
  sprite->physics=physics0;
}

static void farmer_begin_DIG(struct fmn_sprite *sprite) {
  clock=0.0f;
  stagetime=1.0f;
  stage=FARMER_STAGE_DIG;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->tileid=tileid0+3;
  sprite->physics=physics0;
}

static void farmer_begin_WATER(struct fmn_sprite *sprite) {
  clock=0.0f;
  stagetime=1.0f;
  stage=FARMER_STAGE_WATER;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->tileid=tileid0+5;
  sprite->physics=physics0;
}

/* Leaving TRAVEL stage.
 * Enter either HOME or HOLD, depending on which way we were moving.
 */
 
static void farmer_arrive_at_destination(struct fmn_sprite *sprite) {
  if (dy<0.0f) farmer_begin_HOME(sprite);
  else farmer_begin_HOLD(sprite);
}

/* Leaving HOME stage.
 * Normally we'll enter TRAVEL stage with (dy) positive.
 * But if we can see no action is possible, stay put. (ie reenter HOME stage)
 * The final decision of what to do (dig, plant, water, nothing) gets made after we reach the plant, not here.
 */
 
static void farmer_consider_leaving_home(struct fmn_sprite *sprite) {
  if (farmer_test_collisions(sprite)) {
    farmer_begin_HOME(sprite);
    return;
  }
  int8_t col=sprite->x,row=sprite->y;
  row+=hole_distance;
  if ((col<0)||(col>=FMN_COLC)||(row<0)||(row>=FMN_ROWC)) {
    farmer_begin_HOME(sprite);
    return;
  }
  // Find existing plant at my work site.
  const struct fmn_plant *plant=0;
  const struct fmn_plant *q=fmn_global.plantv;
  int i=fmn_global.plantc;
  for (;i-->0;q++) {
    if (q->state==FMN_PLANT_STATE_NONE) continue;
    if (q->x!=col) continue;
    if (q->y!=row) continue;
    plant=q;
    break;
  }
  if (plant) switch (plant->state) {
    case FMN_PLANT_STATE_SEED: break; // We can water it, this is ok.
    default: {
        farmer_begin_HOME(sprite);
        return;
      }
  }
  farmer_begin_TRAVEL(sprite,1.0f);
}

/* Arrived at work site, and done waiting for the HOLD period.
 * Begin doing whatever work is currently possible.
 */
 
static void farmer_consider_working(struct fmn_sprite *sprite) {
  int8_t col=sprite->x,row=sprite->y;
  row++; // we're facing south
  if ((col<0)||(col>=FMN_COLC)||(row<0)||(row>=FMN_ROWC)) {
    farmer_begin_TRAVEL(sprite,-1.0f);
    return;
  }
  uint8_t tileid=fmn_global.map[row*FMN_COLC+col];
  uint8_t cellphysics=fmn_global.cellphysics[tileid];
  if (cellphysics==FMN_CELLPHYSICS_VACANT) {
    // cool
  } else if (tileid==0x0f) {
    // pre-dug hole (cellphysics should be UNSHOVELLABLE). cool
  } else {
    farmer_begin_TRAVEL(sprite,-1.0f);
    return;
  }
  struct fmn_plant *plant=0,*q=fmn_global.plantv;
  int i=fmn_global.plantc;
  for (;i-->0;q++) {
    if (q->state==FMN_PLANT_STATE_NONE) continue;
    if (q->x!=col) continue;
    if (q->y!=row) continue;
    plant=q;
    break;
  }
  // Decide what to do, do it, and enter the appropriate next stage.
  if (plant) {
    if (plant->state==FMN_PLANT_STATE_SEED) {
      if (farmer_water_plant(sprite,plant)) {
        farmer_begin_WATER(sprite);
      } else {
        farmer_begin_TRAVEL(sprite,-1.0f);
      }
    } else {
      farmer_begin_TRAVEL(sprite,-1.0f);
    }
  } else if (tileid==0x0f) {
    if (farmer_sow_plant(sprite,col,row)) {
      farmer_begin_TRAVEL(sprite,-1.0f);
    } else {
      farmer_begin_TRAVEL(sprite,-1.0f);
    }
  } else {
    if (farmer_dig_hole(sprite,col,row)) {
      farmer_begin_DIG(sprite);
    } else {
      farmer_begin_TRAVEL(sprite,-1.0f);
    }
  }
}

/* Stage transition.
 */
 
static void farmer_advance(struct fmn_sprite *sprite) {
  switch (stage) {
    case FARMER_STAGE_HOME: farmer_consider_leaving_home(sprite); break;
    case FARMER_STAGE_TRAVEL: farmer_arrive_at_destination(sprite); break;
    case FARMER_STAGE_HOLD: farmer_consider_working(sprite); break;
    case FARMER_STAGE_PRIDE: farmer_begin_TRAVEL(sprite,-1.0f); break;
    case FARMER_STAGE_DIG: farmer_begin_HOLD(sprite); break; // dig and plant on the same trip
    case FARMER_STAGE_WATER: farmer_begin_PRIDE(sprite); break;
  }
}

/* Update, TRAVEL.
 */
 
static void farmer_update_TRAVEL(struct fmn_sprite *sprite,float elapsed) {
  sprite->y+=dy*FARMER_WALK_SPEED*elapsed;
  uint8_t tilebase=tileid0+((dy<0.0f)?6:0);
  switch (((int)(clock*5.0f))%4) {
    case 0: sprite->tileid=tilebase+0; break;
    case 1: sprite->tileid=tilebase+1; break;
    case 2: sprite->tileid=tilebase+0; break;
    case 3: sprite->tileid=tilebase+2; break;
  }
}

/* Update DIG and WATER stages, animation only.
 */
 
static void farmer_update_DIG(struct fmn_sprite *sprite,float elapsed) {
  if (clock>=0.5f) sprite->tileid=tileid0+4;
  else sprite->tileid=tileid0+3;
}

static void farmer_update_WATER(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=tileid0+5;
}

/* Update.
 */
 
static void _farmer_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    sprite->tileid=tileid0+9;
    sprite->style=FMN_SPRITE_STYLE_TILE;
    return;
  }
  clock+=elapsed;
  if (clock>=stagetime) {
    farmer_advance(sprite);
  } else switch (stage) {
    case FARMER_STAGE_TRAVEL: farmer_update_TRAVEL(sprite,elapsed); break;
    case FARMER_STAGE_DIG: farmer_update_DIG(sprite,elapsed); break;
    case FARMER_STAGE_WATER: farmer_update_WATER(sprite,elapsed); break;
    // HOME, HOLD, PRIDE don't require any update; most action takes place at stage transitions.
  }
}

/* Interact.
 */
 
static int16_t _farmer_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: if (!sleeping&&(sprite->style!=FMN_SPRITE_STYLE_HIDDEN)) { sleeping=1; fmn_sprite_generate_zzz(sprite); } break;
        case FMN_SPELLID_REVEILLE: sleeping=0; break;
        case FMN_SPELLID_PUMPKIN: fmn_sprite_pumpkinize(sprite); break;
      } break;
    case FMN_ITEM_BELL: sleeping=0; break;
  }
  return 0;
}

/* Init.
 */
 
static void _farmer_init(struct fmn_sprite *sprite) {
  if (hole_distance<2) {
    fmn_log("invalid hole_distance %d for farmer",hole_distance);
    fmn_sprite_kill(sprite);
    return;
  }
  clock=0.0f;
  tileid0=sprite->tileid;
  physics0=sprite->physics;
  farmer_begin_HOME(sprite);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_farmer={
  .init=_farmer_init,
  .update=_farmer_update,
  .interact=_farmer_interact,
};

#include "app/sprite/fmn_sprite.h"
#include "app/sprite/fmn_physics.h"

#define FARMER_STAGE_INDOORS 1
#define FARMER_STAGE_GO_OUT 2
#define FARMER_STAGE_DIG 3
#define FARMER_STAGE_HOME_MID 4
#define FARMER_STAGE_WAIT_MID 5
#define FARMER_STAGE_OUT_MID 6
#define FARMER_STAGE_SOW 7
#define FARMER_STAGE_WATER 8
#define FARMER_STAGE_GO_HOME 9

#define FARMER_WALK_SPEED 1.0f

#define clock sprite->fv[0]
#define stagetime sprite->fv[1]
#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define stage sprite->bv[2]
#define physics0 sprite->bv[3]
#define hole_distance sprite->argv[0]

/* INDOOR and WAIT_MID stage: Hidden, do nothing.
 */
 
static void farmer_begin_INDOORS(struct fmn_sprite *sprite) {
  clock=0.0f;
  stagetime=5.0f;
  stage=FARMER_STAGE_INDOORS;
  sprite->style=FMN_SPRITE_STYLE_HIDDEN;
  sprite->physics=0;
}
 
static void farmer_begin_WAIT_MID(struct fmn_sprite *sprite) {
  clock=0.0f;
  stagetime=3.0f;
  stage=FARMER_STAGE_WAIT_MID;
  sprite->style=FMN_SPRITE_STYLE_HIDDEN;
  sprite->physics=0;
}

/* GO_OUT and OUT_MID: Walk down to the plant.
 */
 
static void farmer_begin_GO_OUT(struct fmn_sprite *sprite) {
  clock=0.0f;
  stagetime=(hole_distance-0.750f)/FARMER_WALK_SPEED;
  stage=FARMER_STAGE_GO_OUT;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->physics=physics0;
  sprite->tileid=tileid0;
}

static void farmer_update_GO_OUT(struct fmn_sprite *sprite,float elapsed) {
  sprite->y+=FARMER_WALK_SPEED*elapsed;
  switch (((int)(clock*5.0f))%4) {
    case 0: sprite->tileid=tileid0+0; break;
    case 1: sprite->tileid=tileid0+1; break;
    case 2: sprite->tileid=tileid0+0; break;
    case 3: sprite->tileid=tileid0+2; break;
  }
}
 
static void farmer_begin_OUT_MID(struct fmn_sprite *sprite) {
  clock=0.0f;
  stagetime=(hole_distance-0.750f)/FARMER_WALK_SPEED;
  stage=FARMER_STAGE_OUT_MID;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->physics=physics0;
  sprite->tileid=tileid0;
}

#define farmer_update_OUT_MID farmer_update_GO_OUT

/* GO_HOME and HOME_MID: Walk back into the door where we started.
 */
 
static void farmer_begin_GO_HOME(struct fmn_sprite *sprite) {
  clock=0.0f;
  stagetime=(hole_distance-0.5f)/FARMER_WALK_SPEED;
  stage=FARMER_STAGE_GO_HOME;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->physics=physics0;
  sprite->tileid=tileid0+6;
}

static void farmer_update_GO_HOME(struct fmn_sprite *sprite,float elapsed) {
  sprite->y-=FARMER_WALK_SPEED*elapsed;
  switch (((int)(clock*5.0f))%4) {
    case 0: sprite->tileid=tileid0+6; break;
    case 1: sprite->tileid=tileid0+7; break;
    case 2: sprite->tileid=tileid0+6; break;
    case 3: sprite->tileid=tileid0+8; break;
  }
}
 
static void farmer_begin_HOME_MID(struct fmn_sprite *sprite) {
  clock=0.0f;
  stagetime=(hole_distance-0.5f)/FARMER_WALK_SPEED;
  stage=FARMER_STAGE_HOME_MID;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->physics=physics0;
  sprite->tileid=tileid0+6;
}

#define farmer_update_HOME_MID farmer_update_GO_HOME

/* DIG: Create the hole and animate.
 */
 
static void farmer_begin_DIG(struct fmn_sprite *sprite,int8_t col,int8_t row) {
  clock=0.0f;
  stagetime=1.0f;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->physics=physics0;
  stage=FARMER_STAGE_DIG;
  fmn_sound_effect(FMN_SFX_DIG);
  fmn_global.map[row*FMN_COLC+col]=0x0f;
  fmn_map_dirty();
  sprite->tileid=tileid0+3;
}

static void farmer_update_DIG(struct fmn_sprite *sprite,float elapsed) {
  if (clock>=0.5f) {
    sprite->tileid=tileid0+4;
  } else {
    sprite->tileid=tileid0+3;
  }
}

/* SOW: Create the plant.
 */
 
static void farmer_begin_SOW(struct fmn_sprite *sprite,int8_t col,int8_t row) {
  fmn_sound_effect(FMN_SFX_PLANT);
  clock=0.0f;
  stagetime=0.5f;
  stage=FARMER_STAGE_SOW;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->physics=physics0;
  sprite->tileid=tileid0;
  if (fmn_add_plant(col,row)<0) {
    farmer_begin_GO_HOME(sprite);
  }
}

static void farmer_update_SOW(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=tileid0;
}

/* WATER: Water the plant.
 */
 
static void farmer_begin_WATER(struct fmn_sprite *sprite,struct fmn_plant *plant) {
  fmn_sound_effect(FMN_SFX_SPROUT);
  plant->state=FMN_PLANT_STATE_GROW;
  plant->fruit=FMN_PLANT_FRUIT_SEED;
  fmn_map_dirty();
  clock=0.0f;
  stagetime=1.0f;
  stage=FARMER_STAGE_WATER;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->physics=physics0;
  sprite->tileid=tileid0+5;
}

static void farmer_update_WATER(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=tileid0+5;
}

/* Init.
 */
 
static void _farmer_init(struct fmn_sprite *sprite) {
  clock=0.0f;
  tileid0=sprite->tileid;
  physics0=sprite->physics;
  farmer_begin_INDOORS(sprite);
}

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

/* Advance to next stage.
 */
 
static void farmer_advance(struct fmn_sprite *sprite) {
  switch (stage) {
    case FARMER_STAGE_INDOORS: {
        // If we were configured with a bad hole_distance, stay put.
        int8_t col=(int8_t)sprite->x;
        int8_t row=(int8_t)(sprite->y+0.5f)+hole_distance;
        if ((hole_distance<2)||(col<0)||(col>=FMN_COLC)||(row<0)||(row>=FMN_ROWC)) {
          farmer_begin_INDOORS(sprite);
          return;
        }
        // If there's a plant on my target zone in GROW, FLOWER, or DEAD state, stay put.
        const struct fmn_plant *plant=fmn_global.plantv;
        uint8_t i=fmn_global.plantc;
        for (;i-->0;plant++) {
          if (plant->x!=col) continue;
          if (plant->y!=row) continue;
          if (
            (plant->state==FMN_PLANT_STATE_GROW)||
            (plant->state==FMN_PLANT_STATE_FLOWER)||
            (plant->state==FMN_PLANT_STATE_DEAD)
          ) {
            farmer_begin_INDOORS(sprite);
            return;
          }
        }
        // Finally, don't try to leave home if something is blocking the door.
        if (farmer_test_collisions(sprite)) {
          farmer_begin_INDOORS(sprite);
          return;
        }
        farmer_begin_GO_OUT(sprite);
      } break;
    case FARMER_STAGE_GO_OUT: {
        // Confirm that we're looking down at a plantable tile. If not, go home.
        // If there is a plant here that needs watered, pretend we were OUT_MID rather than GO_OUT.
        int8_t col=(int8_t)sprite->x;
        int8_t row=(int8_t)(sprite->y+0.5f);
        if ((col<0)||(row<0)||(col>=FMN_COLC)||(row>=FMN_ROWC)||(fmn_global.cellphysics[fmn_global.map[row*FMN_COLC+col]]!=FMN_CELLPHYSICS_VACANT)) {
          farmer_begin_GO_HOME(sprite);
          return;
        }
        uint8_t must_water=0;
        const struct fmn_plant *plant=fmn_global.plantv;
        uint8_t i=fmn_global.plantc;
        for (;i-->0;plant++) {
          if ((plant->x==col)&&(plant->y==row)) {
            if (plant->state==FMN_PLANT_STATE_SEED) {
              must_water=1;
            } else {
              farmer_begin_GO_HOME(sprite);
              return;
            }
          }
        }
        if (must_water) goto _begin_water_;
        farmer_begin_DIG(sprite,col,row);
      } break;
    case FARMER_STAGE_DIG: {
        // Check again that there's no plant here, and that it's still dug.
        int8_t col=(int8_t)sprite->x;
        int8_t row=(int8_t)(sprite->y+0.5f);
        if ((col<0)||(row<0)||(col>=FMN_COLC)||(row>=FMN_ROWC)||(fmn_global.map[row*FMN_COLC+col]!=0x0f)) {
          farmer_begin_GO_HOME(sprite);
          return;
        }
        const struct fmn_plant *plant=fmn_global.plantv;
        uint8_t i=fmn_global.plantc;
        for (;i-->0;plant++) {
          if ((plant->x==col)&&(plant->y==row)) {
            farmer_begin_HOME_MID(sprite);
            return;
          }
        }
        farmer_begin_SOW(sprite,col,row);
      } break;
    case FARMER_STAGE_SOW: {
        farmer_begin_HOME_MID(sprite);
      } break;
    case FARMER_STAGE_HOME_MID: {
        farmer_begin_WAIT_MID(sprite);
      } break;
    case FARMER_STAGE_WAIT_MID: {
        farmer_begin_OUT_MID(sprite);
      } break;
    case FARMER_STAGE_OUT_MID: _begin_water_: {
        // Confirm there's a plant here and it's not watered yet.
        int8_t col=(int8_t)sprite->x;
        int8_t row=(int8_t)(sprite->y+0.5f);
        struct fmn_plant *plant=fmn_global.plantv;
        uint8_t i=fmn_global.plantc;
        for (;i-->0;plant++) {
          if ((plant->x==col)&&(plant->y==row)&&(plant->state==FMN_PLANT_STATE_SEED)) {
            farmer_begin_WATER(sprite,plant);
            return;
          }
        }
        farmer_begin_GO_HOME(sprite);
      } break;
    case FARMER_STAGE_GO_HOME: {
        farmer_begin_INDOORS(sprite);
      } break;
    default: farmer_begin_GO_HOME(sprite);
  }
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
    case FARMER_STAGE_GO_OUT: farmer_update_GO_OUT(sprite,elapsed); break;
    case FARMER_STAGE_DIG: farmer_update_DIG(sprite,elapsed); break;
    case FARMER_STAGE_SOW: farmer_update_SOW(sprite,elapsed); break;
    case FARMER_STAGE_HOME_MID: farmer_update_HOME_MID(sprite,elapsed); break;
    case FARMER_STAGE_OUT_MID: farmer_update_OUT_MID(sprite,elapsed); break;
    case FARMER_STAGE_WATER: farmer_update_WATER(sprite,elapsed); break;
    case FARMER_STAGE_GO_HOME: farmer_update_GO_HOME(sprite,elapsed); break;
  }
}

/* Interact.
 */
 
static int16_t _farmer_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: if (!sleeping&&(sprite->style!=FMN_SPRITE_STYLE_HIDDEN)) { sleeping=1; fmn_sprite_generate_zzz(sprite); } break;
        case FMN_SPELLID_REVEILLE: sleeping=0; break;
      } break;
    case FMN_ITEM_BELL: sleeping=0; break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_farmer={
  .init=_farmer_init,
  .update=_farmer_update,
  .interact=_farmer_interact,
};

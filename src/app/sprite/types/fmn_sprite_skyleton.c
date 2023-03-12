/* fmn_sprite_skyleton.c
 * When you place one of these, it creates a singleton skyleton generator.
 * The location of that trigger sprite is not important.
 */
 
#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define SKYLETON_SPAWN_PERIOD 3.0f
#define SKYLETON_SPAWN_PHASE 2.0f
#define SKYLETON_INSTANCE_LIMIT 3

#define SKYLETON_STAGE_FALL 1 /* two sprites, both ornamental: a growing shadow and a falling skeleton */
#define SKYLETON_STAGE_BOUNCE 2 /* stand still for a sec right after landing */
#define SKYLETON_STAGE_CHARGE 3 /* run toward the wall */
#define SKYLETON_STAGE_BREAK 4 /* fall apart after crashing into the wall */

#define SKYLETON_FALL_LEAD_TIME 1.0f /* shadow becomes visible for a little bit before we start falling, as a warning */
#define SKYLETON_FALL_X_CORRECT_SPEED 1.0f
#define SKYLETON_FALL_Y_SPEED 6.0f
#define SKYLETON_BOUNCE_TIME 0.5f
#define SKYLETON_CHARGE_SPEED 6.0f
#define SKYLETON_CHARGE_ACCEL 5.0f
#define SKYLETON_CHARGE_FRAME_TIME 0.125f
#define SKYLETON_BREAK_FRAME_TIME 0.125f

#define stage sprite->bv[0]
#define sclock sprite->fv[0]

/* Class state.
 */
 
static struct skyleton_state {
  float clock;
  struct fmn_sprite template;
} skyleton_state;

/* Cleanup, class level.
 */
 
static void _skyleton_class_cleanup(void *userdata) {
}

/* Instantiate.
 */
 
static void skyleton_instantiate() {

  // Select a starting point on the same row as the hero, somewhere there are three adjacent vacant cells.
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if ((heroy<0.0f)||(heroy>=FMN_ROWC)) return;
  uint8_t row=heroy;
  float y=row+0.5f;
  uint8_t xcandidatev[FMN_COLC];
  uint8_t xcandidatec=0;
  const uint8_t *mapsrc=fmn_global.map+row*FMN_COLC;
  uint8_t col=FMN_COLC-1;
  #define ISVACANT(tileid) ({ \
    uint8_t vacant=0; \
    switch (fmn_global.cellphysics[tileid]) { \
      case FMN_CELLPHYSICS_VACANT: \
      case FMN_CELLPHYSICS_UNSHOVELLABLE: \
        vacant=1; \
    } \
    vacant; \
  })
  for (;col-->1;) {
    if (ISVACANT(mapsrc[col])&&ISVACANT(mapsrc[col-1])&&ISVACANT(mapsrc[col+1])) {
      xcandidatev[xcandidatec++]=col;
    }
  }
  #undef ISVACANT
  if (xcandidatec<1) {
    return;
  }
  col=xcandidatev[rand()%xcandidatec];
  float x=col+0.5f;

  struct fmn_sprite *sprite=fmn_sprite_generate_noparam(FMN_SPRCTL_skyleton,x,-1.0f);
  if (!sprite) return;
  memcpy(sprite,&skyleton_state.template,sizeof(struct fmn_sprite));
  sprite->x=x;
  sprite->y=-1.0f;
  stage=SKYLETON_STAGE_FALL;
  sprite->physics=0;
  sprite->layer=200;
  if (herox<sprite->x) sprite->xform=FMN_XFORM_XREV;
  
  struct fmn_sprite *shadow=fmn_sprite_generate_noparam(0,x,y);
  if (!shadow) {
    fmn_sprite_kill(sprite);
    return;
  }
  shadow->style=FMN_SPRITE_STYLE_TILE;
  shadow->layer=1;
  shadow->imageid=3;
  shadow->tileid=0xab;
  sprite->pv[0]=shadow;
}

/* Count instances.
 */
 
static int skyleton_count_instances_1(struct fmn_sprite *sprite,void *userdata) {
  if (sprite->controller==FMN_SPRCTL_skyleton) {
    (*(uint8_t*)userdata)++;
  }
  return 0;
}
 
static uint8_t skyleton_count_instances() {
  uint8_t c=0;
  fmn_sprites_for_each(skyleton_count_instances_1,&c);
  return c;
}

/* Update, class level.
 */
 
static void _skyleton_class_update(void *userdata,float elapsed) {
  skyleton_state.clock+=elapsed;
  if (skyleton_state.clock>=SKYLETON_SPAWN_PERIOD) {
    skyleton_state.clock-=SKYLETON_SPAWN_PERIOD;
    uint8_t instancec=skyleton_count_instances();
    if (instancec<SKYLETON_INSTANCE_LIMIT) {
      skyleton_instantiate();
    }
  }
}

/* Kill instance.
 */
 
static void skyleton_kill(struct fmn_sprite *sprite) {
  if (sprite->pv[0]) fmn_sprite_kill(sprite->pv[0]);
  fmn_sprite_kill(sprite);
}

/* Spear hero.
 */
 
static void skyleton_sword_hero_collision(struct fmn_sprite *sprite,struct fmn_sprite *hero) {
  // Cheat the horz position inward a little, otherwise it can get past the umbrella.
  float x=sprite->x;
  if (sprite->xform&FMN_XFORM_XREV) x+=0.5f;
  else x-=0.5f;
  fmn_hero_injure(x,sprite->y,sprite);
}

/* Enter BOUNCE stage.
 */
 
static void skyleton_begin_BOUNCE(struct fmn_sprite *sprite) {
  fmn_sound_effect(FMN_SFX_THUD);
  sprite->tileid=skyleton_state.template.tileid+0x01;
  sprite->physics=skyleton_state.template.physics;
  sprite->layer=128;
  sclock=0.0f;
  stage=SKYLETON_STAGE_BOUNCE;
  if (sprite->pv[0]) {
    fmn_sprite_kill(sprite->pv[0]);
    sprite->pv[0]=0;
  }
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (herox<sprite->x) {
    sprite->xform=FMN_XFORM_XREV;
  }
}

/* Enter CHARGE stage.
 */
 
static void skyleton_begin_CHARGE(struct fmn_sprite *sprite) {
  sprite->tileid=skyleton_state.template.tileid+0x02;
  stage=SKYLETON_STAGE_CHARGE;
  sclock=0.0f;
  
  // Sword is offset by 0.875 tiles: There's two pixels of handle that should not be seen during the charge.
  float x=sprite->x;
  if (sprite->xform&FMN_XFORM_XREV) x-=0.875f;
  else x+=0.875f;
  struct fmn_sprite *sword=fmn_sprite_generate_noparam(0,x,sprite->y);
  if (sword) {
    sword->imageid=sprite->imageid;
    sword->tileid=skyleton_state.template.tileid+6;
    sword->xform=sprite->xform;
    sword->layer=sprite->layer-1;
    sword->style=FMN_SPRITE_STYLE_TILE;
    sword->radius=0.25f;
    sword->hero_collision=skyleton_sword_hero_collision;
    sprite->pv[0]=sword;
  }
}

/* Enter BREAK stage.
 */
 
static void skyleton_begin_BREAK(struct fmn_sprite *sprite) {
  fmn_sound_effect(FMN_SFX_BREAK_BONES);
  sprite->physics=0;
  sclock=0.0f;
  sprite->tileid=skyleton_state.template.tileid+7;
  stage=SKYLETON_STAGE_BREAK;
  if (sprite->pv[0]) {
    fmn_sprite_kill(sprite->pv[0]);
    sprite->pv[0]=0;
  }
}

/* Update FALL.
 */
 
static void skyleton_update_FALL(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=skyleton_state.template.tileid;
  struct fmn_sprite *shadow=sprite->pv[0];
  if (!shadow) {
    skyleton_begin_BOUNCE(sprite);
    return;
  }
  // Shadow's tile there are 5 options. Select proportionately based on our vertical positions.
  int8_t shadowtile=(sprite->y*5.0f)/shadow->y;
  if (shadowtile<0) shadowtile=0;
  else if (shadowtile>4) shadowtile=4;
  shadow->tileid=0xab+shadowtile;
  // Match the shadow's X position. Shouldn't ever be off, but who knows.
  if (sprite->x<shadow->x) {
    if ((sprite->x+=SKYLETON_FALL_X_CORRECT_SPEED*elapsed)>=shadow->x) {
      sprite->x=shadow->x;
    }
  } else if (sprite->x>shadow->x) {
    if ((sprite->x-=SKYLETON_FALL_X_CORRECT_SPEED*elapsed)<=shadow->x) {
      sprite->x=shadow->x;
    }
  }
  sclock+=elapsed;
  if (sclock<SKYLETON_FALL_LEAD_TIME) return;
  // Increase Y until we reach the shadow vertically.
  // NB: Generic physics are not in play during this stage.
  sprite->y+=SKYLETON_FALL_Y_SPEED*elapsed;
  if (sprite->y>=shadow->y) {
    sprite->y=shadow->y;
    skyleton_begin_BOUNCE(sprite);
  }
}

/* Update BOUNCE.
 */
 
static void skyleton_update_BOUNCE(struct fmn_sprite *sprite,float elapsed) {
  sclock+=elapsed;
  if (sclock>=SKYLETON_BOUNCE_TIME) {
    skyleton_begin_CHARGE(sprite);
  }
}

/* Update CHARGE.
 */
 
static void skyleton_update_CHARGE(struct fmn_sprite *sprite,float elapsed) {
  sclock+=elapsed;
  if (sclock>=SKYLETON_CHARGE_FRAME_TIME) {
    sclock-=SKYLETON_CHARGE_FRAME_TIME;
    sprite->tileid++;
    if (sprite->tileid>=skyleton_state.template.tileid+6) {
      sprite->tileid=skyleton_state.template.tileid+2;
    }
  }
  if (sprite->xform&FMN_XFORM_XREV) {
    if (sprite->x<-1.0f) {
      skyleton_kill(sprite);
      return;
    }
    if (sprite->velx>-SKYLETON_CHARGE_SPEED) sprite->velx-=SKYLETON_CHARGE_ACCEL*elapsed;
  } else {
    if (sprite->x>FMN_COLC+1.0f) {
      skyleton_kill(sprite);
      return;
    }
    if (sprite->velx<SKYLETON_CHARGE_SPEED) sprite->velx+=SKYLETON_CHARGE_ACCEL*elapsed;
  }
  if (sprite->pv[0]) {
    struct fmn_sprite *sword=sprite->pv[0];
    if (sprite->xform&FMN_XFORM_XREV) sword->x=sprite->x-0.875f;
    else sword->x=sprite->x+0.875f;
    sword->y=sprite->y;
  }
}

/* Update BREAK.
 */
 
static void skyleton_update_BREAK(struct fmn_sprite *sprite,float elapsed) {
  sclock+=elapsed;
  int8_t frame=sclock/SKYLETON_BREAK_FRAME_TIME;
  if (frame>=5) {
    fmn_sprite_kill(sprite);
    return;
  }
  if (frame>=0) sprite->tileid=skyleton_state.template.tileid+7+frame;
}

/* Update, instance level.
 */
 
static void _skyleton_update(struct fmn_sprite *sprite,float elapsed) {
  switch (stage) {
    case SKYLETON_STAGE_FALL: skyleton_update_FALL(sprite,elapsed); break;
    case SKYLETON_STAGE_BOUNCE: skyleton_update_BOUNCE(sprite,elapsed); break;
    case SKYLETON_STAGE_CHARGE: skyleton_update_CHARGE(sprite,elapsed); break;
    case SKYLETON_STAGE_BREAK: skyleton_update_BREAK(sprite,elapsed); break;
  }
}

/* Pressure.
 */
 
static void _skyleton_pressure(struct fmn_sprite *sprite,struct fmn_sprite *dummy,uint8_t dir) {
  if (stage!=SKYLETON_STAGE_CHARGE) return;
  if (sprite->xform&FMN_XFORM_XREV) {
    if (dir!=FMN_DIR_E) return;
  } else {
    if (dir!=FMN_DIR_W) return;
  }
  skyleton_begin_BREAK(sprite);
}

/* Init.
 */
 
static void _skyleton_init(struct fmn_sprite *sprite) {
  if (fmn_game_register_map_singleton(
    _skyleton_init,
    _skyleton_class_update,
    _skyleton_class_cleanup,
    0
  )) {
    // Class setup. Triggers where there were no other skyletons, ie at map load.
    skyleton_state.clock=SKYLETON_SPAWN_PHASE;
    memcpy(&skyleton_state.template,sprite,sizeof(struct fmn_sprite));
    fmn_sprite_kill(sprite);
  } else {
    // Instance setup. Triggers when our class-level updater creates a skyleton.
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_skyleton={
  .init=_skyleton_init,
  .update=_skyleton_update,
  .pressure=_skyleton_pressure,
  .static_pressure=_skyleton_pressure,
};

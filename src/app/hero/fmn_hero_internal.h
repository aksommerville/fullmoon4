#ifndef FMN_HERO_INTERNAL_H
#define FMN_HERO_INTERNAL_H

#include "fmn_hero.h"
#include "app/sprite/fmn_sprite.h"
#include "app/fmn_platform.h"

#define FMN_HERO_WALK_SPEED_MAX    8.000f
#define FMN_HERO_ACCELERATION     40.000f
#define FMN_HERO_INJURY_VELOCITY  16.000f
#define FMN_HERO_INJURY_MIN       10.000f
#define FMN_HERO_INJURY_MAX       18.000f
#define FMN_HERO_INJURY_TIME       0.500f

extern struct fmn_hero {
  struct fmn_sprite *sprite;
  int8_t walkdx,walkdy; // current state of dpad
  float walkspeed;
  float walkspeed_target;
  float walkaccel; // always positive
  int8_t cellx,celly; // quantized position
  
  float walkforce;
  float velx,vely; // to cache across sprite rebuilds
} fmn_hero;

void fmn_hero_motion_event(uint8_t bit,uint8_t value); // dpad changes
void fmn_hero_motion_input(uint8_t state); // all input state changes
void fmn_hero_motion_update(float elapsed);

void fmn_hero_item_event(uint8_t value);
void fmn_hero_item_update(float elapsed);

#endif

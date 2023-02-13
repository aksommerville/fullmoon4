#ifndef FMN_HERO_INTERNAL_H
#define FMN_HERO_INTERNAL_H

#include "fmn_hero.h"
#include "app/sprite/fmn_sprite.h"
#include "app/fmn_platform.h"

#define FMN_HERO_WALK_SPEED_MAX    8.000f
#define FMN_HERO_ACCELERATION     40.000f
#define FMN_HERO_FLY_SPEED_MAX    16.000f
#define FMN_HERO_INJURY_VELOCITY  16.000f
#define FMN_HERO_INJURY_MIN       10.000f
#define FMN_HERO_INJURY_MAX       18.000f
#define FMN_HERO_INJURY_TIME       0.500f
#define FMN_HERO_INJURY_SUPPRESSION_BOUNCEBACK 6.000f
#define FMN_HERO_CHEESE_TIME       4.000f
#define FMN_HERO_CHEESE_ADJUST     2.000f
#define FMN_HERO_MATCH_ILLUMINATION_TIME   5.0f
#define FMN_HERO_SHOVEL_TIME               0.500f
#define FMN_HERO_SPELL_LIMIT 12

extern struct fmn_hero {
  struct fmn_sprite *sprite;
  int8_t walkdx,walkdy; // current state of dpad
  float walkspeed;
  float walkspeed_target;
  float walkaccel; // always positive
  int8_t cellx,celly; // quantized position
  
  float walkforce;
  float velx,vely; // to cache across sprite rebuilds
  float cheesetime;
  
  float item_active_time;
  uint8_t bell_count;
  uint8_t spellv[FMN_HERO_SPELL_LIMIT];
  uint8_t spellc; // may exceed limit
  uint8_t landing_pending; // broom flight ended over a hole, will end when clear
} fmn_hero;

void fmn_hero_motion_event(uint8_t bit,uint8_t value); // dpad changes
void fmn_hero_motion_input(uint8_t state); // all input state changes
void fmn_hero_motion_update(float elapsed);

uint8_t fmn_hero_item_motion(uint8_t bit,uint8_t value); // => nonzero if consumed
void fmn_hero_item_event(uint8_t value);
void fmn_hero_item_update(float elapsed);

void fmn_hero_item_end();

uint8_t fmn_hero_facedir_agrees();
void fmn_hero_reset_facedir();

#endif

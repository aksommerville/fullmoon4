/* fmn_sprite.h
 * Registry of live sprites, and features not visible to the platform.
 */
 
#ifndef FMN_SPRITE_H
#define FMN_SPRITE_H

#include "app/fmn_platform.h"

#define FMN_SPRITE_ARGV_SIZE 4
#define FMN_SPRITE_BV_SIZE 8
#define FMN_SPRITE_SV_SIZE 4
#define FMN_SPRITE_FV_SIZE 4

#define FMN_SPRITE_PHYSICS_NONE 0
#define FMN_SPRITE_PHYSICS_FULL 1

struct fmn_sprite {
  FMN_SPRITE_HEADER
  
  void (*update)(struct fmn_sprite *sprite,float elapsed);
  
  // Reference data recorded at spawn point.
  uint16_t spriteid;
  uint8_t argv[FMN_SPRITE_ARGV_SIZE];
  
  // Controller's state.
  uint8_t bv[FMN_SPRITE_BV_SIZE];
  int16_t sv[FMN_SPRITE_SV_SIZE];
  float fv[FMN_SPRITE_FV_SIZE];
  
  // Generic physics.
  uint8_t physics_mode;
  float velx,vely;
  float veldecay; // Linear velocity decay in m/s**2.
  float radius;
};

// Drop all sprites cold.
void fmn_sprites_clear();

/* Make a new sprite if possible.
 * Note that sprite coordinates and map cell coordinates are the same thing, just sprites are floating-point.
 */
struct fmn_sprite *fmn_sprite_spawn(
  float x,float y,
  uint16_t spriteid,
  const uint8_t *argv,uint8_t argc
);

int fmn_sprites_for_each(int (*cb)(struct fmn_sprite *sprite,void *userdata),void *userdata);

void fmn_sprites_update(float elapsed);

void fmn_sprite_apply_force(struct fmn_sprite *sprite,float dx,float dy);

#endif

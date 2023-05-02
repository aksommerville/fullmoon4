#ifndef FMN_RENDER_INTERNAL_H
#define FMN_RENDER_INTERNAL_H

#include "app/fmn_platform.h"
#include "app/sprite/fmn_sprite.h"

#define FMN_RENDER_TRANSITION_FRAMEC 36
 
#define FMN_RENDER_COMPASS_RATE_MIN 0.010f
#define FMN_RENDER_COMPASS_RATE_MAX 0.200f
#define FMN_RENDER_COMPASS_DISTANCE_MAX 40.0f

#define FMN_RENDER_ILLUMINATION_PERIOD 70 /* frames per cycle */
#define FMN_RENDER_ILLUMINATION_RANGE 0.250f /* full range width in alpha units */
#define FMN_RENDER_ILLUMINATION_FADE_TIME 1.0f /* seconds */
#define FMN_RENDER_ILLUMINATION_PEAK 0.750f /* RANGE..1 */

// Wind and rain share a particle buffer.
#define FMN_RENDER_PARTICLE_LIMIT 250

// Image resources and intermediate framebuffers share a namespace.
// Anything we use that's not associated with an image resource, give it an ID above this.
#define FMN_IMAGEID_PRIVATE_BASE 300
#define FMN_IMAGEID_MAPBITS (FMN_IMAGEID_PRIVATE_BASE+1)
#define FMN_IMAGEID_TRANSITION_TO (FMN_IMAGEID_PRIVATE_BASE+2)
#define FMN_IMAGEID_TRANSITION_FROM (FMN_IMAGEID_PRIVATE_BASE+3)
#define FMN_IMAGEID_VICTORY_BITS (FMN_IMAGEID_PRIVATE_BASE+4)
#define FMN_IMAGEID_SCRATCH (FMN_IMAGEID_PRIVATE_BASE+5)

extern struct fmn_render_global {
  int16_t fbw,fbh;
  int16_t tilesize;
  int framec;
  
  int map_dirty;
  
  int transition; // FMN_TRANSITION_*
  int transitionp,transitionc; // frames; (p) counts up to (c)
  int hero_above_transition;
  uint32_t transition_color;
  int16_t transition_from_x,transition_from_y; // in fb pixels, for spotlight
  int16_t transition_to_x,transition_to_y; // ''
  
  float compassangle;
  
  // Weather.
  struct fmn_draw_line particlev[FMN_RENDER_PARTICLE_LIMIT];
  int particlec;
  float illuminationp;
  
} fmn_render_global;

// Major steps from the main fmn_render():
void fmn_render_freshen_map();
void fmn_render_world(uint8_t include_hero);
void fmn_render_violin();
void fmn_render_menus();

void fmn_render_hero_underlay(int16_t addx,int16_t addy);
void fmn_render_hero_overlay(int16_t addx,int16_t addy);
void fmn_render_hero(struct fmn_sprite *sprite,int16_t addx,int16_t addy);
void fmn_render_sprites(uint8_t include_hero);
void fmn_render_fade_out();
void fmn_render_darkness();
void fmn_render_weather();
void fmn_render_idle_warning(int s);

// Transitions:
void fmn_transition_apply(
  int transition,
  int p,int c,
  int imageid_from,int imageid_to
);
void fmn_transition_get_hero_offset(int16_t *addx,int16_t *addy);

#endif
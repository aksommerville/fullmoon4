/* fmn_hero.h
 * The hero sprite, actions, etc.
 */
 
#ifndef FMN_HERO_H
#define FMN_HERO_H

#include <stdint.h>

int fmn_hero_reset();
void fmn_hero_input(uint8_t bit,uint8_t value,uint8_t state);
void fmn_hero_update(float elapsed);

/* Current position in map cells, can be OOB.
 * Returns nonzero if it changed since the last call.
 * (only fmn_game_update should call this, and exactly once per update).
 */
uint8_t fmn_hero_get_quantized_position(int8_t *x,int8_t *y);

// Direct access to sprite's position, eg for map transitions.
void fmn_hero_get_position(float *x,float *y);
void fmn_hero_set_position(float x,float y);
void fmn_hero_kill_velocity();

#endif

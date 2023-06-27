/* fmn_hero.h
 * The hero sprite, actions, etc.
 */
 
#ifndef FMN_HERO_H
#define FMN_HERO_H

#include <stdint.h>

struct fmn_sprite;

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

/* Begin an injury, focussed at (x,y), typically (assailant)'s location.
 * (assailant) is optional.
 * Nonzero if an injury happened.
 * Can be zero if deflected by umbrella, ignored due to previous injury, or whatever.
 */
uint8_t fmn_hero_injure(float x,float y,struct fmn_sprite *assailant);

uint8_t fmn_hero_curse(struct fmn_sprite *assailant);

uint8_t fmn_hero_feet_on_ground();

void fmn_hero_cancel_item();

/* Killing is not something that happens routinely here.
 * Only the werewolf should call this.
 */
void fmn_hero_kill(struct fmn_sprite *assailant);

int fmn_hero_get_spell_in_progress(uint8_t *dst,int dsta);

#endif

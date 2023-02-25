/* fmn_game.h
 * High level ops.
 */
 
#ifndef FMN_GAME_H
#define FMN_GAME_H

#include "fmn_platform.h"

int fmn_game_init();
int fmn_game_load_map(int mapid);
void fmn_game_input(uint8_t bit,uint8_t value,uint8_t state);
void fmn_game_update(float elapsed);

/* Rebuild the secrets in response to a newly loaded map.
 * fmn_global.(compassx,compassy)
 */
void fmn_secrets_refresh_for_map();

uint8_t fmn_spell_eval(const uint8_t *v,uint8_t c);
void fmn_spell_cast(uint8_t spellid);

uint8_t fmn_gs_get_bit(uint16_t p);
void fmn_gs_set_bit(uint16_t p,uint8_t v);

/* Game will call whenever this gs bit changes.
 * All listeners are blindly dropped at each map transition.
 */
uint16_t fmn_gs_listen_bit(uint16_t p,void (*cb)(void *userdata,uint16_t p,uint8_t v),void *userdata);
void fmn_gs_unlisten(uint16_t id);
void fmn_gs_drop_listeners();

#endif

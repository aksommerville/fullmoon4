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

#endif

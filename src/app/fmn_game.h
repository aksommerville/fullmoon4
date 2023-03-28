/* fmn_game.h
 * High level ops.
 */
 
#ifndef FMN_GAME_H
#define FMN_GAME_H

#include "fmn_platform.h"

#define FMN_WIND_TIME 4.0f
#define FMN_WIND_RATE 2.0f
#define FMN_RAIN_TIME 4.0f
#define FMN_SLOWMO_TIME 4.0f
#define FMN_SLOWMO_RATE 0.25f
#define FMN_INVISIBILITY_TIME 5.0f

// Zero for non-quantity items, or the quantity to collect by default.
extern const uint8_t fmn_item_default_quantities[FMN_ITEM_COUNT];

int fmn_game_init();
int fmn_game_load_map(int mapid);
void fmn_game_input(uint8_t bit,uint8_t value,uint8_t state);
void fmn_game_update(float elapsed);

/* Rebuild the secrets in response to a newly loaded map.
 * fmn_global.(compassx,compassy)
 */
void fmn_secrets_refresh_for_map();

// Points the direction of the secret, or we make something up. Always a valid cardinal direction.
uint8_t fmn_secrets_get_guide_dir();

/* Songs and spells use the same namespace.
 * Both 'eval' return zero if invalid.
 * Caller must trim leading and trailing space.
 * Songs will match by suffix; spells must be a perfect match.
 */
uint8_t fmn_spell_eval(const uint8_t *v,uint8_t c);
uint8_t fmn_song_eval(const uint8_t *v,uint8_t c);
void fmn_spell_cast(uint8_t spellid);

/* (quantity) zero for default, and we ignore it if irrelevant.
 * Returns nonzero if player's inventory actually changed.
 * If not, we do perform whatever rejection is warranted.
 */
uint8_t fmn_collect_item(uint8_t itemid,uint8_t quantity);

uint8_t fmn_gs_get_bit(uint16_t p);
void fmn_gs_set_bit(uint16_t p,uint8_t v);

/* Game will call whenever this gs bit changes.
 * All listeners are blindly dropped at each map transition.
 * Sprites don't normally unlisten themselves. (but if you can be destroyed, you must)
 */
uint16_t fmn_gs_listen_bit(uint16_t p,void (*cb)(void *userdata,uint16_t p,uint8_t v),void *userdata);
void fmn_gs_unlisten(uint16_t id);
void fmn_gs_drop_listeners();

/* Subscribable event bus.
 * All subscriptions get cleared on map loads.
 * Do not unlisten during a callback!
 */
uint16_t fmn_game_event_listen(uint16_t eventid,void (*cb)(void *userdata,uint16_t eventid,void *payload),void *userdata);
void fmn_game_event_unlisten(uint16_t id);
void fmn_game_event_broadcast(uint16_t eventid,void *payload);
void fmn_game_event_unlisten_all();
#define FMN_GAME_EVENT_MISSILE_OOB 1 /* Missile sprite terminating due to off screen. payload=sprite */
#define FMN_GAME_EVENT_SCOREBOARD_WIN 2 /* payload=sprite. argv[0]=gameid */
#define FMN_GAME_EVENT_SCOREBOARD_LOSE 3

/* Extra mechanism for arbitrary update logic.
 * I'm adding this for conveyor belts' sake, but one imagines there will be lots of uses.
 * Any map_singleton registration with a (discriminator) already in use is presumed redundant and will be ignored.
 * Use the address of the function where you call this, or something similarly unique.
 * Returns zero if this discriminator is already registered.
 * (cb_update) will be called once per global update, just like sprites. But after all sprite updates.
 * (cb_cleanup) will be called once, as the map unloads.
 * (cb_cleanup) is NOT called if you unregister manually.
 */
uint16_t fmn_game_register_map_singleton(
  void *discriminator,
  void (*cb_update)(void *userdata,float elapsed),
  void (*cb_cleanup)(void *userdata),
  void *userdata
);
void fmn_game_unregister_map_singleton(uint16_t id);
void fmn_game_drop_map_singletons();
void fmn_game_update_map_singletons(float elapsed);

/* If you want raw sketches, read them straight off fmn_global.sketchv.
 * "word" gives you each horizontally-contiguous range of sketches, interpretted as roman letters.
 * (src) will contain:
 *   A..Z
 *   0..9
 *   ?    For anything else.
 */
int8_t fmn_for_each_sketch_word(int8_t (*cb)(const char *src,uint8_t srcc,void *userdata),void *userdata);

/* Map callbacks.
 * Arbitrary bits of high-level game logic that can be triggered by map events.
 */
void fmn_game_map_callback(uint16_t cbid,uint8_t param,void *userdata);

#define FMN_MAP_CALLBACK_kill_top_pushblock_if_pumpkin_at_nw       1

#define FMN_FOR_EACH_MAP_CALLBACK \
  _(kill_top_pushblock_if_pumpkin_at_nw)
  
#define _(tag) void fmn_map_callback_##tag(uint8_t param,void *userdata);
FMN_FOR_EACH_MAP_CALLBACK
#undef _

#endif

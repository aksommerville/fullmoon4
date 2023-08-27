/* fmn_game.h
 * High level ops.
 */
 
#ifndef FMN_GAME_H
#define FMN_GAME_H

#include "fmn_platform.h"

#define FMN_WIND_TIME 4.0f
#define FMN_WIND_RATE 2.0f
#define FMN_RAIN_TIME 4.0f
#define FMN_SLOWMO_TIME 7.0f
#define FMN_SLOWMO_RATE 0.25f
#define FMN_INVISIBILITY_TIME 5.0f

// Zero for non-quantity items, or the quantity to collect by default.
extern const uint8_t fmn_item_default_quantities[FMN_ITEM_COUNT];

int fmn_game_init();
int fmn_game_has_saved_game();
int fmn_game_load_saved_game();
int fmn_game_load_map(int mapid,float herox,float heroy); // (herox<0) for default, in teleport and restart cases.
void fmn_game_input(uint8_t bit,uint8_t value,uint8_t state);
void fmn_game_update(float elapsed);

// Happens automatically on normal movement. Exposed so fmn_hero_item can call it when broom travel ends.
uint8_t fmn_game_check_static_hazards(uint8_t x,uint8_t y);

/* Rebuild the secrets in response to a newly loaded map.
 * fmn_global.(compassx,compassy)
 */
void fmn_secrets_refresh_for_map();

/* Points the direction of the next recommended step, or we make something up.
 * Zero if we got nothing. We must allow for the possibility.
 * 0xff if the next step is right here.
 */ 
uint8_t fmn_secrets_get_guide_dir();

/* When the crow eats corn, he shows you the next direction to go.
 * He can also call this, to auto-generate a crow on the next (c) maps.
 */
void fmn_add_free_birds(uint8_t c);
void fmn_clear_free_birds();
void fmn_update_free_birds();

void fmn_game_begin_earthquake(uint8_t dir);

/* Songs and spells use the same namespace.
 * Both 'eval' return zero if invalid.
 * Caller must trim leading and trailing space.
 * Songs will match by suffix; spells must be a perfect match.
 */
uint8_t fmn_spell_eval(const uint8_t *v,uint8_t c);
uint8_t fmn_song_eval(const uint8_t *v,uint8_t c);
uint8_t fmn_spell_get(uint8_t *dst,uint8_t dsta,uint8_t spellid);
void fmn_spell_cast(uint8_t spellid);

/* (quantity) zero for default, and we ignore it if irrelevant.
 * Returns nonzero if player's inventory actually changed.
 * If not, we do perform whatever rejection is warranted.
 */
uint8_t fmn_collect_item(uint8_t itemid,uint8_t quantity);

uint8_t fmn_gs_get_bit(uint16_t p);
void fmn_gs_set_bit(uint16_t p,uint8_t v);
uint32_t fmn_gs_get_word(uint16_t p,uint16_t c);
void fmn_gs_set_word(uint16_t p,uint16_t c,uint32_t v);

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
#define FMN_GAME_EVENT_BALCONY 4
#define FMN_GAME_EVENT_TREADMILL 5 /* payload=int *delta */
#define FMN_GAME_EVENT_BLOCKS_MOVED 6
#define FMN_GAME_EVENT_SONG_OK 7 /* payload=uint8_t *spellid; sent as soon as the song is played; it's not cast yet */
#define FMN_GAME_EVENT_TREADLE_CHANGE 8

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

#define FMN_MAP_CALLBACK_kill_top_pushblock_if_pumpkin_at_nw 1
#define FMN_MAP_CALLBACK_set_gsbit_00xx 2
#define FMN_MAP_CALLBACK_swamp_maze 3

#define FMN_FOR_EACH_MAP_CALLBACK \
  _(kill_top_pushblock_if_pumpkin_at_nw) \
  _(set_gsbit_00xx) \
  _(swamp_maze)
  
#define _(tag) void fmn_map_callback_##tag(uint8_t param,void *userdata);
FMN_FOR_EACH_MAP_CALLBACK
#undef _

uint8_t fmn_render_transition_in_progress();

/* "play time" is our own concept of how much time have we spent interactive.
 * "platform time" is exactly what the platform has reported to us.
 * Normally (platform+menuTime+transitionTime==playTime).
 * Always use platform time for plants, since the platform blooms them.
 * Reporting to the user, I recommend play time.
 */
uint32_t fmn_game_get_play_time_ms();
void fmn_game_advance_play_time_ms(uint32_t addms);
void fmn_game_reset_play_time();
uint32_t fmn_game_get_platform_time_ms();
int fmn_get_idle_warning_time_s();

/* Menus.
 *************************************************************/
 
#define FMN_MENU_ARGV_SIZE 8
#define FMN_MENU_FV_SIZE 4
 
struct fmn_menu {
  int menuid;
  int argv[FMN_MENU_ARGV_SIZE];
  float fv[FMN_MENU_FV_SIZE];
  uint8_t pvinput;
  uint8_t opaque; // Hint to renderer that it doesn't need to draw any layers behind this.
  int16_t fbw,fbh; // ctor captures this before init, on the assumption that it will be needed often.
  void (*update)(struct fmn_menu *menu,float elapsed,uint8_t input);
  void (*render)(struct fmn_menu *menu);
  void (*language_changed)(struct fmn_menu *menu);
  
  /* Whoever begins the menu may set a callback.
   * Menu types can make up (message) values 0x80 and above. Low values are defined here.
   * If a callback is set, menus should not dismiss themselves, the callback client should.
   */
  void *userdata;
  void (*cb)(struct fmn_menu *menu,uint8_t message);
};

/* Begin a modal menu interaction.
 * Returns a WEAK reference on success; you can ignore it.
 */
struct fmn_menu *fmn_begin_menu(int menuid,int arg0);
#define FMN_MENU_PAUSE 1
#define FMN_MENU_CHALK 2 /* arg0=bits */
#define FMN_MENU_TREASURE 3 /* arg0=itemid */
#define FMN_MENU_VICTORY 4 /* Demo only. Simple splash of "Victory" and stats. */
#define FMN_MENU_GAMEOVER 5
#define FMN_MENU_HELLO 6
#define FMN_MENU_SETTINGS 7
#define FMN_MENU_CREDITS 8 /* Full only. Extended narrative sequence. */
#define FMN_MENU_INPUT 9
#define FMN_MENU_SOUNDCHECK 10 /* Secret, dev only (enable at fmn_menu_hello.c:_hello_update) */
#define FMN_MENU_ARCADE 11

#define FMN_MENU_MESSAGE_CANCEL 1
#define FMN_MENU_MESSAGE_SUBMIT 2
#define FMN_MENU_MESSAGE_CHANGED 0x80

struct fmn_menu *fmn_get_top_menu();
int fmn_for_each_menu(int (*cb)(struct fmn_menu *menu,void *userdata),void *userdata);

/* Menus dismissed this way do not trigger any callbacks.
 */
void fmn_dismiss_top_menu();
void fmn_dismiss_menu(struct fmn_menu *menu);

uint32_t fmn_chalk_points_from_bit(uint32_t bit);
uint32_t fmn_chalk_bit_from_points(uint32_t points);

#endif

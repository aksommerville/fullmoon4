/* fmn_platform.h
 * Declares things provided by the platform.
 * That means the implementation could be in Javascript, and gets linked at load time.
 * In the future I might write other platform implementations in C for specific targets, the app shouldn't have to care.
 */
 
#ifndef FMN_PLATFORM_H
#define FMN_PLATFORM_H

/* !!! Don't include any libc headers yourself !!!
 * I want the app to be absolutely portable, so either we provide it here or it's not available.
 * Platforms that include libc on their own can simply decline to implement these and let nature take its course.
 */
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>
#include <stddef.h>
#include <math.h> /* I'll add implementations as they become needed. See src/platform/fmn_platform_libm.c */
#if FMN_USE_macos
  // Seems that my MacOS libc has memcpy as a macro. No worries, under macos we're definitely using a real libc, use the real headers.
  #include <stdlib.h>
  #include <string.h>
#else
  void *memcpy(void *dst,const void *src,size_t c);
  void *memmove(void *dst,const void *src,size_t c);
  void *memset(void *dst,int src,size_t c);
  int memcmp(const void *a,const void *b,size_t c);
  int rand();
#endif

#define FMN_COLC 20
#define FMN_ROWC 12

#define FMN_PLANT_LIMIT 16
#define FMN_SKETCH_LIMIT 16
#define FMN_DOOR_LIMIT 16
#define FMN_SPRITE_STORAGE_SIZE 64

#define FMN_XFORM_XREV 1
#define FMN_XFORM_YREV 2
#define FMN_XFORM_SWAP 4

#define FMN_INPUT_LEFT     0x01
#define FMN_INPUT_RIGHT    0x02
#define FMN_INPUT_UP       0x04
#define FMN_INPUT_DOWN     0x08
#define FMN_INPUT_USE      0x10
#define FMN_INPUT_MENU     0x20

#define FMN_DIR_NW   0x80
#define FMN_DIR_N    0x40
#define FMN_DIR_NE   0x20
#define FMN_DIR_W    0x10
#define FMN_DIR_MID  0x00
#define FMN_DIR_E    0x08
#define FMN_DIR_SW   0x04
#define FMN_DIR_S    0x02
#define FMN_DIR_SE   0x01

#define FMN_TRANSITION_CUT        0
#define FMN_TRANSITION_PAN_LEFT   1
#define FMN_TRANSITION_PAN_RIGHT  2
#define FMN_TRANSITION_PAN_UP     3
#define FMN_TRANSITION_PAN_DOWN   4
#define FMN_TRANSITION_FADE_BLACK 5
#define FMN_TRANSITION_DOOR       6
#define FMN_TRANSITION_WARP       7

#define FMN_PLANT_STATE_NONE     0 /* Slot not in use. */
#define FMN_PLANT_STATE_SEED     1 /* Planted but not watered. */
#define FMN_PLANT_STATE_GROW     2 /* Watered, growing. */
#define FMN_PLANT_STATE_FLOWER   3 /* Bearing fruit. */
#define FMN_PLANT_STATE_DEAD     4 /* Withered, candidate for replacement. */

#define FMN_FLOWER_TIME_MS 20000

// PLANT_FRUIT corresponds to PITCHER_CONTENT, what you get by watering a plant with the given liquid.
#define FMN_PLANT_FRUIT_NONE    0
#define FMN_PLANT_FRUIT_SEED    1
#define FMN_PLANT_FRUIT_CHEESE  2
#define FMN_PLANT_FRUIT_COIN    3
#define FMN_PLANT_FRUIT_MATCH   4

#define FMN_PITCHER_CONTENT_EMPTY 0
#define FMN_PITCHER_CONTENT_WATER 1
#define FMN_PITCHER_CONTENT_MILK  2
#define FMN_PITCHER_CONTENT_HONEY 3
#define FMN_PITCHER_CONTENT_SAP   4

#define FMN_ITEM_SNOWGLOBE       0
#define FMN_ITEM_HAT             1
#define FMN_ITEM_PITCHER         2
#define FMN_ITEM_SEED            3
#define FMN_ITEM_COIN            4
#define FMN_ITEM_MATCH           5
#define FMN_ITEM_BROOM           6
#define FMN_ITEM_WAND            7
#define FMN_ITEM_UMBRELLA        8
#define FMN_ITEM_FEATHER         9
#define FMN_ITEM_SHOVEL         10
#define FMN_ITEM_COMPASS        11
#define FMN_ITEM_VIOLIN         12
#define FMN_ITEM_CHALK          13
#define FMN_ITEM_BELL           14
#define FMN_ITEM_CHEESE         15
#define FMN_ITEM_COUNT          16

// Sound effects.
#define FMN_SFX_BELL 1
#define FMN_SFX_REJECT_ITEM 2
#define FMN_SFX_CHEESE 3
#define FMN_SFX_HURT 4
#define FMN_SFX_PITCHER_NO_PICKUP 5
#define FMN_SFX_PITCHER_PICKUP 6
#define FMN_SFX_PITCHER_POUR 7
#define FMN_SFX_MATCH 8
#define FMN_SFX_DIG 9
#define FMN_SFX_REJECT_DIG 10
#define FMN_SFX_INJURY_DEFLECTED 11
#define FMN_SFX_GRIEVOUS_INJURY 12
#define FMN_SFX_UNCHEESE 13
#define FMN_SFX_ITEM_MAJOR 14
#define FMN_SFX_ITEM_MINOR 15
#define FMN_SFX_TREADLE_PRESS 16
#define FMN_SFX_TREADLE_RELEASE 17
#define FMN_SFX_PUSHBLOCK_ENCHANT 18
#define FMN_SFX_BLOCK_EXPLODE 19
#define FMN_SFX_MOO 20
#define FMN_SFX_PLANT 21
#define FMN_SFX_SEED_DROP 22
#define FMN_SFX_SPROUT 23
#define FMN_SFX_BLOOM 24
#define FMN_SFX_TELEPORT 25
#define FMN_SFX_SLOWMO_BEGIN 26
#define FMN_SFX_SLOWMO_END 27
#define FMN_SFX_RAIN 28
#define FMN_SFX_WIND 29
#define FMN_SFX_INVISIBILITY_BEGIN 30
#define FMN_SFX_INVISIBILITY_END 31
#define FMN_SFX_ENCHANT_ANIMAL 32
#define FMN_SFX_PUMPKIN 33
#define FMN_SFX_UNPUMPKIN 34
#define FMN_SFX_KICK_1 35 /* begin GM... */
#define FMN_SFX_KICK_2 36
#define FMN_SFX_SIDE_STICK 37
#define FMN_SFX_SNARE_1 38
#define FMN_SFX_HAND_CLAP 39
#define FMN_SFX_SNARE_2 40
#define FMN_SFX_TOM_1 41
#define FMN_SFX_CLOSED_HAT 42
#define FMN_SFX_TOM_2 43
#define FMN_SFX_PEDAL_HAT 44
#define FMN_SFX_TOM_3 45
#define FMN_SFX_OPEN_HAT 46
#define FMN_SFX_TOM_4 47
#define FMN_SFX_TOM_5 48
#define FMN_SFX_CRASH_1 49
#define FMN_SFX_TOM_6 50
#define FMN_SFX_RIDE_1 51
#define FMN_SFX_CRASH_2 52
#define FMN_SFX_RIDE_2 53
#define FMN_SFX_TAMBOURINE 54
#define FMN_SFX_SPLASH_1 55
#define FMN_SFX_COWBELL 56 /* ...end GM */
#define FMN_SFX_PAYMENT 57
#define FMN_SFX_FIZZLE 58
#define FMN_SFX_THUD 59
#define FMN_SFX_BREAK_BONES 60
#define FMN_SFX_CURSE 61
#define FMN_SFX_UNCURSE 62
#define FMN_SFX_QUACK 63
#define FMN_SFX_KILL_WEREWOLF 64
#define FMN_SFX_UNBURY_TREASURE 65
#define FMN_SFX_UNBURY_DOOR 66
#define FMN_SFX_EARTHQUAKE 67
#define FMN_SFX_PUSH 68
#define FMN_SFX_FIRENOZZLE_ON 69
#define FMN_SFX_TOSS 70
#define FMN_SFX_SPIT 71
#define FMN_SFX_GROWL 72
#define FMN_SFX_BARK 73
#define FMN_SFX_HADOUKEN 74
#define FMN_SFX_PANDA_CRY 75

#define FMN_SPRITE_STYLE_HIDDEN      1 /* don't render */
#define FMN_SPRITE_STYLE_TILE        2 /* single tile */
#define FMN_SPRITE_STYLE_HERO        3 /* special */
#define FMN_SPRITE_STYLE_FOURFRAME   4 /* 4 tiles arranged horizontally, automatic animation */
#define FMN_SPRITE_STYLE_FIRENOZZLE  5 /* special */
#define FMN_SPRITE_STYLE_FIREWALL    6 /* special */
#define FMN_SPRITE_STYLE_DOUBLEWIDE  7 /* renders a second tile (id+1) to the right of the named one, and cheats left to stay centered */
#define FMN_SPRITE_STYLE_PITCHFORK   8 /* special */
#define FMN_SPRITE_STYLE_TWOFRAME    9 /* 2 tiles arranged horizontally, automatic animation */
#define FMN_SPRITE_STYLE_EIGHTFRAME 10 /* 8 tiles arranged horizontally, automatic animation */
#define FMN_SPRITE_STYLE_SCARYDOOR  11 /* special */
#define FMN_SPRITE_STYLE_WEREWOLF   12 /* special */
#define FMN_SPRITE_STYLE_FLOORFIRE  13 /* special */
#define FMN_SPRITE_STYLE_DEADWITCH  14 /* special */
#define FMN_SPRITE_STYLE_PANDA      15 /* special */

#define FMN_SPRITE_BV_SIZE 8
#define FMN_SPRITE_SV_SIZE 4
#define FMN_SPRITE_FV_SIZE 8
#define FMN_SPRITE_PV_SIZE 1

#define FMN_GS_SIZE 64 /* General global persistent state size in bytes. */

#define FMN_CELLPHYSICS_VACANT 0
#define FMN_CELLPHYSICS_SOLID 1
#define FMN_CELLPHYSICS_HOLE 2
#define FMN_CELLPHYSICS_UNSHOVELLABLE 3
#define FMN_CELLPHYSICS_UNCHALKABLE 4
#define FMN_CELLPHYSICS_SAP 5 /* SOLID and also supplies sap (chalkable) */
#define FMN_CELLPHYSICS_SAP_NOCHALK 6
#define FMN_CELLPHYSICS_WATER 7 /* HOLE and also supplies water */
#define FMN_CELLPHYSICS_REVELABLE 8 /* SOLID */

/* Size of the violin input buffer.
 * The longest song must be shorter than this. (not equal)
 */
#define FMN_VIOLIN_SONG_LENGTH 20

#define FMN_SPELLID_BLOOM 1
#define FMN_SPELLID_RAIN 2
#define FMN_SPELLID_WIND_W 3
#define FMN_SPELLID_WIND_E 4
#define FMN_SPELLID_WIND_N 5
#define FMN_SPELLID_WIND_S 6
#define FMN_SPELLID_SLOWMO 7
#define FMN_SPELLID_HOME 8
#define FMN_SPELLID_TELE1 9
#define FMN_SPELLID_TELE2 10
#define FMN_SPELLID_TELE3 11
#define FMN_SPELLID_TELE4 12
#define FMN_SPELLID_OPEN 13
#define FMN_SPELLID_INVISIBLE 14
#define FMN_SPELLID_REVEILLE 15
#define FMN_SPELLID_LULLABYE 16
#define FMN_SPELLID_HITHER 17
#define FMN_SPELLID_THITHER 18
#define FMN_SPELLID_REVELATIONS 19
#define FMN_SPELLID_PUMPKIN 21

/* fmn_sprite_header is the part visible to the platform.
 * The app declares a more useful struct fmn_sprite with more content.
 */
#define FMN_SPRITE_HEADER \
  float x,y; /* midpoint in grid space */ \
  uint8_t style; \
  uint8_t imageid; \
  uint8_t tileid; \
  uint8_t xform; \
  uint16_t controller; \
  uint16_t pad1; \
  uint8_t bv[FMN_SPRITE_BV_SIZE]; \
  int16_t sv[FMN_SPRITE_SV_SIZE]; \
  float fv[FMN_SPRITE_FV_SIZE]; \
  void *pv[FMN_SPRITE_PV_SIZE];
struct fmn_sprite_header { FMN_SPRITE_HEADER };

struct fmn_plant {
  uint8_t x;
  uint8_t y;
  uint8_t state;
  uint8_t fruit;
  uint32_t flower_time;
};

struct fmn_sketch {
  uint8_t x;
  uint8_t y;
  uint16_t pad;
  uint32_t bits;
  uint32_t time; /* Timestamp of last touch, so we can overwrite in chronological order. */
};

struct fmn_door {
  uint8_t x;
  uint8_t y;
  uint16_t mapid; // zero if the door is used for special purposes (ie not an actual door)
  uint8_t dstx;
  uint8_t dsty;
  uint16_t extra;
  /* Use as a door, pretty obvious.
   * Can also be abused for other purposes:
   *  - transmogrify: mapid==0, dstx=0x80(to),0x40(from),0xc0(toggle), dsty=state(nonzero)
   *  - buried_treasure: mapid==0, dstx=0x30, dsty=itemid, extra=gsbit
   *  - buried_door: extra=gsbit(nonzero)
   *  - event_trigger: mapid==0, dstx=0x20, extra=eventid. See fmn_game_event_listen. (in fmn_game, not platform)
   */
};

/* App must define this.
 * The JS side will read it straight out of memory.
 * It is important that you not change the order or type of members, without updating the JS app!
 */
extern struct fmn_global {

  /* Sprites in render order.
   * Game can modify these on the fly; and they are read-only to platform.
   */
  struct fmn_sprite_header **spritev;
  uint32_t spritec;

  /* Map content, loaded by platform.
   * You are allowed to change map tiles but must notify the platform. Changes won't persist.
   */
  uint8_t map[FMN_COLC*FMN_ROWC];
  uint8_t maptsid;
  uint8_t songid;
  uint16_t neighborw;
  uint16_t neighbore;
  uint16_t neighborn;
  uint16_t neighbors;
  uint8_t mapdark;
  uint8_t indoors;
  uint16_t pad2;
  uint8_t pad1;
  uint8_t herostartp;
  uint8_t cellphysics[256];
  uint8_t sprite_storage[FMN_SPRITE_STORAGE_SIZE];
  struct fmn_door doorv[FMN_DOOR_LIMIT];
  uint32_t doorc;
  
  /* Current plants and sketches, loaded by platform.
   * You can modify these and the changes will persist, but you must not add or remove anything.
   * NB Only plants and sketches on the current map are accessible.
   */
  struct fmn_plant plantv[FMN_PLANT_LIMIT];
  uint32_t plantc;
  struct fmn_sketch sketchv[FMN_SKETCH_LIMIT];
  uint32_t sketchc;
  
  /* (selected_item) is the one chosen by user from a menu. Should be read-only to the app.
   * (active_item) is typically zero when idle, or same as (selected_item) while the button is held.
   * They are allowed to go out of sync though. Some items self-terminate and some linger (eg releasing broom over water).
   */
  uint8_t selected_item;
  uint8_t active_item;
  uint8_t show_off_item; // Item ID to display as "just collected". Or (FMN_ITEM_PITCHER|(FMN_PITCHER_CONTENT_*<<4))
  uint8_t show_off_item_time; // Counts down from 0xff, not necessarily contiguous
  uint8_t itemv[16]; // nonzero if possessed
  uint8_t itemqv[16]; // qualifier eg count or enum
  
  /* More hero stuff for renderer.
   */
  uint8_t facedir; // FMN_DIR_*, cardinals only.
  uint8_t walking;
  uint8_t last_horz_dir; // FMN_DIR_W or FMN_DIR_E
  uint8_t wand_dir; // current direction while encoding on wand or violin. also borrowed by other items (pitcher)
  float injury_time;
  float illumination_time; // From non-match sources. Both illumination_times count down concurrently, and renderer uses the larger.
  float match_illumination_time; // From the match only.
  uint8_t cheesing;
  uint8_t spell_repudiation; // app sets to 0xff to begin repudiation (upon ending the wand action). platform handles from there.
  uint8_t transmogrification; // 0=normal, 1=pumpkin
  uint8_t hero_dead;
  float invisibility_time;
  float curse_time;
  
  // Relative position of the secret the compass should point to.
  // (0,0) is special, it means "nothing". (zero is a valid cell, you just can't put a secret there).
  int16_t compassx;
  int16_t compassy;
  
  // Current cell focussed for shovel.
  int8_t shovelx;
  int8_t shovely;
  uint8_t werewolf_dead;
  uint8_t blowback;
  
  // General-purpose global state.
  // The whole thing gets persisted on saves.
  // Don't modify directly. There are helpers to toggle values, which will notify subscribers. (TODO)
  uint8_t gs[FMN_GS_SIZE];
  
  uint8_t violin_song[FMN_VIOLIN_SONG_LENGTH];
  float violin_clock; // 0..1, counts up to the next beat transition.
  uint8_t violin_songp;
  uint8_t pad4;
  uint8_t pad5;
  
  uint8_t wind_dir;
  float wind_time;
  float rain_time;
  float slowmo_time;
  float terminate_time;
  float blowbackx; // Same idea as wind, but app reports a special "can't reach the edge" wind.
  float blowbacky; // (-1,-1)..(0,0)..(1,1)
  
  uint16_t damage_count;
  uint8_t pad6;
  uint8_t earthquake_dir;
  float earthquake_time;
  
  uint16_t facedir_gsbit_ccw;
  uint16_t facedir_gsbit_cw;
  
} fmn_global;

/* App must implement these hooks for platform to call.
 **********************************************/
 
// Called once, during startup. <0 to abort.
int fmn_init();

/* Called often, when the game is running without any modal platform-managed interaction.
 * (ie normal play cases).
 * (timems) is an absolute timestamp counting active time only, since game start.
 * It includes time from prior sessions, and overflows every 1200 hours or so.
 */
void fmn_update(uint32_t timems,uint8_t input);

/* Notification of some change to fmn_global.gs initiated by the platform.
 * (p,c) are in bits, reading each byte big-endianly. (p=0 is [0]&0x80, p=1 is [0]&0x40, ...)
 */
void fmn_gs_notify(uint16_t p,uint16_t c);

/* Called when we're ready for the next scene.
 * fmn_draw_* calls are valid only during this.
 * Normally one render per update, but that's not guaranteed.
 * Return nonzero if you rendered a complete frame that should be delivered to output.
 * Return zero if you did nothing, and we should keep the prior frame.
 * Do not give up partway thru; backend's behavior is undefined in that case.
 */
uint8_t fmn_render();

/* Call if you change map tiles. You don't need to track which ones.
 * Plants too.
 * This was formerly in the platform's API but moved to client -- this kind of thing wouldn't normally live in this header.
 */
void fmn_map_dirty();

/* Prepare a transition while in the "from" state, and declare what style you will want.
 * Then make your changes, and either commit or cancel it.
 * Platform decides whether to update during a transition, I'm thinking no.
 * If you prepare and then finish the frame without commit or cancel, it implicitly cancels.
 * Two prepares in one frame, the second cancels the first.
 */
void fmn_prepare_transition(int transition);
void fmn_commit_transition();
void fmn_cancel_transition();

/* Platform implements the rest.
 *************************************************/

// For debugging. Logs to JS console.
void fmn_log(const char *fmt,...);

// Hard stop. No further app calls will be made, and user will see an unfriendly error.
void fmn_abort();

/* Notify backend that the game is fully restarting.
 * This will trigger fmn_init() again.
 */
void fmn_reset();

/* Replace the global map section from the platform-owned data archive.
 * Also updates (plantv,sketchv) with plants and sketches in the current view.
 * Returns >0 if loaded, 0 if not found, <0 for real errors.
 * We call (cb_spawn) synchronously with all of the new sprites -- hero not included.
 */
int8_t fmn_load_map(
  uint16_t mapid,
  void (*cb_spawn)(
    int8_t x,int8_t y,
    uint16_t spriteid,uint8_t arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3,
    const uint8_t *cmdv,uint16_t cmdc
  )
);

/* Plant a seed or begin editing a sketch.
 * Both return <0 to reject, eg no space available.
 * fmn_begin_sketch() returns the existing bits (zero is normal), or 0xffffffff if we couldn't create one.
 * Caller should invoke the sketch menu after on success.
 */
int8_t fmn_add_plant(uint16_t x,uint16_t y);
uint32_t fmn_begin_sketch(uint16_t x,uint16_t y);

void fmn_sound_effect(uint16_t sfxid);
void fmn_synth_event(uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b);
void fmn_play_song(uint8_t songid);

// Platform is not required to terminate strings, and probably won't.
uint8_t fmn_get_string(char *dst,uint8_t dsta,uint16_t id);

/* Find a command in any map matching (v) according to (mask).
 * If found, the relative position in local meters is written to (xy), and we return nonzero.
 * Up to 8 bytes of a command can be considered, including the opcode.
 * mask 0x01 is the opcode, 0x02 is the first payload byte, etc.
 * (v) must contain dummies for the skipped bytes.
 * eg to find a depumpkinning transmogrifier: mask=0x05, v=[0x44,0,0x41]
 * Platforms are not expected to search very far.
 * They are expected to search at least the current map and cardinal neighbors.
 */
uint8_t fmn_find_map_command(int16_t *xy,uint8_t mask,const uint8_t *v);

/* Like fmn_find_map_command() but designed for crows rather than compasses.
 * We'll search a much broader range, and return FMN_DIR_*, which way should you go from here to get to it.
 * We can only search for items, either as "treasure" sprites or "buried_treasure" map commands.
 * 0 if not found
 * 0xff if it's on the current map.
 */
uint8_t fmn_find_direction_to_item(uint8_t itemid);
uint8_t fmn_find_direction_to_map(uint16_t mapid);

/* Trigger callbacks registered with the current map, for this (evid).
 * This is a generic extension point for attaching your C code with map-specific config.
 */
void fmn_map_callbacks(uint8_t evid,void (*cb)(uint16_t cbid,uint8_t param,void *userdata),void *userdata);
#define FMN_MAP_EVID_LOADED     0x01
#define FMN_MAP_EVID_UNLOAD     0x02

/* Maybe just for demo and beta test?
 * Capture business events to a machine-readable log I can examine later.
 */
void fmn_log_event(const char *key,const char *fmt,...);

/* Rendering API.
 * Everything named "fmn_draw_*" is only valid during fmn_render.
 * I'll use "fmn_video_*" for functions safe to call any time.
 * Render ops all take an array of primitives.
 * Depending on implementation, it can be much more efficient to do large batches than individual ops.
 * "fmn_pixfmt_*" functions are implemented in the client to be more shareable.
 ***********************************************************************/
 
#define FMN_VIDEO_PIXFMT_ANY        0x00
#define FMN_VIDEO_PIXFMT_ANY_1      0x10
#define FMN_VIDEO_PIXFMT_Y1BE       0x11
#define FMN_VIDEO_PIXFMT_W1BE       0x12
#define FMN_VIDEO_PIXFMT_ANY_2      0x20
#define FMN_VIDEO_PIXFMT_ANY_4      0x30
#define FMN_VIDEO_PIXFMT_ANY_8      0x40
#define FMN_VIDEO_PIXFMT_I8         0x41
#define FMN_VIDEO_PIXFMT_Y8         0x42
#define FMN_VIDEO_PIXFMT_ANY_16     0x50
#define FMN_VIDEO_PIXFMT_RGB565LE   0x51
#define FMN_VIDEO_PIXFMT_RGBA4444BE 0x52
#define FMN_VIDEO_PIXFMT_ANY_24     0x60
#define FMN_VIDEO_PIXFMT_ANY_32     0x70
#define FMN_VIDEO_PIXFMT_RGBA       0x71
#define FMN_VIDEO_PIXFMT_BGRA       0x72
#define FMN_VIDEO_PIXFMT_ARGB       0x73
#define FMN_VIDEO_PIXFMT_ABGR       0x74

/* Make the most sensible assumption for wildcard types, and assert known formats.
 * We return zero (FMN_VIDEO_PIXFMT_ANY) if unknown, otherwise a concrete pixfmt.
 * These are implemented in the client.
 */
uint8_t fmn_pixfmt_concrete(uint8_t pixfmt);
uint8_t fmn_pixfmt_get_pixel_size(uint8_t pixfmt); // => pixel size in bits, 0 if unknown
uint8_t fmn_pixfmt_has_alpha(uint8_t pixfmt);

/* Client must call during init.
 * If it returns <0, you may try again with different parameters.
 * If it succeeds and you provided a zero-length range, you got that size exactly.
 * Nonzero range, you'll need to ask fmn_video_get_framebuffer_size().
 */
int8_t fmn_video_init(
  int16_t wmin,int16_t wmax,
  int16_t hmin,int16_t hmax,
  uint8_t pixfmt
);

/* Guaranteed not to change once established.
 */
void fmn_video_get_framebuffer_size(int16_t *w,int16_t *h);
uint8_t fmn_video_get_pixfmt();

/* All drawing commands expect pixels in the main framebuffer's format.
 * Clients are not expected to understand those pixels; use these converters.
 */
uint32_t fmn_video_rgba_from_pixel(uint32_t pixel);
uint32_t fmn_video_pixel_from_rgba(uint32_t rgba);

/* (0,0) if the image doesn't exist.
 * Calling this may cause it to decode.
 */
void fmn_video_get_image_size(int16_t *w,int16_t *h,uint16_t imageid);

void fmn_video_init_image(uint16_t imageid,int16_t w,int16_t h);

/* Select an image to render to.
 * imageid zero is the main framebuffer, and is always selected initially.
 */
int8_t fmn_draw_set_output(uint16_t imageid);

/* Skinny straight lines.
 */
struct fmn_draw_line {
  int16_t ax,ay,bx,by;
  uint32_t pixel;
};
void fmn_draw_line(const struct fmn_draw_line *v,int c);

/* Solid rectangles.
 */
struct fmn_draw_rect {
  int16_t x,y,w,h;
  uint32_t pixel;
};
void fmn_draw_rect(const struct fmn_draw_rect *v,int c);

/* Tiles from a 16x16-tile image.
 * Outputs at the natural size.
 */
struct fmn_draw_mintile {
  int16_t x,y; // center
  uint8_t tileid;
  uint8_t xform;
};
void fmn_draw_mintile(const struct fmn_draw_mintile *v,int c,uint16_t srcimageid);

/* Tiles from a 16x16-tile image, with all the bells and whistles.
 * (primary) replaces pure gray pixels such that 0x00000 and 0xffffff are unchanged, and 0x808080 turns into (pr,pg,pb).
 */
struct fmn_draw_maxtile {
  int16_t x,y; // center
  uint8_t tileid;
  uint8_t rotate; // 1/256 turns clockwise
  uint8_t size; // pixels
  uint8_t xform;
  uint8_t tr,tg,tb,ta; // tint
  uint8_t pr,pg,pb; // primary
  uint8_t alpha;
};
void fmn_draw_maxtile(const struct fmn_draw_maxtile *v,int c,uint16_t srcimageid);

/* Copy any rectangular region of one image to the output.
 * "swap" reverses the axes. It still respects the provided dimensions exactly.
 * You'll probably want to swap (dstw,dsth) on your own.
 * Use negative dimensions to flip on one axis.
 */
struct fmn_draw_decal {
  int16_t dstx,dsty,dstw,dsth;
  int16_t srcx,srcy,srcw,srch;
};
void fmn_draw_decal(const struct fmn_draw_decal *v,int c,uint16_t srcimageid);
void fmn_draw_decal_swap(const struct fmn_draw_decal *v,int c,uint16_t srcimageid);

/* Same as "decal", but replace all colors with the provided pixel.
 */
struct fmn_draw_recal {
  int16_t dstx,dsty,dstw,dsth;
  int16_t srcx,srcy,srcw,srch;
  uint32_t pixel;
};
void fmn_draw_recal(const struct fmn_draw_recal *v,int c,uint16_t srcimageid);
void fmn_draw_recal_swap(const struct fmn_draw_recal *v,int c,uint16_t srcimageid);
  

#endif

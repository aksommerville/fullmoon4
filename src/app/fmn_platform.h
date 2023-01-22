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
#include <math.h> /* I'll add implementations as they become needed. See src/platform/fmn_platform_libm.c */
void *memcpy(void *dst,const void *src,size_t c);
void *memmove(void *dst,const void *src,size_t c);
void *memset(void *dst,int src,size_t c);
int rand();

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

// PLANT_FRUIT corresponds to PITCHER_CONTENT, what you get by watering a plant with the given liquid.
#define FMN_PLANT_FRUIT_NONE    0
#define FMN_PLANT_FRUIT_SEED    1
#define FMN_PLANT_FRUIT_NITRO   2
#define FMN_PLANT_FRUIT_MATCH   3
#define FMN_PLANT_FRUIT_CORN    4
#define FMN_PLANT_FRUIT_COIN    5

#define FMN_PITCHER_CONTENT_EMPTY 0
#define FMN_PITCHER_CONTENT_WATER 1
#define FMN_PITCHER_CONTENT_MILK  2
#define FMN_PITCHER_CONTENT_HONEY 3
#define FMN_PITCHER_CONTENT_SAP   4
#define FMN_PITCHER_CONTENT_BLOOD 5

// Items with a qualifier are listed first.
#define FMN_ITEM_NONE            0
#define FMN_ITEM_CORN            1
#define FMN_ITEM_PITCHER         2
#define FMN_ITEM_SEED            3
#define FMN_ITEM_COIN            4
#define FMN_ITEM_MATCH           5
#define FMN_ITEM_QUALIFIER_COUNT 6
#define FMN_ITEM_BROOM           6
#define FMN_ITEM_WAND            7
#define FMN_ITEM_UMBRELLA        8
#define FMN_ITEM_FEATHER         9
#define FMN_ITEM_SHOVEL         10
#define FMN_ITEM_COMPASS        11
#define FMN_ITEM_VIOLIN         12
#define FMN_ITEM_CHALK          13
#define FMN_ITEM_BELL           14
#define FMN_ITEM_NITRO          15
#define FMN_ITEM_COUNT          16

#define FMN_SPRITE_STYLE_HIDDEN    0 /* don't render */
#define FMN_SPRITE_STYLE_TILE      1 /* single tile */
#define FMN_SPRITE_STYLE_HERO      2 /* special */
#define FMN_SPRITE_STYLE_FOURFRAME 3 /* 4 tiles arranged horizontally, automatic animation */

/* fmn_sprite_header is the part visible to the platform.
 * The app declares a more useful struct fmn_sprite with more content.
 */
#define FMN_SPRITE_HEADER \
  float x,y; /* midpoint in grid space */ \
  uint8_t style; \
  uint8_t imageid; \
  uint8_t tileid; \
  uint8_t xform; \
  uint16_t controller;
struct fmn_sprite_header { FMN_SPRITE_HEADER };

struct fmn_plant {
  uint16_t x;
  uint16_t y;
  uint32_t flower_time;
  uint8_t state;
  uint8_t fruit;
  uint8_t pad1[2];
};

struct fmn_sketch {
  uint16_t x;
  uint16_t y;
  uint32_t bits;
  uint32_t time; /* Timestamp of last touch, so we can overwrite in chronological order. */
};

struct fmn_door {
  uint8_t x;
  uint8_t y;
  uint16_t mapid;
  uint8_t dstx;
  uint8_t dsty;
  uint16_t pad;
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
  uint16_t pad1;
  uint8_t cellphysics[256];
  uint8_t sprite_storage[FMN_SPRITE_STORAGE_SIZE];
  struct fmn_door doorv[FMN_DOOR_LIMIT];
  uint32_t doorc;
  
  /* Current plants and sketches, loaded by platform.
   * You can modify these and the changes will persist, but you must not add or remove anything.
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
  uint8_t pad2[2];
  uint8_t itemv[16]; // nonzero if possessed
  uint8_t itemqv[16]; // qualifier eg count or enum
  
  /* More hero stuff for renderer.
   */
  uint8_t facedir; // FMN_DIR_*, cardinals only.
  uint8_t walking;
  uint8_t pad3[2];
  float injury_time;
  
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

/* Platform implements the rest.
 *************************************************/

// For debugging. Logs to JS console.
void fmn_log(const char *fmt,...);

// Hard stop. No further app calls will be made, and user will see an unfriendly error.
void fmn_abort();

/* Push a modal menu on the stack.
 * Game will not update while a menu is in play.
 * Variadic arguments are any number of (int prompt_stringid,void (*cb)()), followed by a null.
 */
void _fmn_begin_menu(int prompt,.../*int opt1,void (*cb1)(),...,int optN,void (*cbN)()*/);
#define fmn_begin_menu(...) _fmn_begin_menu(__VA_ARGS__,0)

/* Prepare a transition while in the "from" state, and declare what style you will want.
 * Then make your changes, and either commit or cancel it.
 * Platform decides whether to update during a transition, I'm thinking no.
 * If you prepare and then finish the frame without commit or cancel, it implicitly cancels.
 * Two prepares in one frame, the second cancels the first.
 */
void fmn_prepare_transition(int transition);
void fmn_commit_transition();
void fmn_cancel_transition();

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

// Call if you change map tiles. You don't need to track which ones.
void fmn_map_dirty();

/* Plant a seed or begin editing a sketch.
 * Both return <0 to reject, eg no space available.
 * Sketch editing is modal; you will stop receiving updates while it runs.
 */
int8_t fmn_add_plant(uint16_t x,uint16_t y);
int8_t fmn_begin_sketch(uint16_t x,uint16_t y);

#endif

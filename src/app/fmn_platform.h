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

#define FMN_INPUT_LEFT     0x01
#define FMN_INPUT_RIGHT    0x02
#define FMN_INPUT_UP       0x04
#define FMN_INPUT_DOWN     0x08
#define FMN_INPUT_USE      0x10
#define FMN_INPUT_MENU     0x20

#define FMN_TRANSITION_CUT        0
#define FMN_TRANSITION_PAN_LEFT   1
#define FMN_TRANSITION_PAN_RIGHT  2
#define FMN_TRANSITION_PAN_UP     3
#define FMN_TRANSITION_PAN_DOWN   4
#define FMN_TRANSITION_FADE_BLACK 5
#define FMN_TRANSITION_DOOR       6
#define FMN_TRANSITION_WARP       7

struct fmn_sprite {
  float x,y; // midpoint in grid space
  uint8_t imageid;
  uint8_t tileid;
  uint8_t xform;
};

struct fmn_scene {
  uint8_t map[FMN_COLC*FMN_ROWC];
  uint8_t maptsid;
  uint8_t songid;
  uint16_t neighborw;
  uint16_t neighbore;
  uint16_t neighborn;
  uint16_t neighbors;
  uint16_t pad1;
  struct fmn_sprite **spritev; // in render order
  int spritec;
};

/* App must define this.
 * The JS side will read it straight out of memory.
 * It is important that you not change the order or type of members, without updating the JS app!
 */
extern struct fmn_app_model {
  int TODO;
} fmn_app_model;

/* App must implement these hooks for platform to call.
 */
int fmn_init();
void fmn_update(int timems,int input);

/* Platform implements these.
 */
void fmn_log(const char *fmt,...);
void _fmn_begin_menu(int prompt,.../*int opt1,void (*cb1)(),...,int optN,void (*cbN)()*/);
#define fmn_begin_menu(...) _fmn_begin_menu(__VA_ARGS__,0)
void fmn_set_scene(struct fmn_scene *scene,int transition);

#endif

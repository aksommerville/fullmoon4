#ifndef INMGR_INTERNAL_H
#define INMGR_INTERNAL_H

#include "inmgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

struct inmgr_rules;
struct inmgr_rules_button;
struct inmgr_device;
struct inmgr_cap;
struct inmgr_map;

#define INMGR_SRCPART_BUTTON_ON 1 /* The usual case; a 2-state input or 1-way axis */
#define INMGR_SRCPART_BUTTON_OFF 2 /* Inverted 2-state */
#define INMGR_SRCPART_AXIS_LOW 3 /* Lower values of a 3-way axis, usually left or up (but sometimes down). */
#define INMGR_SRCPART_AXIS_MID 4 /* Middle range of a 3-way axis. It would be weird to map this. */
#define INMGR_SRCPART_AXIS_HIGH 5 /* Higher values of a 3-way axis, usually right or down (but sometimes up). */
#define INMGR_SRCPART_HAT_N 6 /* (0,1,7), UP on a hat */
#define INMGR_SRCPART_HAT_E 7 /* (1,2,3), RIGHT on a hat */
#define INMGR_SRCPART_HAT_S 8 /* (3,4,5), DOWN on a hat */
#define INMGR_SRCPART_HAT_W 9 /* (5,6,7), LEFT on a hat */

#define INMGR_DSTTYPE_BUTTON 1
#define INMGR_DSTTYPE_ACTION 2

/* Rules.
 ********************************************************/
 
struct inmgr_rules {
  char *name; // with wildcards
  int namec;
  uint16_t vid,pid; // zero matches all
  struct inmgr_rules_button { // sorted by (srcbtnid,srcpart), duplicates forbidden.
    int srcbtnid;
    uint8_t srcpart;
    uint8_t dsttype;
    uint16_t dstbtnid;
  } *buttonv;
  int buttonc,buttona;
};

void inmgr_rules_del(struct inmgr_rules *rules);
struct inmgr_rules *inmgr_rules_new();

int inmgr_rules_set_name(struct inmgr_rules *rules,const char *src,int srcc);
struct inmgr_rules_button *inmgr_rules_add_button(struct inmgr_rules *rules,int srcbtnid,uint8_t srcpart);

int inmgr_rules_buttonv_search(const struct inmgr_rules *rules,int srcbtnid,uint8_t srcpart);
struct inmgr_rules_button *inmgr_rules_buttonv_insert(struct inmgr_rules *rules,int p,int srcbtnid,uint8_t srcpart);

int inmgr_rules_add_from_map(struct inmgr_rules *rules,const struct inmgr_map *map,const struct inmgr_device *device);

/* Live devices.
 ***********************************************************/
 
#define INMGR_CAP_ROLE_IGNORE 0 /* Don't use this button, we don't know how. */
#define INMGR_CAP_ROLE_BUTTON 1 /* Two-state button or one-way axis. */
#define INMGR_CAP_ROLE_AXIS 2 /* Three-way axis. */
#define INMGR_CAP_ROLE_HAT 3 /* Rotational 9-state hat. */
 
struct inmgr_device {
  int devid;
  char *name;
  int namec;
  uint16_t vid,pid;
  struct inmgr_cap {
    int btnid;
    int usage;
    int lo,hi,rest;
    int role;
  } *capv;
  int capc,capa;
};

void inmgr_device_del(struct inmgr_device *device);
struct inmgr_device *inmgr_device_new(int devid,const char *name,int namec,uint16_t vid,uint16_t pid);

int inmgr_device_set_name(struct inmgr_device *device,const char *src,int srcc);
struct inmgr_cap *inmgr_device_add_capability(struct inmgr_device *device,int btnid,int usage,int lo,int hi,int rest);

struct inmgr_cap *inmgr_device_get_capability(const struct inmgr_device *device,int btnid);

int inmgr_guess_capability_role(const struct inmgr_cap *cap);

/* Live maps.
 * During normal event processing, we do not examine rules or devices.
 * All mappable inputs are stored in this flat list.
 *****************************************************************/
 
struct inmgr_map {
  int devid;
  int srcbtnid;
  int srcvalue;
  int srclo,srchi; // For N hats, range would be discontiguous. Those must have (srchi<srclo), and (srclo) is the middle value, normally zero.
  uint8_t dsttype;
  uint8_t dstvalue;
  uint16_t dstbtnid;
};

/* Context.
 *******************************************************************/
 
struct inmgr {
  struct inmgr_delegate delegate;
  
  uint16_t state;
  
  struct inmgr_map *mapv; // Sorted by (devid,srcbtnid), and duplicates permitted. (expected to occupy different src ranges)
  int mapc,mapa;
  
  struct inmgr_rules **rulesv; // First match wins.
  int rulesc,rulesa;
  
  struct inmgr_device **devicev;
  int devicec,devicea;
  
  struct inmgr_device *device_pending; // STRONG, not listed yet, handshake in progress
  struct inmgr_rules *rules_pending; // WEAK, listed, rules matching (device_pending), if any exists
  
  int live_config_status;
  uint16_t live_config_btnid;
  struct inmgr_device *live_config_device; // WEAK
  int live_config_await_btnid; // given srcbtnid is pressed and must be released before we'll move on
  int live_config_confirm_btnid; // populated during "AGAIN" stages and must match.
  uint8_t live_config_await_srcpart;
  uint8_t live_config_confirm_srcpart;
  struct inmgr_map *deathrow_mapv; // maps removed at start of live config. will restore if cancelled.
  int deathrow_mapc,deathrow_mapa;
};

/* Searching maps always returns the lowest index, if any exists.
 */
int inmgr_mapv_search(const struct inmgr *inmgr,int devid,int srcbtnid);
struct inmgr_map *inmgr_mapv_insert(struct inmgr *inmgr,int p,int devid,int srcbtnid);
void inmgr_mapv_remove_devid(struct inmgr *inmgr,int devid); // may trigger state callback

/* Convenience to add a mapping in one line.
 * We violate isolation a little; you have to include fmn_platform.h for button IDs and bigpc_internal.h for action IDs.
 */
int inmgr_mapv_add(struct inmgr *inmgr,int devid,int srcbtnid,int srcvalue,int srclo,int srchi,uint8_t dsttype,uint8_t dstvalue,uint16_t dstbtnid);
#define inmgr_mapv_add_button(devid,srcbtnid,btntag) inmgr_mapv_add(inmgr,devid,srcbtnid,0,1,INT_MAX,INMGR_DSTTYPE_BUTTON,0,FMN_INPUT_##btntag)
#define inmgr_mapv_add_action(devid,srcbtnid,actiontag) inmgr_mapv_add(inmgr,devid,srcbtnid,0,1,INT_MAX,INMGR_DSTTYPE_ACTION,0,BIGPC_ACTIONID_##actiontag)

/* Generate live maps for a new device.
 */
int inmgr_map_device(struct inmgr *inmgr,const struct inmgr_device *device,const struct inmgr_rules *rules);
int inmgr_map_device_ruleless(struct inmgr *inmgr,const struct inmgr_device *device);

/* Set all the nonzero bits of (btnids) to (value).
 * If current state already agrees, noop.
 * Otherwise adjust state and trigger callbacks.
 * For the 'single' version, caller promises to only call with single-bit (btnid).
 */
void inmgr_set_state_multiple_with_callbacks(struct inmgr *inmgr,uint16_t btnids,uint8_t value);
void inmgr_set_state_single_with_callbacks(struct inmgr *inmgr,uint16_t btnid,uint8_t value);

struct inmgr_rules *inmgr_rulesv_match_device(const struct inmgr *inmgr,const struct inmgr_device *device);
struct inmgr_rules *inmgr_rulesv_new(struct inmgr *inmgr,int p); // (p==0) for start, (p<0) for end, or a real insertion point

struct inmgr_device *inmgr_devicev_search_devid(const struct inmgr *inmgr,int devid);
int inmgr_devicev_add(struct inmgr *inmgr,struct inmgr_device *device); // handoff
void inmgr_devicev_remove_devid(struct inmgr *inmgr,int devid); // deletes device if present

int inmgr_pattern_match(const char *pat,int patc,const char *src,int srcc);

/* (devid) would already be checked and probly shouldn't even be a parameter here.
 */
void inmgr_live_config_event(struct inmgr *inmgr,int devid,int btnid,int value);

int inmgr_deathrow_mapv_rebuild(struct inmgr *inmgr,int devid);
int inmgr_deathrow_mapv_restore(struct inmgr *inmgr);

#endif

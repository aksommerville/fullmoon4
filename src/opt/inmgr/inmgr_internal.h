#ifndef INMGR_INTERNAL_H
#define INMGR_INTERNAL_H

#include "inmgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define INMGR_MAP_MODE_IGNORE     0 /* don't use; just don't add the map if it's disabled */
#define INMGR_MAP_MODE_TWOSTATE   1 /* (srclo<=srcvalue<=srchi)?1:0 */
#define INMGR_MAP_MODE_TWOREV     2 /* Same as TWOSTATE, but anything in bounds is OFF and out of bounds ON */
#define INMGR_MAP_MODE_THREESTATE 3 /* (srcvalue<=srclo)?-1:(srcvalue>=srchi)?1:0, (dstbtnid) must be horz or vert */
#define INMGR_MAP_MODE_THREEREV   4 /* Same as THREESTATE, but (RIGHT,0,LEFT) and (DOWN,0,UP) */
#define INMGR_MAP_MODE_HAT        5 /* (srclo+7==srchi), (N,NE,E,SE,S,SW,W,NW), OOB means (0,0), (dstbtnid) must be dpad */
#define INMGR_MAP_MODE_ACTION     6 /* TWOSTATE but (dstbtnid) is an action, not a button */
#define INMGR_MAP_MODE_ACTIONREV  7 /* TWOREV but '' */

#define INMGR_DEVICE_NAME_LIMIT 64

struct inmgr_map {
  int srcdevid;
  int srcbtnid;
  int srcvalue;
  int srclo,srchi;
  uint8_t mode;
  uint8_t dstplayerid;
  uint16_t dstbtnid;
  int8_t dstvalue;
};

struct inmgr_device {
  int devid;
  uint16_t vid,pid;
  char name[INMGR_DEVICE_NAME_LIMIT];
  int namec;
  struct inmgr_map *mapv;
  int mapc,mapa;
  // Might need to record the complete button capability for everything reported,
  // if we're going to make decisions scoped across multiple inputs.
  // For now at least, let's see if we can keep the decision finite-state. Map each button as it gets reported.
  //TODO Definitely need to find and retain the rules template here.
};

struct inmgr {
  struct inmgr_delegate delegate;
  int ready;
  
  uint16_t btnid_horz,btnid_vert,btnid_dpad;
  
  uint16_t player_statev[1+INMGR_PLAYER_LIMIT];
  
  /* Live input maps, one for each button on each device.
   * Sorted by (srcdevid,srcbtnid) but there can be more than one per.
   */
  struct inmgr_map *mapv;
  int mapc,mapa;
  
  /* Null or the just-connected device in process of configuration. (strong)
   */
  struct inmgr_device *pending_device;
  
  //TODO "rule" for uninstantiated maps
};

int inmgr_maps_acceptable(struct inmgr *inmgr,const struct inmgr_map *mapv,int mapc);

/* Add multiple maps.
 * Incoming must be on the same (srcdevid), must be sorted, and that (srcdevid) must not be mapped yet.
 */
int inmgr_add_maps(struct inmgr *inmgr,const struct inmgr_map *mapv,int mapc);

/* Remove all live maps for a source device.
 * Returns a mask of playerid which had a map removed, caller should blank those player states.
 * *** This does not update states or fire callbacks. Caller is expected to. ***
 */
uint16_t inmgr_remove_maps(struct inmgr *inmgr,int devid);

/* Update player state and fire callbacks for multiple button bits.
 * This does not update player zero (unless playerid is zero).
 */
void inmgr_trigger_multiple_state_changes(struct inmgr *inmgr,uint8_t playerid,uint16_t mask,uint8_t value);

/* Update player state and fire callbacks for one button bit.
 * This **does** update player zero too, if (playerid) nonzero.
 */
void inmgr_change_player_state(struct inmgr *inmgr,uint8_t playerid,uint16_t mask,uint8_t value);

/* Find all maps for a device or device+button.
 * Gives the correct insertion point at (*dstpp) even when result is zero.
 */
int inmgr_mapv_search_srcdevid(struct inmgr_map **dstpp,const struct inmgr *inmgr,int srcdevid);
int inmgr_mapv_search_srcdevid_srcbtnid(struct inmgr_map **dstpp,const struct inmgr *inmgr,int srcdevid,int srcbtnid);

void inmgr_device_del(struct inmgr_device *device);
struct inmgr_device *inmgr_device_new(int devid,uint16_t vid,uint16_t pid,const char *name,int namec);
int inmgr_device_add_capability(struct inmgr_device *device,int btnid,uint32_t usage,int lo,int hi,int value);
int inmgr_device_finalize_maps(struct inmgr_device *device);

#endif

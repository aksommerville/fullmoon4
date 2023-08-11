#include "inmgr_internal.h"

/* Receive event.
 */
 
void inmgr_event(struct inmgr *inmgr,int devid,int btnid,int value) {

  if (inmgr->live_config_status&&inmgr->live_config_device&&(devid==inmgr->live_config_device->devid)) {
    inmgr_live_config_event(inmgr,devid,btnid,value);
    return;
  }

  int p=inmgr_mapv_search(inmgr,devid,btnid);
  if (p<0) return;
  struct inmgr_map *map=inmgr->mapv+p;
  for (;(p<inmgr->mapc)&&(map->srcbtnid==btnid);p++,map++) {
    if (value==map->srcvalue) continue;
    map->srcvalue=value;
    uint8_t dstvalue;
    
    // Special case for North edge of hats.
    if (map->srclo>map->srchi) {
      switch (value-map->srclo) {
        case 0: case 1: case 7: dstvalue=1; break;
        default: dstvalue=0;
      }
      
    // All other source button types are expressed as an inclusive range.
    } else if ((value>=map->srclo)&&(value<=map->srchi)) {
      dstvalue=1;
    } else {
      dstvalue=0;
    }
    
    if (dstvalue==map->dstvalue) continue;
    map->dstvalue=dstvalue;
    
    switch (map->dsttype) {
      case INMGR_DSTTYPE_BUTTON: inmgr_set_state_single_with_callbacks(inmgr,map->dstbtnid,dstvalue); break;
      case INMGR_DSTTYPE_ACTION: if (dstvalue) inmgr->delegate.action(inmgr,map->dstbtnid); break;
    }
  }
}

/* Connect device. Start of handshake.
 */

void inmgr_connect(struct inmgr *inmgr,int devid,const char *name,int namec,uint16_t vid,uint16_t pid) {
  if (inmgr->device_pending) {
    inmgr_device_del(inmgr->device_pending);
    inmgr->device_pending=0;
  }
  inmgr->rules_pending=0;
  if (inmgr_devicev_search_devid(inmgr,devid)) return;
  if (!(inmgr->device_pending=inmgr_device_new(devid,name,namec,vid,pid))) return;
  inmgr->rules_pending=inmgr_rulesv_match_device(inmgr,inmgr->device_pending);
}

/* Add capability to device.
 */
 
void inmgr_add_capability(struct inmgr *inmgr,int btnid,int usage,int lo,int hi,int rest) {
  if (!inmgr->device_pending) return;
  struct inmgr_cap *cap=inmgr_device_add_capability(inmgr->device_pending,btnid,usage,lo,hi,rest);
  if (!cap) return;
  cap->role=inmgr_guess_capability_role(cap);
}

/* Connect device: End of handshake.
 */
 
int inmgr_ready(struct inmgr *inmgr) {
  if (!inmgr->device_pending) return 0;
  if (inmgr_devicev_add(inmgr,inmgr->device_pending)<0) {
    inmgr_device_del(inmgr->device_pending);
    inmgr->device_pending=0;
    inmgr->rules_pending=0;
    return 0;
  }
  if (inmgr->rules_pending) {
    inmgr_map_device(inmgr,inmgr->device_pending,inmgr->rules_pending);
    fprintf(stderr,"Input device '%.*s' mapped per config.\n",inmgr->device_pending->namec,inmgr->device_pending->name);
  } else {
    if (inmgr_map_device_ruleless(inmgr,inmgr->device_pending)>0) {
      fprintf(stderr,"Config not found for input device '%.*s'. Mapped with defaults.\n",inmgr->device_pending->namec,inmgr->device_pending->name);
      // If available, abuse live_config to generate rules.
      if (inmgr->live_config_status==INMGR_LIVE_CONFIG_NONE) {
        inmgr->live_config_status=INMGR_LIVE_CONFIG_SUCCESS;
        inmgr->live_config_device=inmgr->device_pending;
        inmgr_rewrite_rules_for_live_config(inmgr);
        inmgr->live_config_status=INMGR_LIVE_CONFIG_NONE;
        inmgr->live_config_device=0;
        inmgr->device_pending=0;
        inmgr->rules_pending=0;
        return 1;
      }
    } else {
      fprintf(stderr,"Failed to configure input device '%.*s'.\n",inmgr->device_pending->namec,inmgr->device_pending->name);
    }
  }
  inmgr->device_pending=0;
  inmgr->rules_pending=0;
  return 0;
}

/* Disconnect device.
 */

void inmgr_disconnect(struct inmgr *inmgr,int devid) {
  inmgr_devicev_remove_devid(inmgr,devid);
  inmgr_mapv_remove_devid(inmgr,devid);
}

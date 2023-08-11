#include "inmgr_internal.h"

/* Commit a live config map.
 */
 
static void inmgr_live_config_commit(struct inmgr *inmgr,int srcbtnid,uint8_t srcpart) {
  const struct inmgr_cap *cap=inmgr_device_get_capability(inmgr->live_config_device,srcbtnid);
  if (cap) {
    switch (srcpart) {
      case INMGR_SRCPART_BUTTON_ON: {
          inmgr_mapv_add(inmgr,inmgr->live_config_device->devid,srcbtnid,cap->lo,cap->lo+1,INT_MAX,INMGR_DSTTYPE_BUTTON,0,inmgr->live_config_btnid);
        } break;
      case INMGR_SRCPART_AXIS_LOW: {
          int mid=(cap->lo+cap->hi)>>1;
          int midlo=(cap->lo+mid)>>1;
          if (midlo>=mid) midlo=mid-1;
          inmgr_mapv_add(inmgr,inmgr->live_config_device->devid,srcbtnid,mid,INT_MIN,midlo,INMGR_DSTTYPE_BUTTON,0,inmgr->live_config_btnid);
        } break;
      case INMGR_SRCPART_AXIS_HIGH: {
          int mid=(cap->lo+cap->hi)>>1;
          int midhi=(cap->hi+mid)>>1;
          if (midhi<=mid) midhi=mid+1;
          inmgr_mapv_add(inmgr,inmgr->live_config_device->devid,srcbtnid,mid,midhi,INT_MAX,INMGR_DSTTYPE_BUTTON,0,inmgr->live_config_btnid);
        } break;
      case INMGR_SRCPART_HAT_N: {
          inmgr_mapv_add(inmgr,inmgr->live_config_device->devid,srcbtnid,cap->lo-1,cap->lo,cap->lo-1,INMGR_DSTTYPE_BUTTON,0,inmgr->live_config_btnid);
        } break;
      case INMGR_SRCPART_HAT_E: {
          inmgr_mapv_add(inmgr,inmgr->live_config_device->devid,srcbtnid,cap->lo-1,cap->lo+1,cap->lo+3,INMGR_DSTTYPE_BUTTON,0,inmgr->live_config_btnid);
        } break;
      case INMGR_SRCPART_HAT_S: {
          inmgr_mapv_add(inmgr,inmgr->live_config_device->devid,srcbtnid,cap->lo-1,cap->lo+3,cap->lo+5,INMGR_DSTTYPE_BUTTON,0,inmgr->live_config_btnid);
        } break;
      case INMGR_SRCPART_HAT_W: {
          inmgr_mapv_add(inmgr,inmgr->live_config_device->devid,srcbtnid,cap->lo-1,cap->lo+5,cap->lo+7,INMGR_DSTTYPE_BUTTON,0,inmgr->live_config_btnid);
        } break;
    }
  } else {
    inmgr_mapv_add(inmgr,inmgr->live_config_device->devid,srcbtnid,0,1,INT_MAX,INMGR_DSTTYPE_BUTTON,0,inmgr->live_config_btnid);
  }
  inmgr->live_config_await_btnid=0;
  inmgr->live_config_await_srcpart=0;
  inmgr->live_config_confirm_btnid=0;
  inmgr->live_config_confirm_srcpart=0;
  inmgr->live_config_btnid<<=1;
  while (inmgr->live_config_btnid&&!(inmgr->live_config_btnid&inmgr->delegate.all_btnid)) inmgr->live_config_btnid<<=1;
  if (inmgr->live_config_btnid) {
    inmgr->live_config_status=INMGR_LIVE_CONFIG_PRESS;
  } else {
    inmgr->live_config_status=INMGR_LIVE_CONFIG_SUCCESS;
  }
}

/* Digested live-config events.
 */
 
static void inmgr_live_config_receive_press(struct inmgr *inmgr,int btnid,uint8_t srcpart) {
  inmgr->live_config_await_btnid=btnid;
  inmgr->live_config_await_srcpart=srcpart;
  switch (inmgr->live_config_status) {
    case INMGR_LIVE_CONFIG_FAULT:
    case INMGR_LIVE_CONFIG_PRESS: {
        inmgr->live_config_status=INMGR_LIVE_CONFIG_RELEASE;
        inmgr->live_config_confirm_btnid=btnid;
        inmgr->live_config_confirm_srcpart=srcpart;
      } break;
    case INMGR_LIVE_CONFIG_PRESS_AGAIN: {
        inmgr->live_config_status=INMGR_LIVE_CONFIG_RELEASE_AGAIN;
      } break;
  }
}

static void inmgr_live_config_receive_release(struct inmgr *inmgr,int btnid) {
  switch (inmgr->live_config_status) {
    case INMGR_LIVE_CONFIG_RELEASE: {
        inmgr->live_config_status=INMGR_LIVE_CONFIG_PRESS_AGAIN;
      } break;
    case INMGR_LIVE_CONFIG_RELEASE_AGAIN: {
        if ((btnid==inmgr->live_config_confirm_btnid)&&(inmgr->live_config_await_srcpart==inmgr->live_config_confirm_srcpart)) {
          inmgr_live_config_commit(inmgr,btnid,inmgr->live_config_confirm_srcpart);
        } else {
          inmgr->live_config_status=INMGR_LIVE_CONFIG_FAULT;
        }
        inmgr->live_config_confirm_btnid=0;
        inmgr->live_config_confirm_srcpart=0;
      } break;
  }
  inmgr->live_config_await_btnid=0;
  inmgr->live_config_await_srcpart=0;
}

/* Event.
 */
 
void inmgr_live_config_event(struct inmgr *inmgr,int devid,int btnid,int value) {
  const struct inmgr_device *device=inmgr->live_config_device;
  const struct inmgr_cap *cap=inmgr_device_get_capability(device,btnid);
  
  if (!cap) { // No cap, assume it's a two-state button with zero for OFF.
    if (inmgr->live_config_await_btnid) {
      if ((btnid==inmgr->live_config_await_btnid)&&!value) {
        inmgr_live_config_receive_release(inmgr,btnid);
      } else {
        return;
      }
    }
    if (value) {
      inmgr_live_config_receive_press(inmgr,btnid,INMGR_SRCPART_BUTTON_ON);
    }
    return;
  }
  
  switch (cap->role) {
    case INMGR_CAP_ROLE_BUTTON: {
        if (inmgr->live_config_await_btnid) {
          if ((btnid==inmgr->live_config_await_btnid)&&(value==cap->lo)) {
            inmgr_live_config_receive_release(inmgr,btnid);
          } else {
            return;
          }
        }
        if (value>cap->lo) {
          inmgr_live_config_receive_press(inmgr,btnid,INMGR_SRCPART_BUTTON_ON);
        }
      } break;
    
    case INMGR_CAP_ROLE_AXIS: {
        int mid=(cap->lo+cap->hi)>>1;
        int midlo=(cap->lo+mid)>>1;
        int midhi=(cap->hi+mid)>>1;
        if (midlo>=mid) midlo=mid-1;
        if (midhi<=mid) midhi=mid+1;
        if (inmgr->live_config_await_btnid) {
          if ((btnid==inmgr->live_config_await_btnid)&&(value>midlo)&&(value<midhi)) {
            inmgr_live_config_receive_release(inmgr,btnid);
          }
          return;
        }
        if (value<=midlo) {
          inmgr_live_config_receive_press(inmgr,btnid,INMGR_SRCPART_AXIS_LOW);
        } else if (value>=midhi) {
          inmgr_live_config_receive_press(inmgr,btnid,INMGR_SRCPART_AXIS_HIGH);
        }
      } break;
    
    case INMGR_CAP_ROLE_HAT: {
        int srcpart=0;
        switch (value-cap->lo) {
          case 0: srcpart=INMGR_SRCPART_HAT_N; break;
          case 2: srcpart=INMGR_SRCPART_HAT_E; break;
          case 4: srcpart=INMGR_SRCPART_HAT_S; break;
          case 6: srcpart=INMGR_SRCPART_HAT_W; break;
          default: srcpart=0; // For live-config purposes, diagonals are the same as "off"
        }
        if (inmgr->live_config_await_btnid) {
          if ((btnid==inmgr->live_config_await_btnid)&&!srcpart) {
            inmgr_live_config_receive_release(inmgr,btnid);
          }
          return;
        }
        if (srcpart) {
          inmgr_live_config_receive_press(inmgr,btnid,srcpart);
        }
      } break;
  }
}

/* Begin.
 */
 
void inmgr_live_config_begin(struct inmgr *inmgr,int devid) {
  struct inmgr_device *device=inmgr_devicev_search_devid(inmgr,devid);
  if (!device) return;
  inmgr->live_config_status=INMGR_LIVE_CONFIG_PRESS;
  inmgr->live_config_device=device;
  inmgr->live_config_btnid=1;
  inmgr->live_config_await_btnid=0;
  inmgr->live_config_confirm_btnid=0;
  while (!(inmgr->live_config_btnid&inmgr->delegate.all_btnid)) inmgr->live_config_btnid<<=1;
  inmgr_deathrow_mapv_rebuild(inmgr,devid);
  inmgr_mapv_remove_devid(inmgr,devid);
}

/* End.
 */
 
void inmgr_live_config_end(struct inmgr *inmgr) {
  if (inmgr->live_config_status==INMGR_LIVE_CONFIG_NONE) return;
  if (inmgr->live_config_status!=INMGR_LIVE_CONFIG_SUCCESS) {
    inmgr_deathrow_mapv_restore(inmgr);
  }
  inmgr->live_config_status=INMGR_LIVE_CONFIG_NONE;
  inmgr->live_config_btnid=0;
  inmgr->live_config_device=0;
  inmgr->live_config_await_btnid=0;
  inmgr->live_config_confirm_btnid=0;
  inmgr->deathrow_mapc=0;
}

/* Get status.
 */

int inmgr_live_config_status(uint16_t *btnid,const struct inmgr *inmgr) {
  *btnid=inmgr->live_config_btnid;
  return inmgr->live_config_status;
}

/* Rewrite rules.
 */
 
int inmgr_rewrite_rules_for_live_config(struct inmgr *inmgr) {
  if (inmgr->live_config_status==INMGR_LIVE_CONFIG_NONE) return 0;
  if (!inmgr->live_config_device) return 0;
  struct inmgr_device *device=inmgr->live_config_device;
  
  // Drop any rules set which matches this device exactly.
  // Only exact matches here! If we have a matching but non-specific rules, keep it.
  int i=inmgr->rulesc; while (i-->0) {
    struct inmgr_rules *rules=inmgr->rulesv[i];
    if (rules->vid!=device->vid) continue;
    if (rules->pid!=device->pid) continue;
    if (rules->namec!=device->namec) continue;
    if (memcmp(rules->name,device->name,device->namec)) continue;
    inmgr->rulesc--;
    memmove(inmgr->rulesv+i,inmgr->rulesv+i+1,sizeof(void*)*(inmgr->rulesc-i));
    inmgr_rules_del(rules);
    if (inmgr->rules_pending==rules) inmgr->rules_pending=0;
  }
  
  // Make a new rules at the start of the list, and match this device exactly.
  struct inmgr_rules *rules=inmgr_rulesv_new(inmgr,0);
  if (!rules) return -1;
  inmgr_rules_set_name(rules,device->name,device->namec);
  rules->vid=device->vid;
  rules->pid=device->pid;
  
  // Add a rule for each existing map.
  int p=inmgr_mapv_search(inmgr,device->devid,0);
  if (p<0) p=-p-1;
  const struct inmgr_map *map=inmgr->mapv+p;
  for (;(p<inmgr->mapc)&&(map->devid==device->devid);p++,map++) {
    if (inmgr_rules_add_from_map(rules,map,device)<0) return -1;
  }
  
  return 0;
}

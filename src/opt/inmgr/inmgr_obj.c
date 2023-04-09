#include "inmgr_internal.h"

/* Delete.
 */

void inmgr_del(struct inmgr *inmgr) {
  if (!inmgr) return;
  
  if (inmgr->mapv) free(inmgr->mapv);
  
  free(inmgr);
}

/* New.
 */

struct inmgr *inmgr_new(const struct inmgr_delegate *delegate) {
  if (!delegate) return 0;
  if (!delegate->state_change||!delegate->action) return 0;
  struct inmgr *inmgr=calloc(1,sizeof(struct inmgr));
  if (!inmgr) return 0;
  
  inmgr->delegate=*delegate;
  inmgr->btnid_horz=delegate->btnid_left|delegate->btnid_right;
  inmgr->btnid_vert=delegate->btnid_up|delegate->btnid_down;
  inmgr->btnid_dpad=inmgr->btnid_horz|inmgr->btnid_vert;
  
  return inmgr;
}

/* Add configuration.
 */

int inmgr_configure_text(struct inmgr *inmgr,const char *src,int srcc,const char *refname) {
  if (!inmgr||!inmgr->ready) return -1;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (refname&&!refname[0]) refname="<input config>";
  return 0;//TODO inmgr config
}

/* Finalize configuration.
 */

int inmgr_ready(struct inmgr *inmgr) {
  if (!inmgr) return -1;
  if (inmgr->ready) return 0;
  //TODO inmgr ready
  inmgr->ready=1;
  return 0;
}

/* Receive event from raw sources.
 */

int inmgr_event(struct inmgr *inmgr,int devid,int btnid,int value) {
  struct inmgr_map *map=0;
  int i=inmgr_mapv_search_srcdevid_srcbtnid(&map,inmgr,devid,btnid);
  for (;i-->0;map++) {
    if (map->srcvalue==value) continue;
    map->srcvalue=value;
    int8_t dstvalue=map->dstvalue;
    uint8_t apply_mode=0; // convenience, spares us a second select block on map->mode
    switch (map->mode) {
      case INMGR_MAP_MODE_ACTION: {
          dstvalue=((value>=map->srclo)&&(value<=map->srchi))?1:0;
          apply_mode=1;
        } break;
      case INMGR_MAP_MODE_ACTIONREV: {
          dstvalue=((value>=map->srclo)&&(value<=map->srchi))?0:1;
          apply_mode=1;
        } break;
      case INMGR_MAP_MODE_TWOSTATE: {
          dstvalue=((value>=map->srclo)&&(value<=map->srchi))?1:0;
          apply_mode=2;
        } break;
      case INMGR_MAP_MODE_TWOREV: {
          dstvalue=((value>=map->srclo)&&(value<=map->srchi))?0:1;
          apply_mode=2;
        } break;
      case INMGR_MAP_MODE_THREESTATE: {
          dstvalue=(value<=map->srclo)?-1:(value>=map->srchi)?1:0;
          apply_mode=3;
        } break;
      case INMGR_MAP_MODE_THREEREV: {
          dstvalue=(value<=map->srclo)?1:(value>=map->srchi)?-1:0;
          apply_mode=3;
        } break;
      case INMGR_MAP_MODE_HAT: {
          dstvalue=value-map->srclo;
          apply_mode=4;
        } break;
    }
    if (map->dstvalue==dstvalue) continue;
    map->dstvalue=dstvalue;
    
    switch (apply_mode) {
      case 1: if (dstvalue) inmgr->delegate.action(inmgr->delegate.userdata,map->dstplayerid,map->dstbtnid); break;
      case 2: inmgr_change_player_state(inmgr,map->dstplayerid,map->dstbtnid,dstvalue); break;
      case 3: {
          uint16_t btnidlo=(map->dstbtnid&(inmgr->delegate.btnid_left|inmgr->delegate.btnid_up));
          uint16_t btnidhi=(map->dstbtnid&(inmgr->delegate.btnid_right|inmgr->delegate.btnid_down));
          if (dstvalue<0) {
            inmgr_change_player_state(inmgr,map->dstplayerid,btnidhi,0);
            inmgr_change_player_state(inmgr,map->dstplayerid,btnidlo,1);
          } else if (dstvalue>0) {
            inmgr_change_player_state(inmgr,map->dstplayerid,btnidlo,0);
            inmgr_change_player_state(inmgr,map->dstplayerid,btnidhi,1);
          } else {
            inmgr_change_player_state(inmgr,map->dstplayerid,btnidlo,0);
            inmgr_change_player_state(inmgr,map->dstplayerid,btnidhi,0);
          }
        } break;
      case 4: {
          int8_t x=0,y=0;
          switch (dstvalue) {
            case 0: y=-1; break;
            case 1: x=1; y=-1; break;
            case 2: x=1; break;
            case 3: x=1; y=1; break;
            case 4: y=1; break;
            case 5: x=-1; y=1; break;
            case 6: x=-1; break;
            case 7: x=-1; y=-1; break;
          }
          if (x<0) {
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_right,0);
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_left,1);
          } else if (x>0) {
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_left,0);
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_right,1);
          } else {
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_left,0);
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_right,0);
          }
          if (y<0) {
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_down,0);
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_up,1);
          } else if (y>0) {
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_up,0);
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_down,1);
          } else {
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_up,0);
            inmgr_change_player_state(inmgr,map->dstplayerid,inmgr->delegate.btnid_down,0);
          }
        } break;
    }
  }
  return 0;
}

/* Check whether a set of maps is OK to start using.
 */

int inmgr_maps_acceptable(struct inmgr *inmgr,const struct inmgr_map *mapv,int mapc) {
  if (!inmgr||!mapv||(mapc<1)) return 0;
  if (!inmgr->delegate.btnid_required) return 1; // anything goes
  uint16_t buttons=0;
  for (;mapc-->0;mapv++) {
    switch (mapv->mode) {
      case INMGR_MAP_MODE_ACTION:
      case INMGR_MAP_MODE_ACTIONREV:
        break;
      default: // everything else is a btnid
        buttons|=mapv->dstbtnid;
    }
  }
  if (inmgr->delegate.btnid_required&~buttons) return 0;
  return 1;
}

/* Add multiple maps.
 */
 
int inmgr_add_maps(struct inmgr *inmgr,const struct inmgr_map *mapv,int mapc) {
  if (mapc<1) return 0;
  // Validate all the same (srcdevid) and sorted by (srcbtnid).
  const struct inmgr_map *q=mapv+1;
  int i=mapc-1;
  for (;i-->0;q++) {
    if (q[0].srcdevid!=q[-1].srcdevid) return -1;
    if (q[0].srcbtnid<q[-1].srcbtnid) return -1; // sic < not <=, multiple maps on the same button are allowed
  }
  if (inmgr->mapc>inmgr->mapa-mapc) {
    if (inmgr->mapc>INT_MAX-mapc) return -1; // whaaaa...
    int na=inmgr->mapc+mapc;
    if (na<INT_MAX-256) na=(na+256)&~255;
    if (na>INT_MAX/sizeof(struct inmgr_map)) return -1;
    void *nv=realloc(inmgr->mapv,sizeof(struct inmgr_map)*na);
    if (!nv) return -1;
    inmgr->mapv=nv;
    inmgr->mapa=na;
  }
  struct inmgr_map *p=0;
  int c=inmgr_mapv_search_srcdevid(&p,inmgr,mapv->srcdevid);
  if (c) return -1; // Device must not be mapped yet. (in order to spare us from needing to interleave by srcbtnid).
  int insp=p-inmgr->mapv;
  memmove(p+mapc,p,sizeof(struct inmgr_map)*(inmgr->mapc-insp));
  inmgr->mapc+=mapc;
  memcpy(p,mapv,sizeof(struct inmgr_map)*mapc);
  return 0;
}

/* Remove maps for a device.
 */
 
uint16_t inmgr_remove_maps(struct inmgr *inmgr,int devid) {
  struct inmgr_map *mapv=0;
  int mapc=inmgr_mapv_search_srcdevid(&mapv,inmgr,devid);
  if (mapc<=0) return 0;
  uint16_t playeridbits=0;
  int mapp=mapv-inmgr->mapv;
  const struct inmgr_map *p=mapv;
  int i=mapc;
  for (;i-->0;p++) playeridbits|=1<<p->dstplayerid;
  inmgr->mapc-=mapc;
  memmove(mapv,mapv+mapc,sizeof(struct inmgr_map)*(inmgr->mapc-mapp));
  return playeridbits;
}

/* Update multiple buttons on one player.
 */
 
void inmgr_trigger_multiple_state_changes(struct inmgr *inmgr,uint8_t playerid,uint16_t mask,uint8_t value) {
  if (playerid>INMGR_PLAYER_LIMIT) return;
  if (value) {
    if (mask=mask&~inmgr->player_statev[playerid]) {
      inmgr->player_statev[playerid]|=mask;
    } else return;
  } else {
    if (mask=mask&inmgr->player_statev[playerid]) {
      inmgr->player_statev[playerid]&=~mask;
    } else return;
  }
  // It's maybe incorrect of us, if more than one button changes we report (state) on each change as the final state.
  // I feel like that won't matter.
  uint16_t btnid=1;
  for (;btnid<=mask;btnid<<=1) {
    if (btnid&mask) {
      inmgr->delegate.state_change(inmgr->delegate.userdata,playerid,btnid,value,inmgr->player_statev[playerid]);
    }
  }
}

/* Single state change.
 */
 
void inmgr_change_player_state(struct inmgr *inmgr,uint8_t playerid,uint16_t mask,uint8_t value) {
  if (playerid>INMGR_PLAYER_LIMIT) return;
  if (value) {
    if (inmgr->player_statev[playerid]&mask) return;
    inmgr->player_statev[playerid]|=mask;
  } else {
    if (!(inmgr->player_statev[playerid]&mask)) return;
    inmgr->player_statev[playerid]&=~mask;
  }
  inmgr->delegate.state_change(inmgr->delegate.userdata,playerid,mask,value,inmgr->player_statev[playerid]);
  if (playerid) inmgr_change_player_state(inmgr,0,mask,value);
}

/* Forced state change with no propagation.
 */
 
void inmgr_force_input_state(struct inmgr *inmgr,uint8_t playerid,uint16_t mask,uint8_t value) {
  if (playerid>INMGR_PLAYER_LIMIT) return;
  if (value) inmgr->player_statev[playerid]|=mask;
  else inmgr->player_statev[playerid]&=~mask;
}

/* Search for multiple live maps.
 */
 
int inmgr_mapv_search_srcdevid(struct inmgr_map **dstpp,const struct inmgr *inmgr,int srcdevid) {
  int lo=0,hi=inmgr->mapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    struct inmgr_map *q=inmgr->mapv+ck;
         if (srcdevid<q->srcdevid) hi=ck;
    else if (srcdevid>q->srcdevid) lo=ck+1;
    else {
      int c=1;
      while ((ck>lo)&&(q[-1].srcdevid==srcdevid)) { c++; ck--; q--; }
      while ((ck+c<hi)&&(q[c].srcdevid==srcdevid)) c++;
      *dstpp=q;
      return c;
    }
  }
  *dstpp=inmgr->mapv+lo;
  return 0;
}

int inmgr_mapv_search_srcdevid_srcbtnid(struct inmgr_map **dstpp,const struct inmgr *inmgr,int srcdevid,int srcbtnid) {
  int lo=0,hi=inmgr->mapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    struct inmgr_map *q=inmgr->mapv+ck;
         if (srcdevid<q->srcdevid) hi=ck;
    else if (srcdevid>q->srcdevid) lo=ck+1;
    else if (srcbtnid<q->srcbtnid) hi=ck;
    else if (srcbtnid>q->srcbtnid) lo=ck+1;
    else {
      int c=1;
      while ((ck>lo)&&(q[-1].srcdevid==srcdevid)&&(q[-1].srcbtnid==srcbtnid)) { c++; ck--; q--; }
      while ((ck+c<hi)&&(q[c].srcdevid==srcdevid)&&(q[c].srcbtnid==srcbtnid)) c++;
      *dstpp=q;
      return c;
    }
  }
  *dstpp=inmgr->mapv+lo;
  return 0;
}

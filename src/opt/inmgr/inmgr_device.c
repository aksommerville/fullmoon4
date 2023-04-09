#include "inmgr_internal.h"
#include "app/fmn_platform.h" /* only necessary today due to static mapping */
#include "opt/bigpc/bigpc_internal.h" /* '' */

/* Delete.
 */
 
void inmgr_device_del(struct inmgr_device *device) {
  if (!device) return;
  if (device->mapv) free(device->mapv);
  free(device);
}

/* New.
 */
 
struct inmgr_device *inmgr_device_new(int devid,uint16_t vid,uint16_t pid,const char *name,int namec) {
  struct inmgr_device *device=calloc(1,sizeof(struct inmgr_device));
  if (!device) return 0;
  
  device->devid=devid;
  device->vid=vid;
  device->pid=pid;
  if (name) {
    if (namec<0) { namec=0; while (name[namec]) namec++; }
    while (namec&&((unsigned char)name[0]<=0x20)) { namec--; name++; }
    while (namec&&((unsigned char)name[namec-1]<=0x20)) namec--;
    if (namec>=INMGR_DEVICE_NAME_LIMIT) {
      namec=INMGR_DEVICE_NAME_LIMIT-1;
      while (namec&&((unsigned char)name[namec-1]<=0x20)) namec--;
    }
    memcpy(device->name,name,namec);
    device->namec=namec;
  }
  device->name[device->namec]=0;
  
  return device;
}

/* Add capability.
 */
 
int inmgr_device_add_capability(struct inmgr_device *device,int btnid,uint32_t usage,int lo,int hi,int value) {
  //TODO
  return 0;
}

/* Finalize maps.
 */
 
extern void inmgr_XXX_add_hardcoded_maps(struct inmgr_device *device);
 
int inmgr_device_finalize_maps(struct inmgr_device *device) {
  inmgr_XXX_add_hardcoded_maps(device);//TODO
  return 0;
}

/* Disconnect device (public).
 */
 
void inmgr_device_disconnect(struct inmgr *inmgr,int devid) {
  uint16_t playeridbits=inmgr_remove_maps(inmgr,devid);
  if (inmgr->pending_device) {
    if (inmgr->pending_device->devid==devid) {
      inmgr_device_del(inmgr->pending_device);
      inmgr->pending_device=0;
    }
  }
  if (playeridbits) {
    uint16_t buttons_dropped=0;
    uint8_t playerid=1;
    uint16_t mask=2;
    for (;mask&&(mask<=playeridbits);mask<<=1,playerid++) {
      if (!(playeridbits&mask)) continue;
      if (!inmgr->player_statev[playerid]) continue;
      // Any impacted player, we drop its entire state (could be more than just the buttons mapped to this device).
      // That's incorrect but I think it's tolerable, and certainly it's less work this way.
      // If two users are controlling the same hero, and one of them unplugs, the other might hiccup and need to release and repress buttons.
      buttons_dropped|=inmgr->player_statev[playerid];
      inmgr_trigger_multiple_state_changes(inmgr,playerid,inmgr->player_statev[playerid],0);
    }
    inmgr_trigger_multiple_state_changes(inmgr,0,buttons_dropped,0);
  }
}

/* Connect device (public).
 */
 
int inmgr_device_connect(struct inmgr *inmgr,int devid,uint16_t vid,uint16_t pid,const char *name,int namec) {
  if (inmgr->pending_device) {
    inmgr_device_del(inmgr->pending_device);
    inmgr->pending_device=0;
  }
  struct inmgr_device *device=inmgr_device_new(devid,vid,pid,name,namec);
  if (!device) return -1;
  inmgr->pending_device=device;
  return 0;
}

/* Add capability (public).
 */
 
void inmgr_device_capability(struct inmgr *inmgr,int devid,int btnid,uint32_t usage,int lo,int hi,int value) {
  struct inmgr_device *device=inmgr->pending_device;
  if (!device) return;
  if (device->devid!=devid) return;
  inmgr_device_add_capability(device,btnid,usage,lo,hi,value);
}

/* Finalize config (public).
 */
 
int inmgr_device_ready(struct inmgr *inmgr,int devid) {
  struct inmgr_device *device=inmgr->pending_device;
  if (!device) return 0;
  if (device->devid!=devid) return 0;
  
  if (inmgr_device_finalize_maps(device)<0) return -1;
  
  if (inmgr_maps_acceptable(inmgr,device->mapv,device->mapc)) {
    if (inmgr_add_maps(inmgr,device->mapv,device->mapc)<0) return -1;
    fprintf(stderr,
      "inmgr: Mapped device %d %04x:%04x '%.*s'. Ready to use.\n",
      device->devid,device->vid,device->pid,device->namec,device->name
    );
  } else {
    fprintf(stderr,
      "inmgr: Device %d %04x:%04x '%.*s', mapping inadequate. Will ignore.\n",
      device->devid,device->vid,device->pid,device->namec,device->name
    );
  }
  
  inmgr_device_del(inmgr->pending_device);
  inmgr->pending_device=0;
  return 0;
}

/* Configure device in one pass, for system keyboard.
 */
 
static struct inmgr_map inmgr_keyboard_map[]={
  {.srcbtnid=0x0007001b,.dstbtnid=FMN_INPUT_MENU}, // X
  {.srcbtnid=0x0007001d,.dstbtnid=FMN_INPUT_USE}, // Z
  {.srcbtnid=0x00070029,.dstbtnid=BIGPC_ACTIONID_quit,.mode=INMGR_MAP_MODE_ACTION}, // Esc
  {.srcbtnid=0x00070045,.dstbtnid=BIGPC_ACTIONID_fullscreen,.mode=INMGR_MAP_MODE_ACTION}, // F12
  {.srcbtnid=0x0007004f,.dstbtnid=FMN_INPUT_RIGHT},
  {.srcbtnid=0x00070050,.dstbtnid=FMN_INPUT_LEFT},
  {.srcbtnid=0x00070051,.dstbtnid=FMN_INPUT_DOWN},
  {.srcbtnid=0x00070052,.dstbtnid=FMN_INPUT_UP},
};
 
int inmgr_connect_system_keyboard(struct inmgr *inmgr,int devid) {
  // Since we're hard-coding maps across the board at this time, skip most of the generic process.
  //TODO Return here after we've generalized templates and capabilities.
  struct inmgr_map *p=inmgr_keyboard_map;
  int mapc=sizeof(inmgr_keyboard_map)/sizeof(struct inmgr_map);
  int i=mapc;
  for (;i-->0;p++) {
    p->srcdevid=devid;
    p->dstplayerid=1;
    p->srclo=1;
    p->srchi=INT_MAX;
    if (!p->mode) p->mode=INMGR_MAP_MODE_TWOSTATE;
  }
  if (inmgr_add_maps(inmgr,inmgr_keyboard_map,mapc)<0) return -1;
  fprintf(stderr,"inmgr: Mapped system keyboard.\n");
  return 0;
}

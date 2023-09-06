#include "inmgr_internal.h"

/* Delete.
 */

void inmgr_rules_del(struct inmgr_rules *rules) {
  if (!rules) return;
  if (rules->name) free(rules->name);
  if (rules->buttonv) free(rules->buttonv);
  free(rules);
}

/* New.
 */
 
struct inmgr_rules *inmgr_rules_new() {
  struct inmgr_rules *rules=calloc(1,sizeof(struct inmgr_rules));
  if (!rules) return 0;
  return rules;
}

/* Set name.
 */

int inmgr_rules_set_name(struct inmgr_rules *rules,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (rules->name) free(rules->name);
  rules->name=nv;
  rules->namec=srcc;
  return 0;
}

/* Add button.
 */
 
struct inmgr_rules_button *inmgr_rules_add_button(struct inmgr_rules *rules,int srcbtnid,uint8_t srcpart) {
  int p=inmgr_rules_buttonv_search(rules,srcbtnid,srcpart);
  if (p>=0) return 0;
  return inmgr_rules_buttonv_insert(rules,-p-1,srcbtnid,srcpart);
}

/* Search buttons.
 */

int inmgr_rules_buttonv_search(const struct inmgr_rules *rules,int srcbtnid,uint8_t srcpart) {
  int lo=0,hi=rules->buttonc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct inmgr_rules_button *button=rules->buttonv+ck;
         if (srcbtnid<button->srcbtnid) hi=ck;
    else if (srcbtnid>button->srcbtnid) lo=ck+1;
    else if (srcpart<button->srcpart) hi=ck;
    else if (srcpart>button->srcpart) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Insert button.
 */
 
struct inmgr_rules_button *inmgr_rules_buttonv_insert(struct inmgr_rules *rules,int p,int srcbtnid,uint8_t srcpart) {
  if ((p<0)||(p>rules->buttonc)) return 0;
  if (rules->buttonc>=rules->buttona) {
    int na=rules->buttona+16;
    if (na>INT_MAX/sizeof(struct inmgr_rules_button)) return 0;
    void *nv=realloc(rules->buttonv,sizeof(struct inmgr_rules_button)*na);
    if (!nv) return 0;
    rules->buttonv=nv;
    rules->buttona=na;
  }
  struct inmgr_rules_button *button=rules->buttonv+p;
  memmove(button+1,button,sizeof(struct inmgr_rules_button)*(rules->buttonc-p));
  rules->buttonc++;
  memset(button,0,sizeof(struct inmgr_rules_button));
  button->srcbtnid=srcbtnid;
  button->srcpart=srcpart;
  return button;
}

/* Add button to rules from a live map.
 */
 
int inmgr_rules_add_from_map(struct inmgr_rules *rules,const struct inmgr_map *map,const struct inmgr_device *device) {

  /* The hard part is reconstructing (srcpart) from the map's range.
   * But not all that hard...
   */
  uint8_t srcpart=0;
  if (map->srclo>map->srchi) { // Special case that only means HAT_N
    srcpart=INMGR_SRCPART_HAT_N;
  } else {
    const struct inmgr_cap *cap=inmgr_device_get_capability(device,map->srcbtnid);
    if (!cap) {
      // No cap, so let's assume it's a simple button. This would be the case for keyboards, eg.
      srcpart=INMGR_SRCPART_BUTTON_ON;
    } else if ((cap->lo==0)&&((cap->hi==1)||(cap->hi==2)||((map->srclo<cap->hi>>1)&&(map->srclo!=INT_MIN)&&(cap->lo+7!=cap->hi)))) {
      // Typical button.
      srcpart=INMGR_SRCPART_BUTTON_ON;
    } else if (cap->lo==cap->hi-7) {
      // Hat.
      if (map->srclo==cap->lo+1) srcpart=INMGR_SRCPART_HAT_E;
      else if (map->srclo==cap->lo+3) srcpart=INMGR_SRCPART_HAT_S;
      else if (map->srclo==cap->lo+5) srcpart=INMGR_SRCPART_HAT_W;
    } else if (map->srclo==INT_MIN) {
      // Low end of axis.
      srcpart=INMGR_SRCPART_AXIS_LOW;
    } else if (map->srchi==INT_MAX) {
      // High end of axis.
      srcpart=INMGR_SRCPART_AXIS_HIGH;
    } else {
      fprintf(stderr,
        "Unable to determine srcpart for button 0x%08x on device '%.*s', range=%d..%d, map range %d..%d\n",
        map->srcbtnid,device->namec,device->name,cap->lo,cap->hi,map->srclo,map->srchi
      );
      return -1;
    }
  }

  int p=inmgr_rules_buttonv_search(rules,map->srcbtnid,srcpart);
  if (p>=0) return -1;
  p=-p-1;
  struct inmgr_rules_button *button=inmgr_rules_buttonv_insert(rules,p,map->srcbtnid,srcpart);
  if (!button) return -1;
  button->dsttype=map->dsttype;
  button->dstbtnid=map->dstbtnid;
  return 0;
}

#include "bigpc_synth.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

/* Driver registry.
 ************************************************/
 
extern const struct bigpc_synth_type bigpc_synth_type_stdsyn;
extern const struct bigpc_synth_type bigpc_synth_type_minsyn;
extern const struct bigpc_synth_type bigpc_synth_type_dummy;

static const struct bigpc_synth_type *bigpc_synth_typev[]={
#if FMN_USE_stdsyn
  &bigpc_synth_type_stdsyn,
#endif
#if FMN_USE_minsyn
  &bigpc_synth_type_minsyn,
#endif
  &bigpc_synth_type_dummy,
};

const struct bigpc_synth_type *bigpc_synth_type_by_index(int p) {
  if (p<0) return 0;
  int c=sizeof(bigpc_synth_typev)/sizeof(void*);
  if (p>=c) return 0;
  return bigpc_synth_typev[p];
}

const struct bigpc_synth_type *bigpc_synth_type_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  if (!namec) return 0;
  const struct bigpc_synth_type **p=bigpc_synth_typev;
  int i=sizeof(bigpc_synth_typev)/sizeof(void*);
  for (;i-->0;p++) {
    if (!(*p)->name) continue;
    if (memcmp((*p)->name,name,namec)) continue;
    if ((*p)->name[namec]) continue;
    return *p;
  }
  return 0;
}

/* Wrapper.
 ******************************************************/

void bigpc_synth_del(struct bigpc_synth_driver *driver) {
  if (!driver) return;
  if (driver->refc-->1) return;
  if (driver->type->del) driver->type->del(driver);
  free(driver);
}

struct bigpc_synth_driver *bigpc_synth_new(
  const struct bigpc_synth_type *type,
  const struct bigpc_synth_config *config
) {
  if (!type||!config) return 0;
  if (!type->init) return 0;
  struct bigpc_synth_driver *driver=calloc(1,type->objlen);
  if (!driver) return 0;
  driver->type=type;
  driver->refc=1;
  driver->rate=config->rate;
  driver->chanc=config->chanc;
  driver->format=config->format;
  if (type->init(driver)<0) {
    bigpc_synth_del(driver);
    return 0;
  }
  if (!driver->update) {
    bigpc_synth_del(driver);
    return 0;
  }
  return driver;
}

int bigpc_synth_set_instrument(struct bigpc_synth_driver *driver,int id,const void *src,int srcc) {
  if (!driver||!driver->type->set_instrument) return 0;
  return driver->type->set_instrument(driver,id,src,srcc);
}

int bigpc_synth_set_sound(struct bigpc_synth_driver *driver,int id,const void *src,int srcc) {
  if (!driver||!driver->type->set_sound) return 0;
  return driver->type->set_sound(driver,id,src,srcc);
}

void bigpc_synth_event(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  if (!driver||!driver->type->event) return;
  driver->type->event(driver,chid,opcode,a,b);
}

void bigpc_synth_release_all(struct bigpc_synth_driver *driver) {
  if (!driver||!driver->type->event) return;
  driver->type->event(driver,0xff,0xff,0,0);
}

void bigpc_synth_silence_all(struct bigpc_synth_driver *driver) {
  if (!driver||!driver->type->event) return;
  driver->type->event(driver,0xff,0xff,0,0);
}

int bigpc_synth_play_song(struct bigpc_synth_driver *driver,const void *src,int srcc,int force) {
  if (!driver) return -1;
  if (!driver->type->play_song) return -1;
  return driver->type->play_song(driver,src,srcc,force);
}

void bigpc_synth_pause_song(struct bigpc_synth_driver *driver,int pause) {
  if (!driver||!driver->type->pause_song) return;
  driver->type->pause_song(driver,pause);
}

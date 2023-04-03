#include "bigpc_audio.h"
#include <string.h>
#include <stdlib.h>

/* Type registry.
 */
 
extern const struct bigpc_audio_type bigpc_audio_type_alsa;
extern const struct bigpc_audio_type bigpc_audio_type_pulse;
extern const struct bigpc_audio_type bigpc_audio_type_macaudio;
extern const struct bigpc_audio_type bigpc_audio_type_msaudio;
extern const struct bigpc_audio_type bigpc_audio_type_dummy;

static const struct bigpc_audio_type *bigpc_audio_typev[]={
#if FMN_USE_alsa
  &bigpc_audio_type_alsa,
#endif
#if FMN_USE_pulse
  &bigpc_audio_type_pulse,
#endif
#if FMN_USE_macaudio
  &bigpc_audio_type_macaudio,
#endif
#if FMN_USE_msaudio
  &bigpc_audio_type_msaudio,
#endif
  &bigpc_audio_type_dummy,
};

const struct bigpc_audio_type *bigpc_audio_type_by_index(int p) {
  if (p<0) return 0;
  int c=sizeof(bigpc_audio_typev)/sizeof(void*);
  if (p>=c) return 0;
  return bigpc_audio_typev[p];
}

const struct bigpc_audio_type *bigpc_audio_type_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  if (!namec) return 0;
  const struct bigpc_audio_type **p=bigpc_audio_typev;
  int i=sizeof(bigpc_audio_typev)/sizeof(void*);
  for (;i-->0;p++) {
    if (!(*p)->name) continue;
    if (memcmp((*p)->name,name,namec)) continue;
    if ((*p)->name[namec]) continue;
    return *p;
  }
  return 0;
}

/* Instance wrapper.
 */

void bigpc_audio_driver_del(struct bigpc_audio_driver *driver) {
  if (!driver) return;
  if (driver->refc-->1) return;
  if (driver->type->del) driver->type->del(driver);
  free(driver);
}

/* New.
 */

struct bigpc_audio_driver *bigpc_audio_driver_new(
  const struct bigpc_audio_type *type,
  const struct bigpc_audio_delegate *delegate,
  const struct bigpc_audio_config *config
) {
  if (!type) return 0;
  struct bigpc_audio_driver *driver=calloc(1,type->objlen);
  if (!driver) return 0;
  
  driver->type=type;
  driver->refc=1;
  if (delegate) driver->delegate=*delegate;
  
  if (type->init) {
    if (type->init(driver,config)<0) {
      bigpc_audio_driver_del(driver);
      return 0;
    }
  }
  
  return driver;
}

void bigpc_audio_play(struct bigpc_audio_driver *driver,int play) {
  if (!driver) return;
  if (!driver->type->play) return;
  if (play) {
    if (driver->playing) return;
    driver->type->play(driver,1);
  } else {
    if (!driver->playing) return;
    driver->type->play(driver,0);
  }
}

int bigpc_audio_update(struct bigpc_audio_driver *driver) {
  if (!driver) return 0;
  if (!driver->type->update) return 0;
  return driver->type->update(driver);
}

int bigpc_audio_lock(struct bigpc_audio_driver *driver) {
  if (!driver) return 0;
  if (!driver->type->lock) return 0;
  return driver->type->lock(driver);
}

void bigpc_audio_unlock(struct bigpc_audio_driver *driver) {
  if (!driver) return;
  if (!driver->type->unlock) return;
  driver->type->unlock(driver);
}

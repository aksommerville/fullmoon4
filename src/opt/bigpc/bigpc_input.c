#include "bigpc_input.h"
#include <string.h>
#include <stdlib.h>

/* Type and device ID registry.
 */
 
extern const struct bigpc_input_type bigpc_input_type_evdev;
extern const struct bigpc_input_type bigpc_input_type_machid;
extern const struct bigpc_input_type bigpc_input_type_mshid;

static const struct bigpc_input_type *bigpc_input_typev[]={
#if FMN_USE_evdev
  &bigpc_input_type_evdev,
#endif
#if FMN_USE_machid
  &bigpc_input_type_machid,
#endif
#if FMN_USE_mshid
  &bigpc_input_type_mshid,
#endif
};

const struct bigpc_input_type *bigpc_input_type_by_index(int p) {
  if (p<0) return 0;
  int c=sizeof(bigpc_input_typev)/sizeof(void*);
  if (p>=c) return 0;
  return bigpc_input_typev[p];
}

const struct bigpc_input_type *bigpc_input_type_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  if (!namec) return 0;
  const struct bigpc_input_type **p=bigpc_input_typev;
  int i=sizeof(bigpc_input_typev)/sizeof(void*);
  for (;i-->0;p++) {
    if (!(*p)->name) continue;
    if (memcmp((*p)->name,name,namec)) continue;
    if ((*p)->name[namec]) continue;
    return *p;
  }
  return 0;
}

static int _bigpc_input_devid_next=1;

int bigpc_input_devid_next() {
  int devid=_bigpc_input_devid_next++;
  if (_bigpc_input_devid_next<1) _bigpc_input_devid_next=1;
  return devid;
}

void bigpc_input_driver_del(struct bigpc_input_driver *driver) {
  if (!driver) return;
  if (driver->refc-->1) return;
  if (driver->type->del) driver->type->del(driver);
  free(driver);
}

struct bigpc_input_driver *bigpc_input_driver_new(
  const struct bigpc_input_type *type,
  const struct bigpc_input_delegate *delegate
) {
  if (!type) return 0;
  struct bigpc_input_driver *driver=calloc(1,type->objlen);
  if (!driver) return 0;
  
  driver->type=type;
  driver->refc=1;
  if (delegate) driver->delegate=*delegate;
  
  if (type->init) {
    if (type->init(driver)<0) {
      bigpc_input_driver_del(driver);
      return 0;
    }
  }
  
  return driver;
}

int bigpc_input_driver_update(struct bigpc_input_driver *driver) {
  if (!driver) return 0;
  if (!driver->type->update) return 0;
  return driver->type->update(driver);
}

const char *bigpc_input_device_get_ids(
  uint16_t *vid,uint16_t *pid,
  struct bigpc_input_driver *driver,
  int devid
) {
  if (!driver) return 0;
  if (!driver->type->get_ids) return 0;
  return driver->type->get_ids(vid,pid,driver,devid);
}

int bigpc_input_device_for_each_button(
  struct bigpc_input_driver *driver,
  int devid,
  int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
) {
  if (!driver) return -1;
  if (!cb) return -1;
  if (!driver->type->for_each_button) return -1;
  return driver->type->for_each_button(driver,devid,cb,userdata);
}

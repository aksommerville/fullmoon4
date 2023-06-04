#include "bigpc_render.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

/* Types.
 **************************************************************/
 
extern const struct bigpc_render_type bigpc_render_type_gl2;
extern const struct bigpc_render_type bigpc_render_type_metal;
extern const struct bigpc_render_type bigpc_render_type_soft;
extern const struct bigpc_render_type bigpc_render_type_dummy;

static const struct bigpc_render_type *bigpc_render_typev[]={
#if FMN_USE_gl2
  &bigpc_render_type_gl2,
#endif
#if FMN_USE_metal
  &bigpc_render_type_metal,
#endif
#if FMN_USE_soft
  &bigpc_render_type_soft,
#endif
  &bigpc_render_type_dummy,
};

const struct bigpc_render_type *bigpc_render_type_by_index(int p) {
  if (p<0) return 0;
  int c=sizeof(bigpc_render_typev)/sizeof(void*);
  if (p>=c) return 0;
  return bigpc_render_typev[p];
}

const struct bigpc_render_type *bigpc_render_type_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  if (!namec) return 0;
  const struct bigpc_render_type **p=bigpc_render_typev;
  int i=sizeof(bigpc_render_typev)/sizeof(void*);
  for (;i-->0;p++) {
    if (!(*p)->name) continue;
    if (memcmp((*p)->name,name,namec)) continue;
    if ((*p)->name[namec]) continue;
    return *p;
  }
  return 0;
}

/* Wrapper.
 ***********************************************************/

void bigpc_render_del(struct bigpc_render_driver *driver) {
  if (!driver) return;
  if (driver->refc-->1) return;
  if (driver->type->del) driver->type->del(driver);
  free(driver);
}

int bigpc_render_ref(struct bigpc_render_driver *driver) {
  if (!driver) return -1;
  if (driver->refc<1) return -1;
  if (driver->refc==INT_MAX) return -1;
  driver->refc++;
  return 0;
}

struct bigpc_render_driver *bigpc_render_new(
  const struct bigpc_render_type *type,
  struct bigpc_video_driver *video
) {
  if (!type) return 0;
  struct bigpc_render_driver *driver=calloc(1,type->objlen);
  if (!driver) return 0;
  driver->type=type;
  driver->refc=1;
  if (type->init) {
    if (type->init(driver,video)<0) {
      bigpc_render_del(driver);
      return 0;
    }
  }
  return driver;
}

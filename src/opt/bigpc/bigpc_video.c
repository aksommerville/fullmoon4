#include "bigpc_video.h"
#include <string.h>
#include <stdlib.h>

/* Global type registry.
 */
 
extern const struct bigpc_video_type bigpc_video_type_glx;
extern const struct bigpc_video_type bigpc_video_type_drm;
extern const struct bigpc_video_type bigpc_video_type_bcm;
extern const struct bigpc_video_type bigpc_video_type_macwm;
extern const struct bigpc_video_type bigpc_video_type_mswm;
extern const struct bigpc_video_type bigpc_video_type_dummy;
 
static const struct bigpc_video_type *bigpc_video_typev[]={
#if FMN_USE_glx
  &bigpc_video_type_glx,
#endif
#if FMN_USE_drm
  &bigpc_video_type_drm,
#endif
#if FMN_USE_bcm
  &bigpc_video_type_bcm,
#endif
#if FMN_USE_macwm
  &bigpc_video_type_macwm,
#endif
#if FMN_USE_mswm
  &bigpc_video_type_mswm,
#endif
  &bigpc_video_type_dummy,
};

const struct bigpc_video_type *bigpc_video_type_by_index(int p) {
  if (p<0) return 0;
  int c=sizeof(bigpc_video_typev)/sizeof(void*);
  if (p>=c) return 0;
  return bigpc_video_typev[p];
}

const struct bigpc_video_type *bigpc_video_type_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  if (!namec) return 0;
  const struct bigpc_video_type **p=bigpc_video_typev;
  int i=sizeof(bigpc_video_typev)/sizeof(void*);
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
 
void bigpc_video_driver_del(struct bigpc_video_driver *driver) {
  if (!driver) return;
  if (driver->refc-->1) return;
  if (driver->type->del) driver->type->del(driver);
  free(driver);
}

struct bigpc_video_driver *bigpc_video_driver_new(
  const struct bigpc_video_type *type,
  const struct bigpc_video_delegate *delegate,
  const struct bigpc_video_config *config
) {
  if (!type||!config) return 0;
  struct bigpc_video_driver *driver=calloc(1,type->objlen);
  if (!driver) return 0;
  
  driver->type=type;
  driver->refc=1;
  if (delegate) driver->delegate=*delegate;
  driver->w=config->w;
  driver->h=config->h;
  driver->fbw=config->fbw;
  driver->fbh=config->fbh;
  
  if (type->init) {
    if (type->init(driver,config)<0) {
      bigpc_video_driver_del(driver);
      return 0;
    }
  }
  
  return driver;
}

int bigpc_video_driver_update(struct bigpc_video_driver *driver) {
  if (!driver) return 0;
  if (!driver->type->update) return 0;
  return driver->type->update(driver);
}

struct bigpc_image *bigpc_video_driver_begin_soft(struct bigpc_video_driver *driver) {
  if (!driver) return 0;
  if (!driver->type->begin_soft) return 0;
  return driver->type->begin_soft(driver);
}

int bigpc_video_driver_begin_gx(struct bigpc_video_driver *driver) {
  if (!driver) return -1;
  if (!driver->type->begin_gx) return -1;
  return driver->type->begin_gx(driver);
}

void bigpc_video_driver_end(struct bigpc_video_driver *driver) {
  if (!driver) return;
  if (!driver->type->end) return;
  driver->type->end(driver);
}

void bigpc_video_driver_cancel(struct bigpc_video_driver *driver) {
  if (!driver) return;
  if (!driver->type->cancel) return;
  driver->type->cancel(driver);
}

void bigpc_video_show_cursor(struct bigpc_video_driver *driver,int show) {
  if (!driver) return;
  if (!driver->type->show_cursor) return;
  driver->type->show_cursor(driver,show);
}

void bigpc_video_set_fullscreen(struct bigpc_video_driver *driver,int fullscreen) {
  if (!driver) return;
  if (!driver->type->set_fullscreen) return;
  if (fullscreen) {
    if (driver->fullscreen) return;
    driver->type->set_fullscreen(driver,1);
  } else {
    if (!driver->fullscreen) return;
    driver->type->set_fullscreen(driver,0);
  }
}

void bigpc_video_suppress_screensaver(struct bigpc_video_driver *driver) {
  if (!driver) return;
  if (!driver->type->suppress_screensaver) return;
  driver->type->suppress_screensaver(driver);
}

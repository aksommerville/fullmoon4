/* evdev_glue.c
 * Connects the pre-written 'evdev' unit to Full Moon's 'bigpc' unit.
 */
 
#include "evdev.h"
#include "opt/bigpc/bigpc_input.h"

#define FMN_EVDEV_UPDATE_TIMEOUT_MS 2

struct bigpc_input_driver_evdev {
  struct bigpc_input_driver hdr;
  struct evdev *evdev;
  char name_storage[256];
};

#define DRIVER ((struct bigpc_input_driver_evdev*)driver)

static void _evdev_del(struct bigpc_input_driver *driver) {
  evdev_del(DRIVER->evdev);
}

static void _evdev_cb_connect(struct evdev_device *device) {
  struct bigpc_input_driver *driver=evdev_device_get_userdata(device);
  int devid=bigpc_input_devid_next();
  evdev_device_set_devid(device,devid);
  if (driver->delegate.cb_connect) driver->delegate.cb_connect(driver,devid);
}

static void _evdev_cb_disconnect(struct evdev_device *device) {
  struct bigpc_input_driver *driver=evdev_device_get_userdata(device);
  int devid=evdev_device_get_devid(device);
  if (driver->delegate.cb_disconnect) driver->delegate.cb_disconnect(driver,devid);
}

static void _evdev_cb_button(struct evdev_device *device,int type,int code,int value) {
  struct bigpc_input_driver *driver=evdev_device_get_userdata(device);
  int devid=evdev_device_get_devid(device);
  int btnid=(type<<16)|code;
  if (driver->delegate.cb_event) driver->delegate.cb_event(driver,devid,btnid,value);
}

static int _evdev_init(struct bigpc_input_driver *driver) {
  struct evdev_delegate delegate={
    .userdata=driver,
    .connect=_evdev_cb_connect,
    .disconnect=_evdev_cb_disconnect,
    .button=_evdev_cb_button,
  };
  struct evdev_setup setup={0};
  evdev_setup_default(&setup);
  if (!(DRIVER->evdev=evdev_new(&delegate,&setup))) return -1;
  return 0;
}

static int _evdev_update(struct bigpc_input_driver *driver) {
  return evdev_update(DRIVER->evdev,FMN_EVDEV_UPDATE_TIMEOUT_MS);
}

static const char *_evdev_get_ids(uint16_t *vid,uint16_t *pid,struct bigpc_input_driver *driver,int devid) {
  struct evdev_device *device=evdev_device_by_devid(DRIVER->evdev,devid);
  if (!device) return 0;
  if (vid) *vid=evdev_device_get_vid(device);
  if (pid) *pid=evdev_device_get_pid(device);
  int namec=evdev_device_get_name(DRIVER->name_storage,sizeof(DRIVER->name_storage),device);
  if ((namec<1)||(namec>=sizeof(DRIVER->name_storage))) DRIVER->name_storage[0]=0;
  else DRIVER->name_storage[namec]=0;
  return DRIVER->name_storage;
}

struct evdev_for_each_button_context {
  int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata);
  void *userdata;
};

static int _evdev_for_each_button_cb(int type,int code,int usage,int lo,int hi,int value,void *userdata) {
  struct evdev_for_each_button_context *ctx=userdata;
  return ctx->cb((type<<16)|code,usage,lo,hi,value,ctx->userdata);
}

static int _evdev_for_each_button(
  struct bigpc_input_driver *driver,
  int devid,
  int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
) {
  struct evdev_device *device=evdev_device_by_devid(DRIVER->evdev,devid);
  if (!device) return 0;
  struct evdev_for_each_button_context ctx={
    .cb=cb,
    .userdata=userdata,
  };
  return evdev_device_enumerate(device,_evdev_for_each_button_cb,&ctx);
}

const struct bigpc_input_type bigpc_input_type_evdev={
  .name="evdev",
  .desc="Linux input via evdev.",
  .objlen=sizeof(struct bigpc_input_driver_evdev),
  .appointment_only=0,
  .del=_evdev_del,
  .init=_evdev_init,
  .update=_evdev_update,
  .get_ids=_evdev_get_ids,
  .for_each_button=_evdev_for_each_button,
};

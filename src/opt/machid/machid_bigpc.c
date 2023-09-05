#include "machid_internal.h"
#include "opt/bigpc/bigpc_input.h"
#include <stdio.h>

/* Instance definition.
 */

struct bigpc_input_driver_machid {
  struct bigpc_input_driver hdr;
  struct machid *machid;
};

#define DRIVER ((struct bigpc_input_driver_machid*)driver)

/* Cleanup.
 */

static void _machid_del(struct bigpc_input_driver *driver) {
  machid_del(DRIVER->machid);
}

/* Callback glue.
 */

static int _machid_cb_devid_next(struct machid *machid) {
  return bigpc_input_devid_next();
}

static void _machid_cb_connect(struct machid *machid,int devid) {
  struct bigpc_input_driver *driver=machid_get_userdata(machid);
  if (driver->delegate.cb_connect) driver->delegate.cb_connect(driver,devid);
}

static void _machid_cb_disconnect(struct machid *machid,int devid) {
  struct bigpc_input_driver *driver=machid_get_userdata(machid);
  if (driver->delegate.cb_disconnect) driver->delegate.cb_disconnect(driver,devid);
}

static void _machid_cb_button(struct machid *machid,int devid,int btnid,int value) {
  struct bigpc_input_driver *driver=machid_get_userdata(machid);
  if (driver->delegate.cb_event) driver->delegate.cb_event(driver,devid,btnid,value);
}

/* Init.
 */

static int _machid_init(struct bigpc_input_driver *driver) {
  struct machid_delegate delegate={
    .userdata=driver,
    .devid_next=_machid_cb_devid_next,
    .connect=_machid_cb_connect,
    .disconnect=_machid_cb_disconnect,
    .button=_machid_cb_button,
  };
  if (!(DRIVER->machid=machid_new(&delegate))) return -1;
  return 0;
}

/* Update.
 */

static int _machid_update(struct bigpc_input_driver *driver) {
  return machid_update(DRIVER->machid,0.0f);
}

/* Device properties.
 */

static const char *_machid_get_ids(uint16_t *vid,uint16_t *pid,struct bigpc_input_driver *driver,int devid) {
  int ivid,ipid;
  const char *name=machid_get_ids(&ivid,&ipid,DRIVER->machid,devid);
  *vid=ivid;
  *pid=ipid;
  return name;
}

static int _machid_for_each_button(
  struct bigpc_input_driver *driver,
  int devid,
  int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
) {
  // lucky! machid has exactly the same shape as bigpc for this.
  // (well, not all that big a coincidence, they're written by the same guy about six months apart...)
  return machid_enumerate(DRIVER->machid,devid,cb,userdata);
}

static int _machid_for_each_device(
  struct bigpc_input_driver *driver,
  int (*cb)(struct bigpc_input_driver *driver,int devid,void *userdata),
  void *userdata
) {
  int i=0,err;
  for (;i<DRIVER->machid->devc;i++) {
    if (err=cb(driver,DRIVER->machid->devv[i].devid,userdata)) return err;
  }
  return 0;
}

/* Type definition.
 */

const struct bigpc_input_type bigpc_input_type_machid={
  .name="machid",
  .desc="Joystick input for MacOS",
  .objlen=sizeof(struct bigpc_input_driver_machid),
  .del=_machid_del,
  .init=_machid_init,
  .update=_machid_update,
  .get_ids=_machid_get_ids,
  .for_each_button=_machid_for_each_button,
  .for_each_device=_machid_for_each_device,
};

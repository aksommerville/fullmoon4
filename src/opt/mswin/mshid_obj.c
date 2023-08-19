#include "mshid_internal.h"

struct bigpc_input_driver *mshid_global=0;

/* Delete.
 */
 
static void _mshid_del(struct bigpc_input_driver *driver) {
  if (driver==mshid_global) mshid_global=0;
  if (DRIVER->devicev) {
    while (DRIVER->devicec-->0) {
      mshid_device_del(DRIVER->devicev[DRIVER->devicec]);
    }
    free(DRIVER->devicev);
  }
}

/* Init.
 */

static int _mshid_init(struct bigpc_input_driver *driver) {
  if (mshid_global) return -1;
  mshid_global=driver;
  
  HWND window=mswm_get_window_handle();
  RAWINPUTDEVICE devv[]={
    {
      .usUsagePage=0x01, // desktop
      .usUsage=0x04, // joystick
      .dwFlags=RIDEV_DEVNOTIFY|RIDEV_INPUTSINK,
      .hwndTarget=window,
    },
    {
      .usUsagePage=0x01, // desktop
      .usUsage=0x05, // game pad
      .dwFlags=RIDEV_DEVNOTIFY|RIDEV_INPUTSINK,
      .hwndTarget=window,
    },
    {
      .usUsagePage=0x01, // desktop
      .usUsage=0x06, // keyboard
      .dwFlags=RIDEV_DEVNOTIFY|RIDEV_INPUTSINK,
      .hwndTarget=window,
    },
    {
      .usUsagePage=0x01, // desktop
      .usUsage=0x07, // keypad
      .dwFlags=RIDEV_DEVNOTIFY|RIDEV_INPUTSINK,
      .hwndTarget=window,
    },
    {
      .usUsagePage=0x01, // desktop
      .usUsage=0x08, // multi-axis controller, whatever that means
      .dwFlags=RIDEV_DEVNOTIFY|RIDEV_INPUTSINK,
      .hwndTarget=window,
    },
  };
  if (!RegisterRawInputDevices(devv,sizeof(devv)/sizeof(RAWINPUTDEVICE),sizeof(RAWINPUTDEVICE))) {
    fprintf(stderr,"Failed to register for raw input.\n");
    return -1;
  }
  
  DRIVER->poll_soon=1;
  
  return 0;
}

/* Update.
 */

static int _mshid_update(struct bigpc_input_driver *driver) {
  if (DRIVER->poll_soon) {
    DRIVER->poll_soon=0;
    mshid_poll_connections();
  }
  return 0;
}

void mshid_poll_connections_later() {
  struct bigpc_input_driver *driver=mshid_global;
  if (!driver) return;
  DRIVER->poll_soon=1;
}

/* Iterate devices.
 */

static int _mshid_for_each_device(
  struct bigpc_input_driver *driver,
  int (*cb)(struct bigpc_input_driver *driver,int devid,void *userdata),
  void *userdata
) {
  int i=0,err;
  for (;i<DRIVER->devicec;i++) {
    struct mshid_device *device=DRIVER->devicev[i];
    if (err=cb(driver,device->devid,userdata)) return err;
  }
  return 0;
}

/* Type definition.
 */

const struct bigpc_input_type bigpc_input_type_mshid={
  .name="mshid",
  .desc="Joystick input for Windows",
  .objlen=sizeof(struct bigpc_input_driver_mshid),
  .del=_mshid_del,
  .init=_mshid_init,
  .update=_mshid_update,
  .get_ids=_mshid_get_ids,
  .for_each_button=_mshid_for_each_button,
  .for_each_device=_mshid_for_each_device,
};

/* Device list.
 */
 
struct mshid_device *mshid_device_by_handle(struct bigpc_input_driver *driver,HANDLE h) {
  struct mshid_device **p=DRIVER->devicev;
  int i=DRIVER->devicec;
  for (;i-->0;p++) {
    if ((*p)->handle==h) return *p;
  }
  return 0;
}

struct mshid_device *mshid_device_by_devid(struct bigpc_input_driver *driver,int devid) {
  struct mshid_device **p=DRIVER->devicev;
  int i=DRIVER->devicec;
  for (;i-->0;p++) {
    if ((*p)->devid==devid) return *p;
  }
  return 0;
}

int mshid_add_device(struct bigpc_input_driver *driver,struct mshid_device *device) {
  if (DRIVER->devicec>=DRIVER->devicea) {
    int na=DRIVER->devicea+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(DRIVER->devicev,sizeof(void*)*na);
    if (!nv) return -1;
    DRIVER->devicev=nv;
    DRIVER->devicea=na;
  }
  DRIVER->devicev[DRIVER->devicec++]=device;
  return 0;
}

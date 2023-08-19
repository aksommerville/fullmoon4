#include "mshid_internal.h"

struct bigpc_input_driver *mshid_global=0;

/* Delete.
 */
 
static void _mshid_del(struct bigpc_input_driver *driver) {
  if (driver==mshid_global) mshid_global=0;
  fprintf(stderr,"%s %p\n",__func__,driver);
}

/* Init.
 */

static int _mshid_init(struct bigpc_input_driver *driver) {
  if (mshid_global) return -1;
  mshid_global=driver;
  fprintf(stderr,"%s %p\n",__func__,driver);
  
  HWND window=mswm_get_window_handle();
  RAWINPUTDEVICE devv[]={
    /*
    {
      .usUsagePage=0x01, // desktop
      .usUsage=0x04, // joystick
      .dwFlags=0,
    },
    /**/
    {
      .usUsagePage=0x01, // desktop
      .usUsage=0x05, // game pad
      .dwFlags=RIDEV_DEVNOTIFY|RIDEV_INPUTSINK,
      .hwndTarget=window,
    },
    /**
    {
      .usUsagePage=0x01, // desktop
      .usUsage=0x06, // keyboard
      .dwFlags=0,
    },
    {
      .usUsagePage=0x01, // desktop
      .usUsage=0x07, // keypad
      .dwFlags=0,
    },
    {
      .usUsagePage=0x01, // desktop
      .usUsage=0x08, // multi-axis controller, whatever that means
      .dwFlags=0,
    },
    /**/
  };
  if (!RegisterRawInputDevices(devv,sizeof(devv)/sizeof(RAWINPUTDEVICE),sizeof(RAWINPUTDEVICE))) {
    fprintf(stderr,"Failed to register for raw input.\n");
    return -1;
  }
  fprintf(stderr,"REGISTERED\n");
  
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

/* Get IDs.
 */

static const char *_mshid_get_ids(uint16_t *vid,uint16_t *pid,struct bigpc_input_driver *driver,int devid) {
  fprintf(stderr,"%s devid=%d\n",__func__,devid);
  //TODO
  return 0;
}

/* Iterate buttons on one device.
 */

static int _mshid_for_each_button(
  struct bigpc_input_driver *driver,
  int devid,
  int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
) {
  fprintf(stderr,"%s devid=%d\n",__func__,devid);
  //TODO
  return 0;
}

/* Iterate devices.
 */

static int _mshid_for_each_device(
  struct bigpc_input_driver *driver,
  int (*cb)(struct bigpc_input_driver *driver,int devid,void *userdata),
  void *userdata
) {
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

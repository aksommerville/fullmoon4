#include "mshid_internal.h"

/* HID event.
 */

static void mshid_event_hid(struct bigpc_input_driver *driver,RAWINPUT *raw) {
  HANDLE handle=raw->header.hDevice;
  struct mshid_device *device=mshid_device_by_handle(driver,handle);
  if (!device) return;
  mshid_device_receive_report(driver,device,&raw->data.hid.bRawData,raw->data.hid.dwSizeHid);
}

/* General event. Called from mswm's event dispatch.
 */
 
void mshid_event(int wparam,int lparam) {
  struct bigpc_input_driver *driver=mshid_global;
  if (!driver) return;
  //fprintf(stderr,"%s wparam=0x%08x lparam=0x%08x\n",__func__,wparam,lparam);

  uint8_t buf[1024];
  UINT bufa=sizeof(buf);
  int bufc=GetRawInputData((HRAWINPUT)lparam,RID_INPUT,buf,&bufa,sizeof(RAWINPUTHEADER));

  if ((bufc<0)||(bufc>sizeof(buf))) return;
  RAWINPUT *raw=(RAWINPUT*)buf;
  //fprintf(stderr,"RawInput: %d bytes, dwType=%d\n",bufc,raw->header.dwType);
  switch (raw->header.dwType) {
    case RIM_TYPEKEYBOARD: return;// ps_mshid_evt_keyboard(raw);
    case RIM_TYPEMOUSE: return;// ps_mshid_evt_mouse(raw);
    case RIM_TYPEHID: mshid_event_hid(driver,raw); break;
    //default: fprintf(stderr,"Raw input header type %d\n",raw->header.dwType);
  }
}

/* New device: Add a record for it.
 */

static void mshid_connect_device(struct bigpc_input_driver *driver,RAWINPUTDEVICELIST *src) {

  struct mshid_device *device=mshid_device_by_handle(driver,src->hDevice);
  if (device) {
    device->visited=1;
    return;
  }
        
  RID_DEVICE_INFO devinfo={0};
  devinfo.cbSize=sizeof(RID_DEVICE_INFO);
  UINT pcbSize=sizeof(RID_DEVICE_INFO);
  GetRawInputDeviceInfo(src->hDevice,RIDI_DEVICEINFO,&devinfo,&pcbSize); // RIDI_DEVICEINFO,RIDI_DEVICENAME,RIDI_PREPARSEDDATA

  if (!(device=mshid_device_new())) return;
  if (mshid_add_device(driver,device)<0) {
    mshid_device_del(device);
    return;
  }
  device->visited=1;
  device->handle=src->hDevice;
  device->vid=devinfo.hid.dwVendorId;
  device->pid=devinfo.hid.dwProductId;
  device->devid=bigpc_input_devid_next();

  char name[256]={0};
  UINT namec=sizeof(name);
  GetRawInputDeviceInfo(src->hDevice,RIDI_DEVICENAME,name,&namec);
  if (namec>sizeof(name)) namec=sizeof(name);
  HANDLE HIDHandle=CreateFile(name,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
  if (HIDHandle) {
    char buf[256]={0};
    int result=HidD_GetProductString(HIDHandle,buf,sizeof(buf));
    buf[255]=buf[254]=0;
    mshid_device_set_name_utf16lez(device,buf);
    CloseHandle(HIDHandle);
  }

  mshid_device_acquire_report_descriptors(device);
        
  if (driver->delegate.cb_connect) {
    driver->delegate.cb_connect(driver,device->devid);
  }
}

/* Check for dropped or connected devices.
 */
 
void mshid_poll_connections() {
  struct bigpc_input_driver *driver=mshid_global;
  if (!driver) return;

  int i=DRIVER->devicec;
  while (i-->0) {
    struct mshid_device *device=DRIVER->devicev[i];
    device->visited=0;
  }
  
  UINT devc=0;
  GetRawInputDeviceList(0,&devc,sizeof(RAWINPUTDEVICELIST));
  if (devc>0) {
    RAWINPUTDEVICELIST *devv=calloc(devc,sizeof(RAWINPUTDEVICELIST));
    if (!devv) return;
    int err=GetRawInputDeviceList(devv,&devc,sizeof(RAWINPUTDEVICELIST));
    if (err>0) {
      devc=err;
      int i=devc; while (i-->0) {
        if (devv[i].dwType!=RIM_TYPEHID) continue;
        mshid_connect_device(driver,devv+i);
      }
    }
    free(devv);
  }

  for (i=DRIVER->devicec;i-->0;) {
    struct mshid_device *device=DRIVER->devicev[i];
    if (device->visited) continue;
    DRIVER->devicec--;
    memmove(DRIVER->devicev+i,DRIVER->devicev+i+1,sizeof(void*)*(DRIVER->devicec-i));
    if (driver->delegate.cb_disconnect) {
      driver->delegate.cb_disconnect(driver,device->devid);
    }
    mshid_device_del(device);
  }
}

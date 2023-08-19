#include "mshid_internal.h"

/* General event.
 */
 
void mshid_event(int wparam,int lparam) {
  struct bigpc_input_driver *driver=mshid_global;
  if (!driver) return;
  fprintf(stderr,"%s wparam=0x%08x lparam=0x%08x\n",__func__,wparam,lparam);
}

/* Check for dropped devices.
 */
 
void mshid_poll_connections() {
  struct bigpc_input_driver *driver=mshid_global;
  if (!driver) return;
  fprintf(stderr,"%s\n",__func__);
  
  fprintf(stderr,"GetRawInputDeviceList...\n");
  UINT devc=0;
  GetRawInputDeviceList(0,&devc,sizeof(RAWINPUTDEVICELIST));
  if (devc>0) {
    RAWINPUTDEVICELIST *devv=calloc(devc,sizeof(RAWINPUTDEVICELIST));
    if (!devv) return;
    int err=GetRawInputDeviceList(devv,&devc,sizeof(RAWINPUTDEVICELIST));
    if (err>0) {
      devc=err;
      int i=devc; while (i-->0) {
        fprintf(stderr,"  dwType=%d hDevice=0x%08x\n",devv[i].dwType,devv[i].hDevice);
        if (devv[i].dwType!=RIM_TYPEHID) continue;
        
        RID_DEVICE_INFO devinfo={0};
        devinfo.cbSize=sizeof(RID_DEVICE_INFO);
        UINT pcbSize=sizeof(RID_DEVICE_INFO);
        GetRawInputDeviceInfo(devv[i].hDevice,RIDI_DEVICEINFO,&devinfo,&pcbSize); // RIDI_DEVICEINFO,RIDI_DEVICENAME,RIDI_PREPARSEDDATA
        fprintf(stderr,"  ...HID %04x:%04x (page=%04x usage=%04x)\n",
          devinfo.hid.dwVendorId,
          devinfo.hid.dwProductId,
          devinfo.hid.usUsagePage,
          devinfo.hid.usUsage
        );
        /**
        if (devv[i].dwType!=RIM_TYPEHID) continue;
        int devid=ps_mshid_devid_from_handle(devv[i].hDevice);
        int p=ps_input_provider_device_search(ps_mshid.provider,devid);
        if (p<0) continue;
        if (p>=256) continue; // This would be strange, since USB only supports I think 127 devices on a bus.
        devinuse[p>>3]|=1<<(p&7);
        /**/
      }
    }
    free(devv);
  }
}

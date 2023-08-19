#include "mshid_internal.h"

#define MSHID_HIDP_HEADER_SIZE 36
#define MSHID_HIDP_USAGE_SIZE 104

// Arbitrary:
#define MSHID_HIDP_REPORT_COUNT_SANITY_LIMIT 32

#define RD16(src,p) ((((uint8_t*)(src))[p])|(((uint8_t*)(src))[p+1]<<8))
#define RD32(src,p) (RD16(src,p)|(RD16(src,p+2)<<16))

/* Delete.
 */

void mshid_device_del(struct mshid_device *device) {
  if (!device) return;

  if (device->name) free(device->name);
  if (device->buttonv) free(device->buttonv);

  free(device);
}

/* New.
 */

struct mshid_device *mshid_device_new() {
  struct mshid_device *device=calloc(1,sizeof(struct mshid_device));
  if (!device) return 0;

  return device;
}

/* Set name.
 */

int mshid_device_set_name(struct mshid_device *device,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  while (srcc&&((unsigned char)src[srcc-1]<=0x20)) srcc--;
  while (srcc&&((unsigned char)src[0]<=0x20)) { srcc--; src++; }
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (device->name) free(device->name);
  device->name=nv;
  device->namec=srcc;
  return 0;
}
 
int mshid_device_set_name_utf16lez(struct mshid_device *device,const uint8_t *src) {
  if (!device) return -1;
  char tmp[256];
  int srcp=0,tmpc=0;
  while (1) {
    int ch=src[srcp]|(src[srcp+1]<<8);
    if (!ch) break;
    srcp+=2;
    if ((ch>=0xd000)&&(ch<0xd800)) {
      int lo=src[srcp]|(src[srcp+1]<<8);
      if ((lo>=0xd800)&&(lo<0xe000)) {
        srcp+=2;
        ch=0x10000+((ch&0x3ff)<<10)+(lo&0x3ff);
      }
    }
    if ((ch<0x20)||(ch>0x7e)) tmp[tmpc++]='?';
    else tmp[tmpc++]=ch;
    if (tmpc>=sizeof(tmp)) break;
  }
  return mshid_device_set_name(device,tmp,tmpc);
}

/* Get IDs.
 */

const char *_mshid_get_ids(uint16_t *vid,uint16_t *pid,struct bigpc_input_driver *driver,int devid) {
  struct mshid_device *device=mshid_device_by_devid(driver,devid);
  if (!device) return 0;
  *vid=device->vid;
  *pid=device->pid;
  return device->name;
}

/* Iterate buttons on one device.
 */

int _mshid_for_each_button(
  struct bigpc_input_driver *driver,
  int devid,
  int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
) {
  struct mshid_device *device=mshid_device_by_devid(driver,devid);
  if (!device) return 0;
  const struct mshid_button *button=device->buttonv;
  int i=device->buttonc,err;
  for (;i-->0;button++) {
    int lo=button->logmin;
    int hi;
    int rest=0;
    switch (button->size) {
      case 1: hi=lo+1; break;
      case 2: hi=lo+3; break;
      case 4: hi=lo+7; break;
      case 8: hi=lo+255; break;
      case 16: hi=lo+65535; if (!lo) rest=0x8000; break;
      default: continue;
    }
    if (err=cb(button->btnid,button->usage,lo,hi,rest,userdata)) return err;
  }
  return 0;
}

/* Add button from descriptor.
 */

static struct mshid_button *mshid_device_add_button(struct mshid_device *device) {
  if (device->buttonc>=device->buttona) {
    int na=device->buttona+16;
    if (na>INT_MAX/sizeof(struct mshid_button)) return 0;
    void *nv=realloc(device->buttonv,sizeof(struct mshid_button)*na);
    if (!nv) return 0;
    device->buttonv=nv;
    device->buttona=na;
  }
  struct mshid_button *button=device->buttonv+device->buttonc++;
  memset(button,0,sizeof(struct mshid_button));
  button->btnid=device->buttonc;
  return button;
}

/* HID descriptor, single usage.
 */

static void mshid_device_apply_usage(struct mshid_device *device,const uint8_t *src) {

  /* Decode known fields. */
  uint16_t usage_page=RD16(src,0x00);
  uint8_t shift=((uint8_t*)src)[3];
  uint16_t report_size=RD16(src,0x04);
  uint16_t report_count=RD16(src,0x06);
  uint16_t low_offset=RD16(src,0x08);
  uint16_t total_size=RD16(src,0x0a);
  uint16_t flags=RD16(src,0x0c);
  uint16_t high_offset=RD16(src,0x10);
  uint16_t usage_low=RD16(src,0x3c);
  uint16_t usage_high=RD16(src,0x3e);
  uint16_t index_low=RD16(src,0x48);
  uint16_t index_high=RD16(src,0x4a);
  int logical_min=RD32(src,0x50);
  int logical_max=RD32(src,0x54);
  int physical_min=RD32(src,0x58);
  int physical_max=RD32(src,0x5c);

  // Validate.
  if (shift>=8) return;
  switch (report_size) {
    case 1: case 2: case 4: case 8: case 16: break;
    default: return;
  }
  if (!report_size) return;
  if (!report_count) return;
  if (report_size*report_count!=total_size) return;
  if (report_count>MSHID_HIDP_REPORT_COUNT_SANITY_LIMIT) return;

  // Add a button for each count.
  int usage=usage_low;
  int i=0;
  for (;i<report_count;i++) {
    struct mshid_button *button=mshid_device_add_button(device);
    if (!button) return;

    button->usage=(usage_page<<16)|usage;
    button->value=0;
    button->offset=low_offset;
    button->shift=shift;
    button->size=report_size;
    button->logmin=logical_min;
    
    if (usage<usage_high) usage++;
    if (report_size==1) {
      if (shift==7) {
        shift=0;
        low_offset++;
      } else {
        shift++;
      }
    }
  }
}

/* HID descriptor, header.
 * Returns expected count of usage descriptors following.
 */

static int mshid_device_apply_header(struct mshid_device *device,const uint8_t *src) {
  int usage_descriptors_size=RD16(src,0x20);
  int usage_descriptor_count=usage_descriptors_size/MSHID_HIDP_USAGE_SIZE;
  return usage_descriptor_count;
}

/* Apply report descriptor (during setup).
 */

static int mshid_device_apply_preparsed(struct mshid_device *device,const uint8_t *src,int srcc) {

  device->buttonc=0;
  if (srcc<8) return 0;
  if (memcmp(src,"HidP KDR",8)) return 0;
  int srcp=8;
  if (srcp>srcc-MSHID_HIDP_HEADER_SIZE) return 0;
  int usage_descriptor_count=mshid_device_apply_header(device,src+srcp);
  srcp+=MSHID_HIDP_HEADER_SIZE;

  while ((srcp<=srcc-MSHID_HIDP_USAGE_SIZE)&&(usage_descriptor_count-->0)) {
    mshid_device_apply_usage(device,src+srcp);
    srcp+=MSHID_HIDP_USAGE_SIZE;
  }

  // Now we've got all the buttons, determine the expected report length.
  device->rptlen=1;
  const struct mshid_button *button=device->buttonv;
  int i=device->buttonc;
  for (;i-->0;button++) {
    int bitp=(button->offset<<3)+button->size;
    int bytep=(bitp+7)>>3;
    if (bytep>device->rptlen) device->rptlen=bytep;
  }

  return 0;
}

/* Acquire descriptors (during setup).
 */

int mshid_device_acquire_report_descriptors(struct mshid_device *device) {
  int a=1024;
  uint8_t *v=malloc(a);
  if (!v) return -1;
  while (1) {
    UINT len=a;
    int c=GetRawInputDeviceInfo(device->handle,RIDI_PREPARSEDDATA,v,&len);
    if (c<0) c=len;
    if (c<0) break;
    if (c>a) {
      if (a>INT_MAX>>1) break;
      a<<=1;
      void *nv=realloc(v,a);
      if (!nv) break;
      v=nv;
      continue;
    }
    mshid_device_apply_preparsed(device,v,c);
    break;
  }
  free(v);
  return 0;
}

/* Read one field from report.
 */

static int mshid_button_read(const struct mshid_button *button,const uint8_t *src) {
  if (button->size==16) {
    int v=src[button->offset]|(src[button->offset+1]<<8);
    if (!button->logmin) return v; // unsigned
    return (int16_t)v; // anything else assume signed
  }
  int v=(src[button->offset]>>button->shift)&((1<<button->size)-1);
  return v;
}

/* Receive report (ongoing).
 */
 
void mshid_device_receive_report(struct bigpc_input_driver *driver,struct mshid_device *device,const void *src,int srcc) {
  if (srcc<device->rptlen) return;
  struct mshid_button *button=device->buttonv;
  int i=device->buttonc;
  for (;i-->0;button++) {
    int v=mshid_button_read(button,src);
    if (v==button->value) continue;
    button->value=v;
    if (driver->delegate.cb_event) driver->delegate.cb_event(driver,device->devid,button->btnid,v);
  }
}

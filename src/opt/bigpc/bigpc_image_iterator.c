#include "bigpc_image.h"
#include "app/fmn_platform.h"

/* Iterator bits.
 */
 
#define P ((uint8_t*)p)
 
static void _bigpc_image_iter_write_dummy(void *p,int q,uint32_t src) {}

// Whole words, nice and easy:
static uint32_t _bigpc_image_iter_read_32(const void *p,int q) { return *(uint32_t*)p; }
static void _bigpc_image_iter_write_32(void *p,int q,uint32_t src) { *(uint32_t*)p=src; }
static uint32_t _bigpc_image_iter_read_16(const void *p,int q) { return *(uint16_t*)p; }
static void _bigpc_image_iter_write_16(void *p,int q,uint32_t src) { *(uint16_t*)p=src; }
static uint32_t _bigpc_image_iter_read_8(const void *p,int q) { return *(uint8_t*)p; }
static void _bigpc_image_iter_write_8(void *p,int q,uint32_t src) { *(uint8_t*)p=src; }

// Bytewise alignment, also pretty easy:
static uint32_t _bigpc_image_iter_read_B24(const void *p,int q) {
  return (P[0]<<16)|(P[1]<<8)|P[2];
}
static void _bigpc_image_iter_write_B24(void *p,int q,uint32_t src) {
  P[0]=src>>16;
  P[1]=src>>8;
  P[2]=src;
}
static uint32_t _bigpc_image_iter_read_L24(const void *p,int q) {
  return P[0]|(P[1]<<8)|(P[2]<<16);
}
static void _bigpc_image_iter_write_L24(void *p,int q,uint32_t src) {
  P[0]=src;
  P[1]=src>>8;
  P[2]=src>>16;
}
static uint32_t _bigpc_image_iter_read_B64(const void *p,int q) {
  return (P[0]<<24)|(P[2]<<16)|(P[4]<<8)|P[6];
}
static void _bigpc_image_iter_write_B64(void *p,int q,uint32_t src) {
  P[0]=P[1]=src>>24;
  P[2]=P[3]=src>>16;
  P[4]=P[5]=src>>8;
  P[6]=P[7]=src;
}
static uint32_t _bigpc_image_iter_read_B48(const void *p,int q) {
  return (P[0]<<16)|(P[2]<<8)|P[4];
}
static void _bigpc_image_iter_write_B48(void *p,int q,uint32_t src) {
  P[0]=P[1]=src>>16;
  P[2]=P[3]=src>>8;
  P[4]=P[5]=src;
}

// Sub-byte pixels, a bit more involved:
static uint32_t _bigpc_image_iter_read_B4(const void *p,int q) {
  if (q) return P[0]&0x0f;
  return P[0]>>4;
}
static void _bigpc_image_iter_write_B4(void *p,int q,uint32_t src) {
  if (q) P[0]=(P[0]&0xf0)|src;
  else P[0]=(P[0]&0x0f)|(src<<4);
}
static uint32_t _bigpc_image_iter_read_L4(const void *p,int q) {
  if (q) return P[0]>>4;
  return P[0]&0x0f;
}
static void _bigpc_image_iter_write_L4(void *p,int q,uint32_t src) {
  if (q) P[0]=(P[0]&0x0f)|(src<<4);
  else P[0]=(P[0]&0xf0)|src;
}
static uint32_t _bigpc_image_iter_read_B2(const void *p,int q) {
  return (P[0]>>((3-q)<<1))&3;
}
static void _bigpc_image_iter_write_B2(void *p,int q,uint32_t src) {
  q<<=1;
  q=(8-q)&7;
  P[0]=(P[0]&~(3<<q))|(src<<q);
}
static uint32_t _bigpc_image_iter_read_L2(const void *p,int q) {
  return (P[0]>>(q<<1))&3;
}
static void _bigpc_image_iter_write_L2(void *p,int q,uint32_t src) {
  q<<=1;
  P[0]=(P[0]&~(3<<q))|(src<<q);
}
static uint32_t _bigpc_image_iter_read_B1(const void *p,int q) {
  return (P[0]&(0x80>>q))?1:0;
}
static void _bigpc_image_iter_write_B1(void *p,int q,uint32_t src) {
  if (src) P[0]|=0x80>>q;
  else P[0]&=~(0x80>>q);
}
static uint32_t _bigpc_image_iter_read_L1(const void *p,int q) {
  return (P[0]&(0x01<<q))?1:0;
}
static void _bigpc_image_iter_write_L1(void *p,int q,uint32_t src) {
  if (src) P[0]|=0x01<<q;
  else P[0]&=~(0x01<<q);
}

#undef P

/* Set up iterator.
 */

int bigpc_image_iterate(
  struct bigpc_image_iterator *iter,
  struct bigpc_image *image,
  int x,int y,int w,int h,
  uint8_t xform
) {
  if (!iter||!image) return -1;
  if ((w<1)||(h<1)) return -1;
  if ((x<0)||(y<0)) return -1;
  if ((x>image->w-w)||(y>image->h-h)) return -1;
  
  // Rephrase bounds and transform as a starting point and major/minor vectors.
  int startx=0,starty=0,dxminor=1,dyminor=0;
  if (xform&FMN_XFORM_XREV) startx=1;
  if (xform&FMN_XFORM_YREV) starty=1;
  if (xform&FMN_XFORM_SWAP) { dxminor=0; dyminor=1; }
  if (startx) dxminor=-dxminor;
  if (starty) dyminor=-dyminor;
  int dxmajor=dxminor?0:startx?-1:1;
  int dymajor=dyminor?0:starty?-1:1;
  int minorc,majorc;
  if (dxminor) { minorc=w; majorc=h; }
  else { minorc=h; majorc=w; }
  if (startx) startx=w-1; startx+=x;
  if (starty) starty=h-1; starty+=y;
  
  #define INITLRTB(pixelsize) { \
    iter->minor.p=pxv+starty*image->stride+((startx*pixelsize)>>3); \
    iter->minor.q=(startx*pixelsize)&7; \
    iter->minor.c=minorc; \
    iter->minor.d=pixelsize*dxminor+((image->stride*dyminor)<<3); \
    iter->major.p=iter->minor.p; \
    iter->major.q=iter->minor.q; \
    iter->major.c=majorc; \
    iter->major.d=pixelsize*dxmajor+((image->stride*dymajor)<<3); \
  }
  #define ACCESSORS(tag) { \
    iter->read=_bigpc_image_iter_read_##tag; \
    iter->write=_bigpc_image_iter_write_##tag; \
  }
  
  uint8_t *pxv=image->v;
  switch (image->storage) {
    case BIGPC_IMAGE_STORAGE_32: INITLRTB(32) ACCESSORS(32) break;
    case BIGPC_IMAGE_STORAGE_B24: INITLRTB(24) ACCESSORS(B24) break;
    case BIGPC_IMAGE_STORAGE_L24: INITLRTB(24) ACCESSORS(L24) break;
    case BIGPC_IMAGE_STORAGE_16: INITLRTB(16) ACCESSORS(16) break;
    case BIGPC_IMAGE_STORAGE_8: INITLRTB(8) ACCESSORS(8) break;
    case BIGPC_IMAGE_STORAGE_B4: INITLRTB(4) ACCESSORS(B4) break;
    case BIGPC_IMAGE_STORAGE_L4: INITLRTB(4) ACCESSORS(L4) break;
    case BIGPC_IMAGE_STORAGE_B2: INITLRTB(2) ACCESSORS(B2) break;
    case BIGPC_IMAGE_STORAGE_L2: INITLRTB(2) ACCESSORS(L2) break;
    case BIGPC_IMAGE_STORAGE_B1: INITLRTB(1) ACCESSORS(B1) break;
    case BIGPC_IMAGE_STORAGE_L1: INITLRTB(1) ACCESSORS(L1) break;
    case BIGPC_IMAGE_STORAGE_B64: INITLRTB(64) ACCESSORS(B64) break;
    case BIGPC_IMAGE_STORAGE_B48: INITLRTB(48) ACCESSORS(B48) break;
    case BIGPC_IMAGE_STORAGE_THUMBY: return -1; // TODO
    default: return -1;
  }
  
  #undef INITLRTB
  #undef ACCESSORS
  
  iter->minor.c--;
  iter->major.c--;
  iter->minor0=iter->minor;
  if (!(image->flags&BIGPC_IMAGE_FLAG_WRITEABLE)) iter->write=_bigpc_image_iter_write_dummy;
  return 0;
}

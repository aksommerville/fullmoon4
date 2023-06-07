#include "fmn_soft_internal.h"

/* Fill rect.
 */
 
void bigpc_image_fill_rect(struct bigpc_image *dst,int16_t x,int16_t y,int16_t w,int16_t h,uint32_t pixel) {
  if (x<0) { w+=x; x=0; }
  if (y<0) { h+=y; y=0; }
  if (x>dst->w-w) w=dst->w-x;
  if (y>dst->h-h) h=dst->h-y;
  if ((w<1)||(h<1)) return;
  switch (dst->storage) {
  
    case BIGPC_IMAGE_STORAGE_32: {
        // In 32-bit only, we allow alpha in the input. (also, we do it for "X" formats in addition to "A").
        uint8_t a=0xff;
        switch (dst->pixfmt) {
          case BIGPC_IMAGE_PIXFMT_RGBA: case BIGPC_IMAGE_PIXFMT_RGBX: a=pixel; break;
          case BIGPC_IMAGE_PIXFMT_BGRA: case BIGPC_IMAGE_PIXFMT_BGRX: a=pixel; break;
          case BIGPC_IMAGE_PIXFMT_ARGB: case BIGPC_IMAGE_PIXFMT_XRGB: a=pixel>>24; break;
          case BIGPC_IMAGE_PIXFMT_ABGR: case BIGPC_IMAGE_PIXFMT_XBGR: a=pixel>>24; break;
        }
        if (!a) return;
        if (a==0xff) {
          int wstride=dst->stride>>2;
          uint32_t *dstrow=((uint32_t*)dst->v)+wstride*y+x;
          for (;h-->0;dstrow+=wstride) {
            uint32_t *dstp=dstrow;
            int xi=w;
            for (;xi-->0;dstp++) *dstp=pixel;
          }
        } else {
          uint8_t da=0xff-a;
          uint8_t *dstrow=((uint8_t*)dst->v)+dst->stride*y+(x<<2);
          uint8_t s0,s1,s2,s3;
          uint16_t bocheck=0xff00;
          if (((uint8_t*)&bocheck)[1]) {
            s0=pixel;
            s1=pixel>>8;
            s2=pixel>>16;
            s3=pixel>>24;
          } else {
            s0=pixel>>24;
            s1=pixel>>16;
            s2=pixel>>8;
            s3=pixel;
          }
          for (;h-->0;dstrow+=dst->stride) {
            uint8_t *dstp=dstrow;
            int xi=w;
            for (;xi-->0;dstp+=4) {
              dstp[0]=(dstp[0]*da+s0*a)>>8;
              dstp[1]=(dstp[1]*da+s1*a)>>8;
              dstp[2]=(dstp[2]*da+s2*a)>>8;
              dstp[3]=(dstp[3]*da+s3*a)>>8;
            }
          }
        }
      } break;
      
    case BIGPC_IMAGE_STORAGE_16: {
        int wstride=dst->stride>>1;
        uint16_t *dstrow=((uint16_t*)dst->v)+wstride*y+x;
        for (;h-->0;dstrow+=wstride) {
          uint16_t *dstp=dstrow;
          int xi=w;
          for (;xi-->0;dstp++) *dstp=pixel;
        }
      } break;
    case BIGPC_IMAGE_STORAGE_8: {
        uint8_t *dstrow=((uint8_t*)dst->v)+dst->stride*y+x;
        for (;h-->0;dstrow+=dst->stride) memset(dstrow,pixel,w);
      } break;
  }
}

/* Trace line.
 */
 
void bigpc_image_trace_line(struct bigpc_image *dst,int16_t ax,int16_t ay,int16_t bx,int16_t by,uint32_t pixel) {
  //fprintf(stderr,"%s (%d,%d)..(%d,%d)=0x%08x\n",__func__,ax,ay,bx,by,pixel);
  
  // A quirk of OpenGL I guess, we try to draw points as lines with identical endpoints and it doesn't fly.
  // So our 1-pixel stars in the hello menu are actually 2x1.
  // We're not using line all that much, so whatever, make an exception for it here.
  // (we draw lines as the lord intended, 1x1 is perfectly fine)
  if ((ay==by)&&(ax==bx-1)) bx=ax;
  
  // If it's axis-aligned, draw as a rect instead.
  if ((ax==bx)||(ay==by)) {
    int16_t x=ax,y=ay,w=bx-ax,h=by-ay;
    if (w<0) { x=bx; w=-w; }
    if (h<0) { y=by; h=-h; }
    w++;
    h++;
    bigpc_image_fill_rect(dst,x,y,w,h,pixel);
    return;
  }
  
  if (dst->storage!=BIGPC_IMAGE_STORAGE_32) return;
  uint8_t sa=0xff;
  switch (dst->pixfmt) {
    case BIGPC_IMAGE_PIXFMT_RGBA: case BIGPC_IMAGE_PIXFMT_RGBX: sa=pixel; break;
    case BIGPC_IMAGE_PIXFMT_BGRA: case BIGPC_IMAGE_PIXFMT_BGRX: sa=pixel; break;
    case BIGPC_IMAGE_PIXFMT_ARGB: case BIGPC_IMAGE_PIXFMT_XRGB: sa=pixel>>24; break;
    case BIGPC_IMAGE_PIXFMT_ABGR: case BIGPC_IMAGE_PIXFMT_XBGR: sa=pixel>>24; break;
  }
  if (!sa) return;
  uint8_t da=0xff-sa;
  uint32_t *sv=(uint32_t*)&pixel;
  
  int16_t dx,dy,xweight,yweight; // xweight positive, yweight negative
  if (ax<bx) {
    dx=1;
    xweight=bx-ax;
  } else {
    dx=-1;
    xweight=ax-bx;
  }
  if (ay<by) {
    dy=1;
    yweight=ay-by;
  } else {
    dy=-1;
    yweight=by-ay;
  }
  int16_t xthresh=xweight>>1;
  int16_t ythresh=yweight>>1;
  int16_t weight=xweight+yweight;
  int16_t px=ax,py=ay;
  uint32_t *p=((uint32_t*)((uint8_t*)dst->v+py*dst->stride))+px;
  int dpy=(dy*dst->stride)>>2;
  while (1) {
    if ((px>=0)&&(px<dst->w)&&(py>=0)&&(py<dst->h)) {
      if (sa==0xff) *p=pixel;
      else {
        uint8_t *P=(uint8_t*)p;
        P[0]=(P[0]*da+sv[0]*sa)>>8;
        P[1]=(P[1]*da+sv[1]*sa)>>8;
        P[2]=(P[2]*da+sv[2]*sa)>>8;
        P[3]=(P[3]*da+sv[3]*sa)>>8;
      }
    }
    if ((px==bx)&&(py==by)) break;
    if ((weight>=xthresh)&&(px!=bx)) {
      px+=dx;
      p+=dx;
      weight+=yweight;
    } else if ((weight<=ythresh)&&(py!=by)) {
      py+=dy;
      p+=dpy;
      weight+=xweight;
    } else {
      if (px!=bx) { px+=dx; p+=dx; weight+=yweight; }
      if (py!=by) { py+=dy; p+=dpy; weight+=xweight; }
    }
  }
}

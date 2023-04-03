#include "bigpc_image.h"
#include <stdlib.h>
#include <limits.h>

/* Delete.
 */

void bigpc_image_del(struct bigpc_image *image) {
  if (!image) return;
  if (image->refc-->1) return;
  if (image->v&&(image->flags&BIGPC_IMAGE_FLAG_OWNV)) free(image->v);
  if (image->keepalive) bigpc_image_del(image->keepalive);
  free(image);
}

int bigpc_image_ref(struct bigpc_image *image) {
  if (!image) return -1;
  if (image->refc<1) return -1;
  if (image->refc==INT_MAX) return -1;
  image->refc++;
  return 0;
}

/* New, allocating pixels.
 */

struct bigpc_image *bigpc_image_new_alloc(int w,int h,int storage,int pixfmt) {
  int len=bigpc_image_measure(w,h,storage);
  if (len<1) return 0;
  void *v=calloc(1,len);
  if (!v) return 0;
  struct bigpc_image *image=bigpc_image_new_handoff(v,w,h,bigpc_image_measure(w,1,storage),storage,pixfmt);
  if (!image) {
    free(v);
    return 0;
  }
  return image;
}

struct bigpc_image *bigpc_image_new_handoff(
  void *v,
  int w,int h,int stride,
  int storage,int pixfmt
) {
  if (!v) return 0;
  int minstride=bigpc_image_measure(w,1,storage);
  if (minstride<1) return 0;
  if (stride<=0) stride=minstride;
  else if (stride<minstride) return 0;
  if (bigpc_image_measure(w,h,storage)<1) return 0;
  struct bigpc_image *image=calloc(1,sizeof(struct bigpc_image));
  if (!image) return 0;
  image->v=v;
  image->w=w;
  image->h=h;
  image->stride=stride;
  image->storage=storage;
  image->pixfmt=pixfmt;
  image->flags=BIGPC_IMAGE_FLAG_OWNV|BIGPC_IMAGE_FLAG_WRITEABLE;
  image->refc=1;
  return image;
}

struct bigpc_image *bigpc_image_new_borrow(
  const void *v,
  int w,int h,int stride,
  int storage,int pixfmt,
  struct bigpc_image *keepalive
) {
  struct bigpc_image *image=bigpc_image_new_handoff((void*)v,w,h,stride,storage,pixfmt);
  if (!image) return 0;
  image->flags=0;
  if (keepalive) {
    if (bigpc_image_ref(keepalive)<0) {
      bigpc_image_del(image);
      return 0;
    }
    image->keepalive=keepalive;
  }
  return image;
}

struct bigpc_image *bigpc_image_new_convert(
  const struct bigpc_image *src,
  int x,int y,int w,int h,
  int storage,int pixfmt
) {
  if (!src) return 0;
  if (w<0) w=src->w-x;
  if (h<0) h=src->h-y;
  if (storage<0) storage=src->storage;
  if (pixfmt<0) pixfmt=src->pixfmt;
  struct bigpc_image *dst=bigpc_image_new_alloc(w,h,storage,pixfmt);
  if (!dst) return 0;
  bigpc_image_xfer(dst,0,0,(struct bigpc_image*)src,x,y,w,h);
  return dst;
}

/* Copy pixels.
 */
 
void bigpc_image_xfer(
  struct bigpc_image *dst,int dstx,int dsty,
  struct bigpc_image *src,int srcx,int srcy,
  int w,int h
) {
  if (!dst||!src) return;
  if (!(dst->flags&BIGPC_IMAGE_FLAG_WRITEABLE)) return;
  if (dstx<0) { w+=dstx; dstx=0; }
  if (dsty<0) { h+=dsty; dsty=0; }
  if (srcx<0) { w+=srcx; srcx=0; }
  if (srcy<0) { h+=srcy; srcy=0; }
  if (dstx>dst->w-w) w=dst->w-dstx;
  if (dsty>dst->h-h) h=dst->h-dsty;
  if (srcx>src->w-w) w=src->w-srcx;
  if (srcy>src->h-h) h=src->h-srcy;
  if ((w<1)||(h<1)) return;
  
  // TODO This generic algorithm is very inefficient in likely cases like same format.
  // Write a few extra paths. Rowwise memcpy, iterate without convert, maybe hard code some common conversions?
  // Even TODO er! We need to do something about xfer across different pixfmt. For now, we transfer verbatim, which is way wrong.
  
  struct bigpc_image_iterator dstiter,srciter;
  if (bigpc_image_iterate(&dstiter,dst,dstx,dsty,w,h,0)<0) return;
  if (bigpc_image_iterate(&srciter,src,srcx,srcy,w,h,0)<0) return;
  do {
    uint32_t pixel=BIGPC_IMAGE_ITERATOR_READ(&srciter);
    BIGPC_IMAGE_ITERATOR_WRITE(&dstiter,pixel);
  } while (
    BIGPC_IMAGE_ITERATOR_NEXT(&dstiter)&&
    BIGPC_IMAGE_ITERATOR_NEXT(&srciter)
  );
}

/* Measure pixel buffer.
 */

int bigpc_image_measure(int w,int h,int storage) {
  if ((w<1)||(h<1)) return 0;
  if ((w>BIGPC_IMAGE_SIZE_LIMIT)||(h>BIGPC_IMAGE_SIZE_LIMIT)) return 0;
  switch (storage) {
    case BIGPC_IMAGE_STORAGE_32: return (w<<2)*h;
    case BIGPC_IMAGE_STORAGE_B24:
    case BIGPC_IMAGE_STORAGE_L24: return (w*3)*h;
    case BIGPC_IMAGE_STORAGE_16: return (w<<1)*h;
    case BIGPC_IMAGE_STORAGE_8: return w*h;
    case BIGPC_IMAGE_STORAGE_B4:
    case BIGPC_IMAGE_STORAGE_L4: return ((w+1)>>1)*h;
    case BIGPC_IMAGE_STORAGE_B2:
    case BIGPC_IMAGE_STORAGE_L2: return ((w+3)>>2)*h;
    case BIGPC_IMAGE_STORAGE_B1:
    case BIGPC_IMAGE_STORAGE_L1: return ((w+7)>>3)*h;
    case BIGPC_IMAGE_STORAGE_B64: return (w<<3)*h;
    case BIGPC_IMAGE_STORAGE_B48: return (w*6)*h;
    case BIGPC_IMAGE_STORAGE_THUMBY: return w*((h+7)>>3);
  }
  return 0;
}

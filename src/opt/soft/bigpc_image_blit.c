#include "fmn_soft_internal.h"

/* No transform, no scale, bounds pre-clipped.
 */
 
static void bigpc_image_blit_straight_32(int pixfmt,void *dst,int dstwstride,void *src,int srcwstride,int16_t w,int16_t h) {
  int alphap;
  switch (pixfmt) {
    case BIGPC_IMAGE_PIXFMT_RGBA: case BIGPC_IMAGE_PIXFMT_RGBX: alphap=3; goto _alpha_;
    case BIGPC_IMAGE_PIXFMT_BGRA: case BIGPC_IMAGE_PIXFMT_BGRX: alphap=3; goto _alpha_;
    case BIGPC_IMAGE_PIXFMT_ARGB: case BIGPC_IMAGE_PIXFMT_XRGB: alphap=0; goto _alpha_;
    case BIGPC_IMAGE_PIXFMT_ABGR: case BIGPC_IMAGE_PIXFMT_XBGR: alphap=0; goto _alpha_;
    _alpha_: {
        uint16_t bocheck=0xff00;
        if (((uint8_t*)&bocheck)[1]) alphap=3-alphap;
        for (;h-->0;dst=(uint32_t*)dst+dstwstride,src=(uint32_t*)src+srcwstride) {
          uint8_t *dstp=dst,*srcp=src;
          int16_t xi=w;
          for (;xi-->0;dstp+=4,srcp+=4) {
            uint8_t sa=srcp[alphap];
            if (!sa) {
            } else if (sa==0xff) {
              *(uint32_t*)dstp=*(uint32_t*)srcp;
            } else {
              uint8_t da=0xff-sa;
              dstp[1]=(dstp[1]*da+srcp[1]*sa)>>8;
              dstp[2]=(dstp[2]*da+srcp[2]*sa)>>8;
              if (alphap) dstp[0]=(dstp[0]*da+srcp[0]*sa)>>8;
              else dstp[3]=(dstp[3]*da+srcp[3]*sa)>>8;
            }
          }
        }
      } break;
    default: { // opaque
        for (;h-->0;dst=(uint32_t*)dst+dstwstride,src=(uint32_t*)src+srcwstride) {
          uint32_t *dstp=dst,*srcp=src;
          int16_t xi=w;
          for (;xi-->0;dstp++,srcp++) {
            if (*srcp) *dstp=*srcp; // TODO correct alpha check per pixfmt
          }
        }
      }
  }
}
 
static void bigpc_image_blit_straight_16(void *dst,int dstwstride,void *src,int srcwstride,int16_t w,int16_t h) {
  for (;h-->0;dst=(uint16_t*)dst+dstwstride,src=(uint16_t*)src+srcwstride) {
    uint16_t *dstp=dst,*srcp=src;
    int16_t xi=w;
    for (;xi-->0;dstp++,srcp++) {
      if (*srcp) *dstp=*srcp; // TODO correct alpha check per pixfmt
    }
  }
}
 
static void bigpc_image_blit_straight_8(void *dst,int dstwstride,void *src,int srcwstride,int16_t w,int16_t h) {
  for (;h-->0;dst=(uint8_t*)dst+dstwstride,src=(uint8_t*)src+srcwstride) {
    uint8_t *dstp=dst,*srcp=src;
    int16_t xi=w;
    for (;xi-->0;dstp++,srcp++) {
      if (*srcp) *dstp=*srcp; // TODO correct alpha check per pixfmt
    }
  }
}
 
static void bigpc_image_blit_straight(
  struct bigpc_image *dst,int16_t dstx,int16_t dsty,
  struct bigpc_image *src,int16_t srcx,int16_t srcy,
  int16_t w,int16_t h
) {
  uint8_t *dstrow=(uint8_t*)dst->v+dsty*dst->stride;
  uint8_t *srcrow=(uint8_t*)src->v+srcy*src->stride;
  if (dst->storage==src->storage) switch (dst->storage) {
    //TODO need to check pixfmt too
    case BIGPC_IMAGE_STORAGE_32: bigpc_image_blit_straight_32(src->pixfmt,dstrow+(dstx<<2),dst->stride>>2,srcrow+(srcx<<2),src->stride>>2,w,h); return;
    case BIGPC_IMAGE_STORAGE_16: bigpc_image_blit_straight_16(dstrow+(dstx<<1),dst->stride>>1,srcrow+(srcx<<1),src->stride>>1,w,h); return;
    case BIGPC_IMAGE_STORAGE_8: bigpc_image_blit_straight_8(dstrow+dstx,dst->stride,srcrow+srcx,src->stride,w,h); return;
  }
  //TODO General case with iterators.
}

/* No scaling or clipping, possible transform.
 */
 
static void bigpc_image_blit_noscale_noclip(
  struct bigpc_image *dst,int16_t dstx,int16_t dsty,
  struct bigpc_image *src,int16_t srcx,int16_t srcy,
  int16_t w,int16_t h,
  uint8_t xform
) {
  struct bigpc_image_iterator srciter,dstiter;
  if (bigpc_image_iterate(&dstiter,dst,dstx,dsty,w,h,0)<0) return;
  if (bigpc_image_iterate(&srciter,src,srcx,srcy,w,h,xform)<0) return;
  int alphashift=-1;
  switch (src->pixfmt) {
    case BIGPC_IMAGE_PIXFMT_RGBA: case BIGPC_IMAGE_PIXFMT_RGBX: alphashift=0; break;
    case BIGPC_IMAGE_PIXFMT_BGRA: case BIGPC_IMAGE_PIXFMT_BGRX: alphashift=0; break;
    case BIGPC_IMAGE_PIXFMT_ARGB: case BIGPC_IMAGE_PIXFMT_XRGB: alphashift=24; break;
    case BIGPC_IMAGE_PIXFMT_ABGR: case BIGPC_IMAGE_PIXFMT_XBGR: alphashift=24; break;
  }
  if (alphashift<0) {
    while (1) {
      uint32_t pixel=BIGPC_IMAGE_ITERATOR_READ(&srciter);
      if (pixel) {
        BIGPC_IMAGE_ITERATOR_WRITE(&dstiter,pixel);
      }
      if (!BIGPC_IMAGE_ITERATOR_NEXT(&dstiter)) break;
      if (!BIGPC_IMAGE_ITERATOR_NEXT(&srciter)) break;
    }
  } else {
    while (1) {
      uint32_t pixel=BIGPC_IMAGE_ITERATOR_READ(&srciter);
      uint8_t alpha=pixel>>alphashift;
      if (alpha==0x00) {
      } else if (alpha==0xff) {
        BIGPC_IMAGE_ITERATOR_WRITE(&dstiter,pixel);
      } else {
        uint32_t dstpixel=BIGPC_IMAGE_ITERATOR_READ(&dstiter);
        uint32_t compose=0;
        int i=4,shift=0;
        for (;i-->0;shift+=8) {
          int a=(pixel>>shift)&0xff,b=(dstpixel>>shift)&0xff;
          int c=((a*alpha)+(b*(0xff-alpha)))>>8;
          compose|=c<<shift;
        }
        BIGPC_IMAGE_ITERATOR_WRITE(&dstiter,compose);
      }
      if (!BIGPC_IMAGE_ITERATOR_NEXT(&dstiter)) break;
      if (!BIGPC_IMAGE_ITERATOR_NEXT(&srciter)) break;
    }
  }
}

/* No scaling, possible transform and clipping.
 */
 
static void bigpc_image_blit_noscale(
  struct bigpc_image *dst,int16_t dstx,int16_t dsty,
  struct bigpc_image *src,int16_t srcx,int16_t srcy,
  int16_t w,int16_t h,
  uint8_t xform
) {
  if (dst->storage!=BIGPC_IMAGE_STORAGE_32) return;
  if (src->storage!=BIGPC_IMAGE_STORAGE_32) return;
  int alphashift=-1;
  switch (src->pixfmt) {
    case BIGPC_IMAGE_PIXFMT_RGBA: case BIGPC_IMAGE_PIXFMT_RGBX: alphashift=0; break;
    case BIGPC_IMAGE_PIXFMT_BGRA: case BIGPC_IMAGE_PIXFMT_BGRX: alphashift=0; break;
    case BIGPC_IMAGE_PIXFMT_ARGB: case BIGPC_IMAGE_PIXFMT_XRGB: alphashift=24; break;
    case BIGPC_IMAGE_PIXFMT_ABGR: case BIGPC_IMAGE_PIXFMT_XBGR: alphashift=24; break;
  }
  int16_t srcdxx=1,srcdyx=0,srcdxy=0,srcdyy=1;
  if (xform&FMN_XFORM_XREV) {
    srcdxx=-1;
    srcx+=w-1;
  }
  if (xform&FMN_XFORM_YREV) {
    srcdyy=-1;
    srcy+=h-1;
  }
  if (xform&FMN_XFORM_SWAP) {
    srcdyx=srcdyy;
    srcdxy=srcdxx;
    srcdyy=0;
    srcdxx=0;
  }
  if (alphashift<0) {
    int16_t yi=0; for (;yi<h;yi++) {
      int16_t dsty1=dsty+yi;
      if ((dsty1>=0)&&(dsty1<dst->h)) {
        uint32_t *dstrow=(uint32_t*)((uint8_t*)dst->v+dst->stride*dsty1);
        int16_t xi=0; for (;xi<w;xi++) {
          int16_t dstx1=dstx+xi;
          if ((dstx1>=0)&&(dstx1<dst->w)) {
            int16_t srcx1=srcx+srcdxx*xi+srcdyx*yi;
            if ((srcx1>=0)&&(srcx1<src->w)) {
              int16_t srcy1=srcy+srcdxy*xi+srcdyy*yi;
              if ((srcy1>=0)&&(srcy1<src->h)) {
                uint32_t pixel=((uint32_t*)((uint8_t*)src->v+src->stride*srcy1))[srcx1];
                dstrow[dstx1]=pixel;
              }
            }
          }
        }
      }
    }
  } else {
    int16_t yi=0; for (;yi<h;yi++) {
      int16_t dsty1=dsty+yi;
      if ((dsty1>=0)&&(dsty1<dst->h)) {
        uint32_t *dstrow=(uint32_t*)((uint8_t*)dst->v+dst->stride*dsty1);
        int16_t xi=0; for (;xi<w;xi++) {
          int16_t dstx1=dstx+xi;
          if ((dstx1>=0)&&(dstx1<dst->w)) {
            int16_t srcx1=srcx+srcdxx*xi+srcdyx*yi;
            if ((srcx1>=0)&&(srcx1<src->w)) {
              int16_t srcy1=srcy+srcdxy*xi+srcdyy*yi;
              if ((srcy1>=0)&&(srcy1<src->h)) {
                uint32_t pixel=((uint32_t*)((uint8_t*)src->v+src->stride*srcy1))[srcx1];
                uint8_t alpha=pixel>>alphashift;
                if (!alpha) continue;
                if (alpha==0xff) {
                  dstrow[dstx1]=pixel;
                  continue;
                }
                uint32_t dstpixel=dstrow[dstx1];
                uint32_t compose=0;
                int i=4,shift=0;
                for (;i-->0;shift+=8) {
                  int a=(pixel>>shift)&0xff,b=(dstpixel>>shift)&0xff;
                  int c=((a*alpha)+(b*(0xff-alpha)))>>8;
                  compose|=c<<shift;
                }
                dstrow[dstx1]=compose;
              }
            }
          }
        }
      }
    }
  }
}

/* Scaling, so we might as well do all the other checks per-pixel too.
 */
 
static void bigpc_image_blit_sample(
  struct bigpc_image *dst,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,
  struct bigpc_image *src,int16_t srcx,int16_t srcy,int16_t srcw,int16_t srch,
  uint8_t xform
) {
  //TODO ...I'm pretty sure this case never happens.
}

/* Blit, public entry point.
 * Select strategy and dispatch above.
 */
 
void bigpc_image_blit(
  struct bigpc_image *dst,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,
  struct bigpc_image *src,int16_t srcx,int16_t srcy,int16_t srcw,int16_t srch,
  uint8_t xform
) {
  if (!dst||!src) return;
  
  // Most optimal paths are only for no-scale cases.
  if ((dstw==srcw)&&(dsth==srch)) {
  
    /* If not transformed or scaled, we can easily clip in advance.
     * This is of course possible in transform cases too, but it would be complicated.
     * The no-transform-no-scale case is very common.
     */
    if (!xform) {
      if (dstx<0) { dstw+=dstx; srcx-=dstx; dstx=0; }
      if (dsty<0) { dsth+=dsty; srcy-=dsty; dsty=0; }
      if (srcx<0) { dstw+=srcx; dstx-=srcx; srcx=0; }
      if (srcy<0) { dsth+=srcy; dsty-=srcy; srcy=0; }
      if (dstx>dst->w-dstw) dstw=dst->w-dstx;
      if (dsty>dst->h-dsth) dsth=dst->h-dsty;
      if (srcx>src->w-dstw) dstw=src->w-srcx;
      if (srcy>src->h-dsth) dsth=src->h-srcy;
      bigpc_image_blit_straight(dst,dstx,dsty,src,srcx,srcy,dstw,dsth);
      return;
    }
  
    // If both bounds are already valid, we can skip the per-pixel bounds check.
    if (
      (dstx>=0)&&(dsty>=0)&&(srcx>=0)&&(srcy>=0)&&
      (dstx+dstw<=dst->w)&&(dsty+dsth<=dst->h)&&
      (srcx+dstw<=src->w)&&(srcy+dsth<=src->h)
    ) {
      bigpc_image_blit_noscale_noclip(dst,dstx,dsty,src,srcx,srcy,dstw,dsth,xform);
      return;
    }
    
    bigpc_image_blit_noscale(dst,dstx,dsty,src,srcx,srcy,dstw,dsth,xform);
    return;
  }
  
  // Scaling might be in play, so we have to do it the hard way.
  bigpc_image_blit_sample(dst,dstx,dsty,dstw,dsth,src,srcx,srcy,srcw,srch,xform);
}

/* Blit, recoloring and scaling.
 */

static void bigpc_image_blit_recolor_scale(
  struct bigpc_image *dst,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,
  struct bigpc_image *src,int16_t srcx,int16_t srcy,int16_t srcw,int16_t srch,
  uint32_t pixel
) {
  int16_t drawx=dstx,drawy=dsty,draww=dstw,drawh=dsth; // (dst) bounds but clipped. keep the original for scaling.
  int16_t x0=0,y0=0;
  if (drawx<0) { x0=-drawx; draww+=drawx; drawx=0; }
  if (drawy<0) { y0=-drawy; drawh+=drawy; drawy=0; }
  if (drawx>dst->w-draww) draww=dst->w-drawx;
  if (drawy>dst->h-drawh) drawh=dst->h-drawy;
  if ((draww<1)||(drawh<1)) return;
  int alphashift=-1;
  switch (dst->pixfmt) {
    case BIGPC_IMAGE_PIXFMT_RGBA: case BIGPC_IMAGE_PIXFMT_RGBX: alphashift=0; break;
    case BIGPC_IMAGE_PIXFMT_BGRA: case BIGPC_IMAGE_PIXFMT_BGRX: alphashift=0; break;
    case BIGPC_IMAGE_PIXFMT_ARGB: case BIGPC_IMAGE_PIXFMT_XRGB: alphashift=24; break;
    case BIGPC_IMAGE_PIXFMT_ABGR: case BIGPC_IMAGE_PIXFMT_XBGR: alphashift=24; break;
  }
  if (alphashift<0) {
    bigpc_image_fill_rect(dst,drawx,drawy,draww,drawh,pixel);
    return;
  }
  uint8_t ka=pixel>>alphashift;
  if (!ka) return;
  int16_t srcwstride=src->stride>>2;
  int16_t dstwstride=dst->stride>>2;
  uint32_t *dstrow=((uint32_t*)dst->v)+dstwstride*drawy+drawx;
  uint8_t *pixp=(uint8_t*)&pixel;
  int16_t yi=0;
  for (;yi<drawh;yi++,dstrow+=dstwstride) {
    int16_t sy=srcy+((yi+y0)*srch)/dsth;
    if ((sy<srcy)||(sy>=srcy+srch)) continue;
    uint8_t *dstp=(uint8_t*)dstrow;
    int16_t xi=0;
    for (;xi<draww;xi++,dstp+=4) {
      int16_t sx=srcx+((xi+x0)*srcw)/dstw;
      if ((sx<srcx)||(sx>=srcx+srcw)) continue;
      uint32_t pixel=((uint32_t*)src->v+sy*srcwstride)[sx];
      uint8_t sa=pixel>>alphashift;
      sa=(sa*ka)>>8;
      if (!sa) {
      } else if (sa==0xff) {
        *(uint32_t*)dstp=pixel;
      } else {
        uint8_t da=0xff-sa;
        dstp[0]=(dstp[0]*da+pixp[0]*sa)>>8;
        dstp[1]=(dstp[1]*da+pixp[1]*sa)>>8;
        dstp[2]=(dstp[2]*da+pixp[2]*sa)>>8;
        dstp[3]=(dstp[3]*da+pixp[3]*sa)>>8;
      }
    }
  }
}

/* Blit, recoloring.
 */

void bigpc_image_blit_recolor(
  struct bigpc_image *dst,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,
  struct bigpc_image *src,int16_t srcx,int16_t srcy,int16_t srcw,int16_t srch,
  uint32_t pixel
) {
  if (!dst||(dst->storage!=BIGPC_IMAGE_STORAGE_32)) return;
  if (!src||(src->storage!=BIGPC_IMAGE_STORAGE_32)) return;
  if ((dstw!=srcw)||(dsth!=srch)) {
    bigpc_image_blit_recolor_scale(dst,dstx,dsty,dstw,dsth,src,srcx,srcy,srcw,srch,pixel);
    return;
  }
  int16_t w=dstw,h=dsth;
  if (dstx<0) { srcx-=dstx; w+=dstx; dstx=0; }
  if (dsty<0) { srcy-=dsty; h+=dsty; dsty=0; }
  if (srcx<0) { dstx-=srcx; w+=srcx; srcx=0; }
  if (srcy<0) { dsty-=srcy; h+=srcy; srcy=0; }
  if (dstx>dst->w-w) w=dst->w-dstx;
  if (dsty>dst->h-h) h=dst->h-dsty;
  if (srcx>src->w-w) w=src->w-srcx;
  if (srcy>src->h-h) h=src->h-srcy;
  if ((w<1)||(h<1)) return;
  int alphashift=-1;
  switch (dst->pixfmt) {
    case BIGPC_IMAGE_PIXFMT_RGBA: case BIGPC_IMAGE_PIXFMT_RGBX: alphashift=0; break;
    case BIGPC_IMAGE_PIXFMT_BGRA: case BIGPC_IMAGE_PIXFMT_BGRX: alphashift=0; break;
    case BIGPC_IMAGE_PIXFMT_ARGB: case BIGPC_IMAGE_PIXFMT_XRGB: alphashift=24; break;
    case BIGPC_IMAGE_PIXFMT_ABGR: case BIGPC_IMAGE_PIXFMT_XBGR: alphashift=24; break;
  }
  if (alphashift<0) {
    bigpc_image_fill_rect(dst,dstx,dsty,w,h,pixel);
    return;
  }
  uint8_t ka=pixel>>alphashift;
  if (!ka) return;
  int16_t srcwstride=src->stride>>2;
  int16_t dstwstride=dst->stride>>2;
  uint32_t *dstrow=((uint32_t*)dst->v)+dstwstride*dsty+dstx;
  uint32_t *srcrow=((uint32_t*)src->v)+srcwstride*srcy+srcx;
  uint8_t *pixp=(uint8_t*)&pixel;
  for (;h-->0;dstrow+=dstwstride,srcrow+=srcwstride) {
    uint8_t *dstp=(uint8_t*)dstrow;
    uint32_t *srcp=srcrow;
    int16_t xi=w;
    for (;xi-->0;dstp+=4,srcp++) {
      uint8_t sa=(*srcp)>>alphashift;
      sa=(sa*ka)>>8;
      if (!sa) {
      } else if (sa==0xff) {
        *(uint32_t*)dstp=pixel;
      } else {
        uint8_t da=0xff-sa;
        dstp[0]=(dstp[0]*da+pixp[0]*sa)>>8;
        dstp[1]=(dstp[1]*da+pixp[1]*sa)>>8;
        dstp[2]=(dstp[2]*da+pixp[2]*sa)>>8;
        dstp[3]=(dstp[3]*da+pixp[3]*sa)>>8;
      }
    }
  }
}

/* Blit with rotation.
 */

void bigpc_image_blit_rotate(
  struct bigpc_image *dst,int16_t dstx,int16_t dsty,
  struct bigpc_image *src,int16_t srcx,int16_t srcy,int16_t srcw,int16_t srch,
  uint8_t rotate
) {
  if (dst->storage!=BIGPC_IMAGE_STORAGE_32) return;
  if (src->storage!=BIGPC_IMAGE_STORAGE_32) return;
  uint32_t amask=0xffffffff;
  switch (dst->pixfmt) {
    case BIGPC_IMAGE_PIXFMT_RGBA: case BIGPC_IMAGE_PIXFMT_RGBX:
    case BIGPC_IMAGE_PIXFMT_BGRA: case BIGPC_IMAGE_PIXFMT_BGRX: amask=0x000000ff; break;
    case BIGPC_IMAGE_PIXFMT_ARGB: case BIGPC_IMAGE_PIXFMT_XRGB:
    case BIGPC_IMAGE_PIXFMT_ABGR: case BIGPC_IMAGE_PIXFMT_XBGR: amask=0xff000000; break;
  }
  
  int16_t radius=ceilf((srcw*M_SQRT2)/2.0f);
  int16_t px=dstx-radius;
  int16_t py=dsty-radius;
  int16_t pw=radius<<1;
  int16_t ph=pw;
  if (px<0) { pw+=px; px=0; }
  if (py<0) { ph+=py; py=0; }
  if (px>dst->w-pw) pw=dst->w-px;
  if (py>dst->h-ph) ph=dst->h-py;
  
  float smidx=srcx+srcw*0.5f;
  float smidy=srcy+srch*0.5f;
  float t=(rotate*M_PI*2.0f)/-256.0f;
  float mtx[4]={
    cosf(t),-sinf(t),
    sinf(t),cosf(t),
  };
  
  int16_t srcwstride=src->stride>>2;
  int16_t dstwstride=dst->stride>>2;
  uint32_t *dstrow=((uint32_t*)dst->v)+dstwstride*py+px;
  int16_t yi=ph;
  for (;yi-->0;dstrow+=dstwstride) {
    float fy=py+ph-yi-dsty;
    uint32_t *dstp=dstrow;
    int16_t xi=pw;
    for (;xi-->0;dstp++) {
      float fx=px+pw-xi-dstx;
      int16_t sx=smidx+fx*mtx[0]+fy*mtx[1];
      if ((sx<srcx)||(sx>=srcx+srcw)) continue;
      int16_t sy=smidy+fx*mtx[2]+fy*mtx[3];
      if ((sy<srcy)||(sy>=srcy+srch)) continue;
      uint32_t pixel=((uint32_t*)src->v+sy*srcwstride)[sx];
      if (!(pixel&amask)) continue;
      // Skipping alpha blend...
      *dstp=pixel;
    }
  }
}

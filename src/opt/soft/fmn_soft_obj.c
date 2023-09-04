#include "fmn_soft_internal.h"

/* Cleanup.
 */
 
static void soft_image_cleanup(struct soft_image *image) {
  bigpc_image_del(image->image);
}
 
static void _soft_del(struct bigpc_render_driver *driver) {
  if (DRIVER->imagev) {
    while (DRIVER->imagec-->0) soft_image_cleanup(DRIVER->imagev+DRIVER->imagec);
    free(DRIVER->imagev);
  }
  bigpc_image_del(DRIVER->output);
}

/* Init.
 */
 
static struct bigpc_image *_soft_init_fb(void *userdata) {
  struct bigpc_image *image=bigpc_image_new_borrow("\0\0\0\0",1,1,4,BIGPC_IMAGE_STORAGE_32,BIGPC_IMAGE_PIXFMT_RGBA,0);
  if (!image) return 0;
  image->flags=BIGPC_IMAGE_FLAG_WRITEABLE;
  return image;
}
 
static int _soft_init(struct bigpc_render_driver *driver,struct bigpc_video_driver *video) {
  //fprintf(stderr,"%s:%d:%s: Initializing soft renderer for video driver '%s'\n",__FILE__,__LINE__,__func__,video->type->name);
  
  if (video->renderer!=BIGPC_RENDERER_rgb32) {
    return -1;
  }
  
  // Image zero must always exist. We'll create a placeholder.
  if (!fmn_soft_image_get(driver,0,_soft_init_fb,0)) return -1;
  if (bigpc_image_ref(DRIVER->imagev[0].image)<0) return -1;
  DRIVER->output=DRIVER->imagev[0].image;
  DRIVER->outputid=0;
  
  return 0;
}

/* Configure.
 */
 
static int8_t _soft_video_init(
  struct bigpc_render_driver *driver,
  int16_t wmin,int16_t wmax,
  int16_t hmin,int16_t hmax,
  uint8_t pixfmt
) {

  // Prefer (320,192) but clamp to requested range. bigpc_image_new_alloc() will assert absolute limits for us.
  int fbw=320,fbh=192;
  if (fbw<wmin) fbw=wmin; else if (fbw>wmax) fbw=wmax;
  if (fbh<hmin) fbh=hmin; else if (fbh>hmax) fbh=hmax;
  DRIVER->fbw=fbw;
  DRIVER->fbh=fbh;
  DRIVER->pixfmt=pixfmt;
  
  return 0;
}

/* Trivial accessors.
 */

static void _soft_video_get_framebuffer_size(int16_t *w,int16_t *h,struct bigpc_render_driver *driver) {
  *w=DRIVER->fbw;
  *h=DRIVER->fbh;
}

static uint8_t _soft_video_get_pixfmt(struct bigpc_render_driver *driver) {
  return DRIVER->pixfmt;
}

/* Framebuffer format.
 */

static uint32_t _soft_video_rgba_from_pixel(struct bigpc_render_driver *driver,uint32_t pixel) {
  switch (DRIVER->mainpixfmt) {
    case BIGPC_IMAGE_PIXFMT_RGBA: return pixel;
    case BIGPC_IMAGE_PIXFMT_BGRA: return (pixel&0x00ff00ff)|((pixel&0xff000000)>>16)|((pixel&0xff00)<<16);
    case BIGPC_IMAGE_PIXFMT_ARGB: return (pixel<<8)|(pixel>>24);
    case BIGPC_IMAGE_PIXFMT_ABGR: return (pixel>>24)|((pixel&0xff0000)>>8)|((pixel&0xff00)<<8)|(pixel<<24);
    case BIGPC_IMAGE_PIXFMT_RGBX: return pixel|0xff;
    case BIGPC_IMAGE_PIXFMT_BGRX: return (pixel&0x00ff0000)|((pixel&0xff000000)>>16)|((pixel&0xff00)<<16)|0xff;
    case BIGPC_IMAGE_PIXFMT_XRGB: return (pixel<<8)|0xff;
    case BIGPC_IMAGE_PIXFMT_XBGR: return 0xff|((pixel&0xff0000)>>8)|((pixel&0xff00)<<8)|(pixel<<24);
    case BIGPC_IMAGE_PIXFMT_RGB565: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_BGR565: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_RGBA5551: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_BGRA5551: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_ARGB1555: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_ABGR1555: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_RGBX5551: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_BGRX5551: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_XRGB1555: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_XBGR1555: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_Y1: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_Y2: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_Y4: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_Y8: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_Y16: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_YA88: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_YA16: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_AY88: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_AY16: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_A1: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_A2: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_A4: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_A8: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_W1: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_I1: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_I2: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_I4: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_I8: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_I16: return pixel;//TODO
    case BIGPC_IMAGE_PIXFMT_I32: return pixel;//TODO
  }
  return pixel;
}

static uint32_t _soft_video_pixel_from_rgba(struct bigpc_render_driver *driver,uint32_t rgba) {
  switch (DRIVER->mainpixfmt) {
    case BIGPC_IMAGE_PIXFMT_RGBA: return rgba;
    case BIGPC_IMAGE_PIXFMT_BGRA: return (rgba&0x00ff00ff)|((rgba&0xff000000)>>16)|((rgba&0xff00)<<16);
    case BIGPC_IMAGE_PIXFMT_ARGB: return (rgba>>8)|(rgba<<24);
    case BIGPC_IMAGE_PIXFMT_ABGR: return (rgba<<24)|((rgba&0xff00)<<8)|((rgba&0xff0000)>>8)|(rgba>>24);
    case BIGPC_IMAGE_PIXFMT_RGBX: return rgba;
    case BIGPC_IMAGE_PIXFMT_BGRX: return (rgba&0x00ff00ff)|((rgba&0xff000000)>>16)|((rgba&0xff00)<<16);
    case BIGPC_IMAGE_PIXFMT_XRGB: return (rgba>>8)|(rgba<<24);
    case BIGPC_IMAGE_PIXFMT_XBGR: return (rgba<<24)|((rgba&0xff00)<<8)|((rgba&0xff0000)>>8)|(rgba>>24);
    case BIGPC_IMAGE_PIXFMT_RGB565: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_BGR565: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_RGBA5551: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_BGRA5551: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_ARGB1555: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_ABGR1555: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_RGBX5551: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_BGRX5551: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_XRGB1555: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_XBGR1555: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_Y1: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_Y2: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_Y4: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_Y8: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_Y16: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_YA88: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_YA16: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_AY88: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_AY16: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_A1: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_A2: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_A4: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_A8: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_W1: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_I1: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_I2: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_I4: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_I8: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_I16: return rgba;//TODO
    case BIGPC_IMAGE_PIXFMT_I32: return rgba;//TODO
  }
  return rgba;
}

/* Images.
 */

struct _soft_video_init_image_create_context {
  struct bigpc_render_driver *driver;
  int16_t w,h;
};

static struct bigpc_image *_soft_video_init_image_create_cb(void *userdata) {
  struct _soft_video_init_image_create_context *ctx=userdata;
  struct bigpc_render_driver *driver=ctx->driver;
  int pixfmt=DRIVER->mainpixfmt?DRIVER->mainpixfmt:DRIVER->pixfmt;
  return bigpc_image_new_alloc(ctx->w,ctx->h,BIGPC_IMAGE_STORAGE_32,pixfmt);
}

static void _soft_video_init_image(struct bigpc_render_driver *driver,uint16_t imageid,int16_t w,int16_t h) {
  struct _soft_video_init_image_create_context ctx={
    .driver=driver,
    .w=w,
    .h=h,
  };
  struct bigpc_image *image=fmn_soft_image_get(driver,imageid,_soft_video_init_image_create_cb,&ctx);
}

static void _soft_video_get_image_size(int16_t *w,int16_t *h,struct bigpc_render_driver *driver,uint16_t imageid) {
  struct bigpc_image *image=fmn_soft_image_get(driver,imageid,0,0);
  if (image) {
    *w=image->w;
    *h=image->h;
  } else {
    *w=*h=0;
  }
}

static int8_t _soft_draw_set_output(struct bigpc_render_driver *driver,uint16_t imageid) {
  if (imageid==DRIVER->outputid) return 0;
  struct _soft_video_init_image_create_context ctx={
    .driver=driver,
    .w=DRIVER->fbw,
    .h=DRIVER->fbh,
  };
  struct bigpc_image *image=fmn_soft_image_get(driver,imageid,_soft_video_init_image_create_cb,&ctx);
  if (bigpc_image_ref(image)<0) return -1;
  bigpc_image_del(DRIVER->output);
  DRIVER->output=image;
  DRIVER->outputid=imageid;
  return 0;
}

/* Render, public entry points.
 */
 
static void _soft_draw_clear(struct bigpc_render_driver *driver) {
  if (!DRIVER->output) return;
  bigpc_image_clear(DRIVER->output);
}
 
static void _soft_draw_line(struct bigpc_render_driver *driver,const struct fmn_draw_line *v,int c) {
  if (!DRIVER->output) return;
  for (;c-->0;v++) {
    bigpc_image_trace_line(DRIVER->output,v->ax,v->ay,v->bx,v->by,v->pixel);
  }
}

static void _soft_draw_rect(struct bigpc_render_driver *driver,const struct fmn_draw_rect *v,int c) {
  if (!DRIVER->output) return;
  for (;c-->0;v++) {
    bigpc_image_fill_rect(DRIVER->output,v->x,v->y,v->w,v->h,v->pixel);
  }
}

static void _soft_draw_mintile(struct bigpc_render_driver *driver,const struct fmn_draw_mintile *v,int c,uint16_t srcimageid) {
  if (!DRIVER->output) return;
  struct bigpc_image *srcimage=fmn_soft_image_get(driver,srcimageid,0,0);
  if (!srcimage) return;
  int16_t colw=srcimage->w>>4;
  int16_t rowh=srcimage->h>>4;
  int16_t halfw=colw>>1;
  int16_t halfh=rowh>>1;
  for (;c-->0;v++) {
    int16_t srcx=(v->tileid&0x0f)*colw;
    int16_t srcy=(v->tileid>>4)*rowh;
    int16_t dstx=v->x-halfw;
    int16_t dsty=v->y-halfh;
    bigpc_image_blit(DRIVER->output,dstx,dsty,colw,rowh,srcimage,srcx,srcy,colw,rowh,v->xform);
  }
}

static void _soft_draw_maxtile(struct bigpc_render_driver *driver,const struct fmn_draw_maxtile *v,int c,uint16_t srcimageid) {
  if (!DRIVER->output) return;
  struct bigpc_image *srcimage=fmn_soft_image_get(driver,srcimageid,0,0);
  if (!srcimage) return;
  int16_t colw=srcimage->w>>4;
  int16_t rowh=srcimage->h>>4;
  int16_t halfw=colw>>1;
  int16_t halfh=rowh>>1;
  for (;c-->0;v++) {
    int16_t srcx=(v->tileid&0x0f)*colw;
    int16_t srcy=(v->tileid>>4)*rowh;
    int16_t dstx=v->x-halfw;
    int16_t dsty=v->y-halfh;
    // rotate, scale, recolor... actually the only thing we use is rotate
    bigpc_image_blit_rotate(DRIVER->output,v->x,v->y,srcimage,srcx,srcy,colw,rowh,v->rotate);
  }
}

static void _soft_draw_decal(struct bigpc_render_driver *driver,const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {
  if (!DRIVER->output) return;
  struct bigpc_image *srcimage=fmn_soft_image_get(driver,srcimageid,0,0);
  if (!srcimage) return;
  for (;c-->0;v++) {
    int16_t dstx=v->dstx,dsty=v->dsty,dstw=v->dstw,dsth=v->dsth;
    int16_t srcx=v->srcx,srcy=v->srcy,srcw=v->srcw,srch=v->srch;
    uint8_t xform=0;
    if (dstw<0) { dstx+=dstw; dstw=-dstw; xform^=FMN_XFORM_XREV; }
    if (dsth<0) { dsty+=dsth; dsth=-dsth; xform^=FMN_XFORM_YREV; }
    if (srcw<0) { srcx+=srcw; srcw=-srcw; xform^=FMN_XFORM_XREV; }
    if (srch<0) { srcy+=srch; srch=-srch; xform^=FMN_XFORM_YREV; }
    bigpc_image_blit(
      DRIVER->output,dstx,dsty,dstw,dsth,
      srcimage,srcx,srcy,srcw,srch,
      xform
    );
  }
}

static void _soft_draw_recal(struct bigpc_render_driver *driver,const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {
  if (!DRIVER->output) return;
  struct bigpc_image *srcimage=fmn_soft_image_get(driver,srcimageid,0,0);
  if (!srcimage) return;
  for (;c-->0;v++) {
    bigpc_image_blit_recolor(
      DRIVER->output,v->dstx,v->dsty,v->dstw,v->dsth,
      srcimage,v->srcx,v->srcy,v->srcw,v->srch,
      v->pixel
    );
  }
}

// I didn't end up using the axis-swap decal options, so not bothering to implement here.
static void _soft_draw_decal_swap(struct bigpc_render_driver *driver,const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {
}
static void _soft_draw_recal_swap(struct bigpc_render_driver *driver,const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {
}

/* Frame fences.
 */

static void _soft_begin(struct bigpc_render_driver *driver,struct bigpc_image *fb) {
  if (_soft_draw_set_output(driver,0)<0) return;
  DRIVER->output->v=fb->v;
  DRIVER->output->w=fb->w;
  DRIVER->output->h=fb->h;
  DRIVER->output->stride=fb->stride;
  DRIVER->output->storage=fb->storage;
  DRIVER->output->pixfmt=fb->pixfmt;
  
  /* All of our images should internally convert to the main output format.
   * Unfortunately, we don't reliably know that format until the first render.
   * So check it here, convert any images already created, and record it for future images.
   */
  if (!DRIVER->mainpixfmt) {
    DRIVER->mainpixfmt=fb->pixfmt;
    struct soft_image *image=DRIVER->imagev;
    int i=DRIVER->imagec;
    for (;i-->0;image++) {
      bigpc_image_convert_in_place(image->image,DRIVER->mainpixfmt);
    }
  }
}

static void _soft_end(struct bigpc_render_driver *driver,uint8_t client_result) {
}

/* Type.
 */
 
const struct bigpc_render_type bigpc_render_type_soft={
  .name="soft",
  .desc="Pure software rendering, no acceleration.",
  .objlen=sizeof(struct bigpc_render_driver_soft),
  .video_renderer_id=BIGPC_RENDERER_rgb32,
  .del=_soft_del,
  .init=_soft_init,
  
  .video_init=_soft_video_init,
  .video_get_framebuffer_size=_soft_video_get_framebuffer_size,
  .video_get_pixfmt=_soft_video_get_pixfmt,
  .video_rgba_from_pixel=_soft_video_rgba_from_pixel,
  .video_pixel_from_rgba=_soft_video_pixel_from_rgba,
  .video_init_image=_soft_video_init_image,
  .video_get_image_size=_soft_video_get_image_size,
  .draw_set_output=_soft_draw_set_output,
  .draw_clear=_soft_draw_clear,
  .draw_line=_soft_draw_line,
  .draw_rect=_soft_draw_rect,
  .draw_mintile=_soft_draw_mintile,
  .draw_maxtile=_soft_draw_maxtile,
  .draw_decal=_soft_draw_decal,
  .draw_decal_swap=_soft_draw_decal_swap,
  .draw_recal=_soft_draw_recal,
  .draw_recal_swap=_soft_draw_recal_swap,
  .begin=_soft_begin,
  .end=_soft_end,
};

#include "fmn_gl2_internal.h"

/* Cleanup.
 */
 
static void _gl2_del(struct bigpc_render_driver *driver) {
  if (DRIVER->texturev) {
    while (DRIVER->texturec-->0) fmn_gl2_texture_del(DRIVER->texturev[DRIVER->texturec]);
    free(DRIVER->texturev);
  }
  fmn_gl2_texture_cleanup(&DRIVER->mainfb);
  #define _(tag) fmn_gl2_program_cleanup(&DRIVER->program_##tag);
  FMN_GL2_FOR_EACH_PROGRAM
  #undef _
  if (DRIVER->rawvtxv) free(DRIVER->rawvtxv);
}

/* Init.
 */
 
static int _gl2_init(struct bigpc_render_driver *driver,struct bigpc_video_driver *video) {
  int err;
  
  DRIVER->viewscale=video->viewscale;

  #define _(tag) if ((err=fmn_gl2_program_##tag##_init(&DRIVER->program_##tag,driver))<0) { \
    if (err!=-2) fprintf(stderr,"gl2:%s: Failed to initialize shader.\n",#tag); \
    return -2; \
  }
  FMN_GL2_FOR_EACH_PROGRAM
  #undef _
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_POINT_SPRITE);
  
  const int fbborder=16;
  if (
    (fmn_gl2_texture_init_rgb(&DRIVER->mainfb,video->fbw+(fbborder<<1),video->fbh+(fbborder<<1),0)<0)||
    (fmn_gl2_texture_require_framebuffer(&DRIVER->mainfb)<0)
  ) return -1;
  DRIVER->mainfb.border=fbborder;
  
  return 0;
}

/* Raw vertex cache.
 */
 
int fmn_gl2_rawvtxv_require(struct bigpc_render_driver *driver,int c) {
  if (c<=DRIVER->rawvtxa) return 0;
  if (c>4096) return -1;
  int na=(c+256)&~255;
  void *nv=realloc(DRIVER->rawvtxv,sizeof(struct fmn_gl2_vertex_raw)*na);
  if (!nv) return -1;
  DRIVER->rawvtxv=nv;
  DRIVER->rawvtxa=na;
  return 0;
}

/* Change filter for main framebuffer texture.
 */
 
static void fmn_gl2_set_main_filter(struct bigpc_render_driver *driver,int filter) {
  glBindTexture(GL_TEXTURE_2D,DRIVER->mainfb.texid);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);
}

/* Freshen output bounds if needed.
 */
 
static void fmn_gl2_require_output_bounds(struct bigpc_render_driver *driver) {
  if ((DRIVER->pvw==driver->w)&&(DRIVER->pvh==driver->h)) return;
  int fbw=DRIVER->mainfb.w-(DRIVER->mainfb.border<<1);
  int fbh=DRIVER->mainfb.h-(DRIVER->mainfb.border<<1);
  
  // Initial guess. Take the largest box in window that preserve's framebuffer's aspect ratio.
  int wforh=(fbw*driver->h)/fbh;
  if (wforh<=driver->w) {
    DRIVER->dstw=wforh;
    DRIVER->dsth=driver->h;
  } else {
    DRIVER->dstw=driver->w;
    DRIVER->dsth=(fbh*driver->w)/fbw;
  }
  int scale=DRIVER->dstw/fbw;
  
  // If we ended up within 10 pixels below or 20 above an integer multiple, cheat it out.
  // Used to reset the filter here, but now the user can do that manually.
  int perfectw=fbw*scale;
  if ((scale>0)&&(DRIVER->dstw>=perfectw-10)&&(DRIVER->dstw<=perfectw+20)) {
    DRIVER->dstw=perfectw;
    DRIVER->dsth=fbh*scale;
  }
  
  DRIVER->dstx=(driver->w>>1)-(DRIVER->dstw>>1);
  DRIVER->dsty=(driver->h>>1)-(DRIVER->dsth>>1);
  
  DRIVER->pvw=driver->w;
  DRIVER->pvh=driver->h;
}

/* Commit frame.
 */

static void _gl2_end(struct bigpc_render_driver *driver,uint8_t client_result) {
  if (!client_result) return;

  fmn_gl2_framebuffer_use_object(driver,0);
  fmn_gl2_require_output_bounds(driver);
  
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  fmn_gl2_texture_use_object(driver,&DRIVER->mainfb);
  
  fmn_gl2_draw_decal(
    driver,
    DRIVER->dstx,DRIVER->dsty,DRIVER->dstw,DRIVER->dsth,
    DRIVER->mainfb.border,DRIVER->mainfb.h-DRIVER->mainfb.border,
    DRIVER->mainfb.w-(DRIVER->mainfb.border<<1),
    -DRIVER->mainfb.h+(DRIVER->mainfb.border<<1)
  );
  
  // When the projected framebuffer is smaller than the screen, we letterbox or pillarbox.
  // It's more efficient to draw the bars on top, rather than the perhaps more obvious clear output first.
  if (DRIVER->dstw<driver->w) {
    fmn_gl2_program_use(driver,&DRIVER->program_raw);
    fmn_gl2_draw_raw_rect(0,0,DRIVER->dstx,driver->h,0x000000ff);
    fmn_gl2_draw_raw_rect(DRIVER->dstx+DRIVER->dstw,0,driver->w-DRIVER->dstw-DRIVER->dstx,driver->h,0x000000ff);
  }
  if (DRIVER->dsth<driver->h) {
    fmn_gl2_program_use(driver,&DRIVER->program_raw);
    fmn_gl2_draw_raw_rect(0,0,driver->w,DRIVER->dsty,0x000000ff);
    fmn_gl2_draw_raw_rect(0,DRIVER->dsty+DRIVER->dsth,driver->w,driver->h-DRIVER->dsth-DRIVER->dsty,0x000000ff);
  }
}

/* Initialize per client.
 */
 
static int8_t _gl2_video_init(
  struct bigpc_render_driver *driver,
  int16_t wmin,int16_t wmax,
  int16_t hmin,int16_t hmax,
  uint8_t pixfmt
) {

  // Prefer (320,192) if in range, next take the nearest in range.
  int fbw,fbh;
  if (wmin>320) fbw=wmin;
  else if (wmax<320) fbw=wmax;
  else fbw=320;
  if (hmin>192) fbh=hmin;
  else if (hmax<192) fbh=hmax;
  else fbh=192;
  const int fbborder=16;
  int err=fmn_gl2_texture_init_rgb(&DRIVER->mainfb,fbw+(fbborder<<1),fbh+(fbborder<<1),0);
  if (err<0) return err;
  DRIVER->mainfb.border=fbborder;
  
  // Force reconsideration of output bounds.
  DRIVER->pvw=DRIVER->pvh=0;
  
  // RGBA only.
  switch (pixfmt) {
    case FMN_VIDEO_PIXFMT_ANY:
    case FMN_VIDEO_PIXFMT_ANY_32:
    case FMN_VIDEO_PIXFMT_RGBA: {
        DRIVER->pixfmt=FMN_VIDEO_PIXFMT_RGBA;
      } break;
    default: return -1;
  }
  
  return 0;
}

/* Trivial accessors.
 */

static void _gl2_video_get_framebuffer_size(int16_t *w,int16_t *h,struct bigpc_render_driver *driver) {
  *w=DRIVER->mainfb.w-(DRIVER->mainfb.border<<1);
  *h=DRIVER->mainfb.h-(DRIVER->mainfb.border<<1);
}

static uint8_t _gl2_video_get_pixfmt(struct bigpc_render_driver *driver) {
  return DRIVER->pixfmt;
}

static uint32_t _gl2_video_rgba_from_pixel(struct bigpc_render_driver *driver,uint32_t pixel) { return pixel; }
static uint32_t _gl2_video_pixel_from_rgba(struct bigpc_render_driver *driver,uint32_t rgba) { return rgba; }

/* Init image.
 */
 
static void _gl2_video_init_image(struct bigpc_render_driver *driver,uint16_t imageid,int16_t w,int16_t h) {
  struct fmn_gl2_texture *texture=fmn_gl2_get_texture(driver,imageid,1);
  if (!texture) return;
  fmn_gl2_texture_init_rgba(texture,w,h,0);
}

/* Get image size.
 */
 
static void _gl2_video_get_image_size(int16_t *w,int16_t *h,struct bigpc_render_driver *driver,uint16_t imageid) {
  if ((fmn_gl2_texture_use_imageid(driver,imageid)<0)||!DRIVER->texture) { *w=*h=0; return; }
  *w=DRIVER->texture->w;
  *h=DRIVER->texture->h;
}

/* Set output image.
 */
 
static int8_t _gl2_draw_set_output(struct bigpc_render_driver *driver,uint16_t imageid) {
  return fmn_gl2_framebuffer_use_imageid(driver,imageid);
}

/* Render ops.
 */
 
static void _gl2_draw_clear(struct bigpc_render_driver *driver) {
  glClearColor(0.0f,0.0f,0.0f,0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}
 
static void _gl2_draw_line(struct bigpc_render_driver *driver,const struct fmn_draw_line *v,int c) {
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  int vtxc=c<<1;
  if (fmn_gl2_rawvtxv_require(driver,vtxc)<0) return;
  struct fmn_gl2_vertex_raw *dst=DRIVER->rawvtxv;
  int i=c; for (;i-->0;dst+=2,v++) {
    dst[0].x=v->ax;
    dst[0].y=v->ay;
    dst[1].x=v->bx;
    dst[1].y=v->by;
    dst[0].r=dst[1].r=v->pixel>>24;
    dst[0].g=dst[1].g=v->pixel>>16;
    dst[0].b=dst[1].b=v->pixel>>8;
    dst[0].a=dst[1].a=v->pixel;
  }
  fmn_gl2_draw_raw(GL_LINES,DRIVER->rawvtxv,vtxc);
}

static void _gl2_draw_rect(struct bigpc_render_driver *driver,const struct fmn_draw_rect *v,int c) {
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  for (;c-->0;v++) {
    fmn_gl2_draw_raw_rect(v->x,v->y,v->w,v->h,v->pixel);
  }
}

static void _gl2_draw_mintile(struct bigpc_render_driver *driver,const struct fmn_draw_mintile *v,int c,uint16_t srcimageid) {
  if (fmn_gl2_texture_use_imageid(driver,srcimageid)<0) return;
  fmn_gl2_program_use(driver,&DRIVER->program_mintile);
  fmn_gl2_draw_mintile(v,c);
}

static void _gl2_draw_maxtile(struct bigpc_render_driver *driver,const struct fmn_draw_maxtile *v,int c,uint16_t srcimageid) {
  if (fmn_gl2_texture_use_imageid(driver,srcimageid)<0) return;
  fmn_gl2_program_use(driver,&DRIVER->program_maxtile);
  fmn_gl2_draw_maxtile(v,c);
}

// TODO Consider whether it's feasible or worthwhile to batch decal/recal calls.
// As it stands, our shader is built to run one at a time.

static void _gl2_draw_decal(struct bigpc_render_driver *driver,const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {
  if (srcimageid) {
    if (fmn_gl2_texture_use_imageid(driver,srcimageid)<0) return;
  } else {
    if (fmn_gl2_texture_use_object(driver,&DRIVER->mainfb)<0) return;
  }
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  for (;c-->0;v++) {
    fmn_gl2_draw_decal(driver,
      v->dstx,v->dsty,v->dstw,v->dsth,
      v->srcx+DRIVER->texture->border,v->srcy+DRIVER->texture->border,
      v->srcw,v->srch
    );
  }
}

static void _gl2_draw_decal_swap(struct bigpc_render_driver *driver,const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {
  if (srcimageid) {
    if (fmn_gl2_texture_use_imageid(driver,srcimageid)<0) return;
  } else {
    if (fmn_gl2_texture_use_object(driver,&DRIVER->mainfb)<0) return;
  }
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  for (;c-->0;v++) {
    fmn_gl2_draw_decal_swap(driver,
      v->dstx,v->dsty,v->dstw,v->dsth,
      v->srcx+DRIVER->texture->border,v->srcy+DRIVER->texture->border,
      v->srcw,v->srch
    );
  }
}

static void _gl2_draw_recal(struct bigpc_render_driver *driver,const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {
  if (srcimageid) {
    if (fmn_gl2_texture_use_imageid(driver,srcimageid)<0) return;
  } else {
    if (fmn_gl2_texture_use_object(driver,&DRIVER->mainfb)<0) return;
  }
  fmn_gl2_program_use(driver,&DRIVER->program_recal);
  for (;c-->0;v++) {
    fmn_gl2_draw_recal(
      driver,
      &DRIVER->program_recal,
      v->dstx,v->dsty,v->dstw,v->dsth,
      v->srcx+DRIVER->texture->border,v->srcy+DRIVER->texture->border,
      v->srcw,v->srch,
      v->pixel
    );
  }
}

static void _gl2_draw_recal_swap(struct bigpc_render_driver *driver,const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {
  if (srcimageid) {
    if (fmn_gl2_texture_use_imageid(driver,srcimageid)<0) return;
  } else {
    if (fmn_gl2_texture_use_object(driver,&DRIVER->mainfb)<0) return;
  }
  fmn_gl2_program_use(driver,&DRIVER->program_recal);
  for (;c-->0;v++) {
    fmn_gl2_draw_recal_swap(
      driver,
      &DRIVER->program_recal,
      v->dstx,v->dsty,v->dstw,v->dsth,
      v->srcx+DRIVER->texture->border,v->srcy+DRIVER->texture->border,
      v->srcw,v->srch,
      v->pixel
    );
  }
}

static void _gl2_begin(struct bigpc_render_driver *driver,struct bigpc_image *always_null) {
  fmn_gl2_framebuffer_use_object(driver,&DRIVER->mainfb);
}

/* Read framebuffer.
 */
 
static int _gl2_read_framebuffer(void *dstpp,int *w,int *h,struct bigpc_render_driver *driver) {
  //TODO This is going to include the border. If we're actually using this, we'll want to crop that off.
  #if FMN_USE_bcm
    // glGetTexImage not found on the Pi. This is only for taking screencaps, so not bothering to investigate.
    return -1;
  #else
  fmn_gl2_texture_use_object(driver,&DRIVER->mainfb);
  void *v=malloc(DRIVER->mainfb.w*DRIVER->mainfb.h*4);
  if (!v) return -1;
  glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,v);
  *(void**)dstpp=v;
  *w=DRIVER->mainfb.w;
  *h=DRIVER->mainfb.h;
  
  // But of course, GL is all backward...
  int rowlen=DRIVER->mainfb.w<<2;
  void *rowbuf=malloc(rowlen);
  if (rowbuf) {
    uint8_t *a=v;
    uint8_t *b=a+rowlen*(DRIVER->mainfb.h-1);
    int c=DRIVER->mainfb.h>>1;
    for (;c-->0;a+=rowlen,b-=rowlen) {
      memcpy(rowbuf,a,rowlen);
      memcpy(a,b,rowlen);
      memcpy(b,rowbuf,rowlen);
    }
    free(rowbuf);
  }
  #endif
  return 0;
}

/* Set scaler.
 */
 
static void _gl2_set_scaler(struct bigpc_render_driver *driver,int scaler) {
  switch (scaler) {
    case FMN_SCALER_PIXELLY: fmn_gl2_set_main_filter(driver,GL_NEAREST); break;
    case FMN_SCALER_BLURRY: fmn_gl2_set_main_filter(driver,GL_LINEAR); break;
  }
}

/* Type.
 */
 
const struct bigpc_render_type bigpc_render_type_gl2={
  .name="gl2",
  .desc="OpenGL 2+. Recommended.",
  .objlen=sizeof(struct bigpc_render_driver_gl2),
  .video_renderer_id=BIGPC_RENDERER_opengl2,
  .del=_gl2_del,
  .init=_gl2_init,
  .read_framebuffer=_gl2_read_framebuffer,
  
  .video_init=_gl2_video_init,
  .video_get_framebuffer_size=_gl2_video_get_framebuffer_size,
  .video_get_pixfmt=_gl2_video_get_pixfmt,
  .video_rgba_from_pixel=_gl2_video_rgba_from_pixel,
  .video_pixel_from_rgba=_gl2_video_pixel_from_rgba,
  .video_init_image=_gl2_video_init_image,
  .video_get_image_size=_gl2_video_get_image_size,
  .draw_set_output=_gl2_draw_set_output,
  .draw_clear=_gl2_draw_clear,
  .draw_line=_gl2_draw_line,
  .draw_rect=_gl2_draw_rect,
  .draw_mintile=_gl2_draw_mintile,
  .draw_maxtile=_gl2_draw_maxtile,
  .draw_decal=_gl2_draw_decal,
  .draw_decal_swap=_gl2_draw_decal_swap,
  .draw_recal=_gl2_draw_recal,
  .draw_recal_swap=_gl2_draw_recal_swap,
  .begin=_gl2_begin,
  .end=_gl2_end,
  .set_scaler=_gl2_set_scaler,
};

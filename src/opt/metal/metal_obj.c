#include "metal_internal.h"

/* Delete.
 */
 
static void _metal_del(struct bigpc_render_driver *driver) {
}

/* Init.
 */
 
static int _metal_init(struct bigpc_render_driver *driver,struct bigpc_video_driver *video) {
  if (!(DRIVER->macwm=bigpc_video_get_macwm(video))) return -1;
  return 0;
}

/* Prepare video.
 */
  
static int8_t _metal_video_init(
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
  //int err=fmn_gl2_texture_init_rgb(&DRIVER->mainfb,fbw,fbh,0);
  //if (err<0) return err;
  DRIVER->fbw=fbw;
  DRIVER->fbh=fbh;
  
  // Force reconsideration of output bounds.
  DRIVER->pvw=DRIVER->pvh=0;
  
  // RGBA only. (copied from gl2. What is Metal's opinion on pixel formats?)
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

/* Trivial hooks.
 */
 
static void _metal_video_get_framebuffer_size(int16_t *w,int16_t *h,struct bigpc_render_driver *driver) {
  *w=DRIVER->fbw;
  *h=DRIVER->fbh;
}

uint8_t _metal_video_get_pixfmt(struct bigpc_render_driver *driver) {
  return DRIVER->pixfmt;
}

uint32_t _metal_video_rgba_from_pixel(struct bigpc_render_driver *driver,uint32_t pixel) {
  return pixel;
}

uint32_t _metal_video_pixel_from_rgba(struct bigpc_render_driver *driver,uint32_t rgba) {
  return rgba;
}

/* Images.
 */

void _metal_video_init_image(struct bigpc_render_driver *driver,uint16_t imageid,int16_t w,int16_t h) {
}

void _metal_video_get_image_size(int16_t *w,int16_t *h,struct bigpc_render_driver *driver,uint16_t imageid) {
}

int8_t _metal_draw_set_output(struct bigpc_render_driver *driver,uint16_t imageid) {
  return 0;
}

/* Render.
 */

void _metal_draw_line(struct bigpc_render_driver *driver,const struct fmn_draw_line *v,int c) {
}

void _metal_draw_rect(struct bigpc_render_driver *driver,const struct fmn_draw_rect *v,int c) {
}

void _metal_draw_mintile(struct bigpc_render_driver *driver,const struct fmn_draw_mintile *v,int c,uint16_t srcimageid) {
}

void _metal_draw_maxtile(struct bigpc_render_driver *driver,const struct fmn_draw_maxtile *v,int c,uint16_t srcimageid) {
}

void _metal_draw_decal(struct bigpc_render_driver *driver,const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {
}

void _metal_draw_decal_swap(struct bigpc_render_driver *driver,const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {
}

void _metal_draw_recal(struct bigpc_render_driver *driver,const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {
}

void _metal_draw_recal_swap(struct bigpc_render_driver *driver,const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {
}

/* Frame fencing.
 */
  
static void _metal_begin(struct bigpc_render_driver *driver) {
}

static void _metal_end(struct bigpc_render_driver *driver,uint8_t client_result) {
}

/* Type definition.
 */
 
const struct bigpc_render_type bigpc_render_type_metal={
  .name="metal",
  .desc="Video via Metal for MacOS. Recommended for Macs after 2013 or so.",
  .objlen=sizeof(struct bigpc_render_driver_metal),
  .del=_metal_del,
  .init=_metal_init,
  .video_init=_metal_video_init,
  .video_get_framebuffer_size=_metal_video_get_framebuffer_size,
  .video_get_pixfmt=_metal_video_get_pixfmt,
  .video_rgba_from_pixel=_metal_video_rgba_from_pixel,
  .video_pixel_from_rgba=_metal_video_pixel_from_rgba,
  .video_init_image=_metal_video_init_image,
  .video_get_image_size=_metal_video_get_image_size,
  .draw_set_output=_metal_draw_set_output,
  .draw_line=_metal_draw_line,
  .draw_rect=_metal_draw_rect,
  .draw_mintile=_metal_draw_mintile,
  .draw_maxtile=_metal_draw_maxtile,
  .draw_decal=_metal_draw_decal,
  .draw_decal_swap=_metal_draw_decal_swap,
  .draw_recal=_metal_draw_recal,
  .draw_recal_swap=_metal_draw_recal_swap,
  .begin=_metal_begin,
  .end=_metal_end,
};

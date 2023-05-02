/* bigpc_video_api.c
 * These are entry points from client into the bigpc platform.
 * Nothing interesting here.
 * Our render interface is deliberately designed to match this API exactly, and we pass right thru.
 */

#include "bigpc_internal.h"

int8_t fmn_video_init(
  int16_t wmin,int16_t wmax,
  int16_t hmin,int16_t hmax,
  uint8_t pixfmt
) {
  return bigpc.render->type->video_init(bigpc.render,wmin,wmax,hmin,hmax,pixfmt);
}

void fmn_video_get_framebuffer_size(int16_t *w,int16_t *h) {
  return bigpc.render->type->video_get_framebuffer_size(w,h,bigpc.render);
}

uint8_t fmn_video_get_pixfmt() {
  return bigpc.render->type->video_get_pixfmt(bigpc.render);
}

uint32_t fmn_video_rgba_from_pixel(uint32_t pixel) {
  return bigpc.render->type->video_rgba_from_pixel(bigpc.render,pixel);
}

uint32_t fmn_video_pixel_from_rgba(uint32_t rgba) {
  return bigpc.render->type->video_pixel_from_rgba(bigpc.render,rgba);
}

void fmn_video_upload_image(
  uint16_t imageid,
  int16_t x,int16_t y,int16_t w,int16_t h,
  const void *src,int srcstride,uint8_t srcpixfmt
) {
  bigpc.render->type->video_upload_image(bigpc.render,imageid,x,y,w,h,src,srcstride,srcpixfmt);
}

void fmn_video_init_image(uint16_t imageid,int16_t w,int16_t h) {
  bigpc.render->type->video_init_image(bigpc.render,imageid,w,h);
}

void fmn_video_get_image_size(int16_t *w,int16_t *h,uint16_t imageid) {
  bigpc.render->type->video_get_image_size(w,h,bigpc.render,imageid);
}

int8_t fmn_draw_set_output(uint16_t imageid) {
  return bigpc.render->type->draw_set_output(bigpc.render,imageid);
}

void fmn_draw_line(const struct fmn_draw_line *v,int c) {
  bigpc.render->type->draw_line(bigpc.render,v,c);
}

void fmn_draw_rect(const struct fmn_draw_rect *v,int c) {
  bigpc.render->type->draw_rect(bigpc.render,v,c);
}

void fmn_draw_mintile(const struct fmn_draw_mintile *v,int c,uint16_t srcimageid) {
  bigpc.render->type->draw_mintile(bigpc.render,v,c,srcimageid);
}

void fmn_draw_maxtile(const struct fmn_draw_maxtile *v,int c,uint16_t srcimageid) {
  bigpc.render->type->draw_maxtile(bigpc.render,v,c,srcimageid);
}

void fmn_draw_decal(const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {
  bigpc.render->type->draw_decal(bigpc.render,v,c,srcimageid);
}

void fmn_draw_decal_swap(const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {
  bigpc.render->type->draw_decal_swap(bigpc.render,v,c,srcimageid);
}

void fmn_draw_recal(const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {
  bigpc.render->type->draw_recal(bigpc.render,v,c,srcimageid);
}

void fmn_draw_recal_swap(const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {
  bigpc.render->type->draw_recal_swap(bigpc.render,v,c,srcimageid);
}

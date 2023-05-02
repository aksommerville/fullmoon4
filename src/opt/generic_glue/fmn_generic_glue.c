#include "app/fmn_platform.h"

int main(int argc,char **argv) {
  return 0;
}

void fmn_log(const char *fmt,...) {
}

void fmn_abort() {
}

void _fmn_begin_menu(int prompt,.../*int opt1,void (*cb1)(),...,int optN,void (*cbN)()*/) {
}

int8_t fmn_load_map(
  uint16_t mapid,
  void (*cb_spawn)(
    int8_t x,int8_t y,
    uint16_t spriteid,uint8_t arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3,
    const uint8_t *cmdv,uint16_t cmdc
  )
) {
  return 0;
}

int8_t fmn_add_plant(uint16_t x,uint16_t y) {
  return 0;
}

uint32_t fmn_begin_sketch(uint16_t x,uint16_t y) {
  return 0;
}

void fmn_update_sketch(uint16_t x,uint16_t y,uint32_t bits) {
}

void fmn_sound_effect(uint16_t sfxid) {
}

void fmn_synth_event(uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
}

void fmn_play_song(uint8_t songid) {
}

uint8_t fmn_get_string(char *dst,uint8_t dsta,uint16_t id) {
  return 0;
}

uint8_t fmn_find_map_command(int16_t *xy,uint8_t mask,const uint8_t *v) {
  return 0;
}

uint8_t fmn_find_direction_to_item(uint8_t itemid) {
  return 0;
}

uint8_t fmn_find_direction_to_map(uint16_t mapid) {
  return 0;
}

void fmn_map_callbacks(uint8_t evid,void (*cb)(uint16_t cbid,uint8_t param,void *userdata),void *userdata) {
}

void fmn_log_event(const char *key,const char *fmt,...) {
}

/* Render API.
 **********************************************/
 
static struct {
  int16_t w,h;
  uint8_t pixfmt;
} fmn_generic_video={0};
 
int8_t fmn_video_init(
  int16_t wmin,int16_t wmax,
  int16_t hmin,int16_t hmax,
  uint8_t pixfmt
) {
  if (wmin>320) fmn_generic_video.w=wmin;
  else if (wmax<320) fmn_generic_video.w=wmax;
  else fmn_generic_video.w=320;
  if (hmin>192) fmn_generic_video.h=hmin;
  else if (hmax<192) fmn_generic_video.h=hmax;
  else fmn_generic_video.h=192;
  switch (pixfmt) {
    case FMN_VIDEO_PIXFMT_ANY: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_RGBA; break;
    case FMN_VIDEO_PIXFMT_ANY_1: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_Y1BE; break;
    case FMN_VIDEO_PIXFMT_Y1BE: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_Y1BE; break;
    case FMN_VIDEO_PIXFMT_W1BE: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_W1BE; break;
    case FMN_VIDEO_PIXFMT_ANY_2: return -1;
    case FMN_VIDEO_PIXFMT_ANY_4: return -1;
    case FMN_VIDEO_PIXFMT_ANY_8: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_Y8; break;
    case FMN_VIDEO_PIXFMT_I8: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_I8; break;
    case FMN_VIDEO_PIXFMT_Y8: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_Y8; break;
    case FMN_VIDEO_PIXFMT_ANY_16: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_RGB565LE; break;
    case FMN_VIDEO_PIXFMT_RGB565LE: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_RGB565LE; break;
    case FMN_VIDEO_PIXFMT_RGBA4444BE: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_RGBA4444BE; break;
    case FMN_VIDEO_PIXFMT_ANY_24: return -1;
    case FMN_VIDEO_PIXFMT_ANY_32: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_RGBA; break;
    case FMN_VIDEO_PIXFMT_RGBA: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_RGBA; break;
    case FMN_VIDEO_PIXFMT_BGRA: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_BGRA; break;
    case FMN_VIDEO_PIXFMT_ARGB: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_ARGB; break;
    case FMN_VIDEO_PIXFMT_ABGR: fmn_generic_video.pixfmt=FMN_VIDEO_PIXFMT_ABGR; break;
    default: return -1;
  }
  return 0;
}

void fmn_video_get_framebuffer_size(int16_t *w,int16_t *h) {
  *w=fmn_generic_video.w;
  *h=fmn_generic_video.h;
}
uint8_t fmn_video_get_pixfmt() { return fmn_generic_video.pixfmt; }

uint32_t fmn_video_rgba_from_pixel(uint32_t pixel) { return pixel; }
uint32_t fmn_video_pixel_from_rgba(uint32_t rgba) { return rgba; }
void fmn_video_get_image_size(int16_t *w,int16_t *h,uint16_t imageid) { *w=*h=0; }
void fmn_video_upload_image(
  uint16_t imageid,
  int16_t x,int16_t y,int16_t w,int16_t h,
  const void *src,int srcstride,uint8_t srcpixfmt
) {}
void fmn_video_init_image(uint16_t imageid,int16_t w,int16_t h) {}
int8_t fmn_draw_set_output(uint16_t imageid) { return 0; }

void fmn_draw_line(const struct fmn_draw_line *v,int c) {}
void fmn_draw_rect(const struct fmn_draw_rect *v,int c) {}
void fmn_draw_mintile(const struct fmn_draw_mintile *v,int c,uint16_t srcimageid) {}
void fmn_draw_maxtile(const struct fmn_draw_maxtile *v,int c,uint16_t srcimageid) {}
void fmn_draw_decal(const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {}
void fmn_draw_decal_swap(const struct fmn_draw_decal *v,int c,uint16_t srcimageid) {}
void fmn_draw_recal(const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {}
void fmn_draw_recal_swap(const struct fmn_draw_recal *v,int c,uint16_t srcimageid) {}

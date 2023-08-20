/* bigpc_render.h
 * Pluggable rendering units.
 * I expect there will be two implementations: Software and OpenGL 2.
 * Maybe we'll do OpenGL 1? Metal? Vulkan? And maybe specialized software renderers for low-performance systems.
 */
 
#ifndef BIGPC_RENDER_H
#define BIGPC_RENDER_H

#include <stdint.h>
#include "app/fmn_platform.h"

struct bigpc_render_driver;
struct bigpc_render_type;
struct bigpc_video_driver;
struct bigpc_image;
struct fmn_datafile;

/* Instance.
 *************************************************************/
 
struct bigpc_render_driver {
  const struct bigpc_render_type *type;
  int refc;
  int w,h; // Provider should update before each render.
  struct fmn_datafile *datafile; // WEAK
  int transition_in_progress; // bigpc makes the call whether to suspend during transition; render has to declare whether one is happening.
};

void bigpc_render_del(struct bigpc_render_driver *driver);
int bigpc_render_ref(struct bigpc_render_driver *driver);

struct bigpc_render_driver *bigpc_render_new(
  const struct bigpc_render_type *type,
  struct bigpc_video_driver *video
);

/* Type.
 ********************************************************/
 
struct bigpc_render_type {
  const char *name;
  const char *desc;
  int objlen;
  int appointment_only;
  int video_renderer_id; // BIGPC_RENDERER_*
  void (*del)(struct bigpc_render_driver *driver);
  int (*init)(struct bigpc_render_driver *driver,struct bigpc_video_driver *video);
  
  /* Optional, for screencaps.
   * On success, driver produces packed RGBA in a new buffer at (*dstpp), caller frees it.
   */
  int (*read_framebuffer)(void *dstpp,int *w,int *h,struct bigpc_render_driver *driver);
  
  // New heavy-client video API.
  // To reduce overhead, all types must implement all of these, and there are no wrapper functions.
  int8_t (*video_init)(
    struct bigpc_render_driver *driver,
    int16_t wmin,int16_t wmax,
    int16_t hmin,int16_t hmax,
    uint8_t pixfmt
  );
  void (*video_get_framebuffer_size)(int16_t *w,int16_t *h,struct bigpc_render_driver *driver);
  uint8_t (*video_get_pixfmt)(struct bigpc_render_driver *driver);
  uint32_t (*video_rgba_from_pixel)(struct bigpc_render_driver *driver,uint32_t pixel);
  uint32_t (*video_pixel_from_rgba)(struct bigpc_render_driver *driver,uint32_t rgba);
  void (*video_init_image)(struct bigpc_render_driver *driver,uint16_t imageid,int16_t w,int16_t h);
  void (*video_get_image_size)(int16_t *w,int16_t *h,struct bigpc_render_driver *driver,uint16_t imageid);
  int8_t (*draw_set_output)(struct bigpc_render_driver *driver,uint16_t imageid);
  void (*draw_clear)(struct bigpc_render_driver *driver);
  void (*draw_line)(struct bigpc_render_driver *driver,const struct fmn_draw_line *v,int c);
  void (*draw_rect)(struct bigpc_render_driver *driver,const struct fmn_draw_rect *v,int c);
  void (*draw_mintile)(struct bigpc_render_driver *driver,const struct fmn_draw_mintile *v,int c,uint16_t srcimageid);
  void (*draw_maxtile)(struct bigpc_render_driver *driver,const struct fmn_draw_maxtile *v,int c,uint16_t srcimageid);
  void (*draw_decal)(struct bigpc_render_driver *driver,const struct fmn_draw_decal *v,int c,uint16_t srcimageid);
  void (*draw_decal_swap)(struct bigpc_render_driver *driver,const struct fmn_draw_decal *v,int c,uint16_t srcimageid);
  void (*draw_recal)(struct bigpc_render_driver *driver,const struct fmn_draw_recal *v,int c,uint16_t srcimageid);
  void (*draw_recal_swap)(struct bigpc_render_driver *driver,const struct fmn_draw_recal *v,int c,uint16_t srcimageid);
  
  // Extra fencing before and after fmn_render().
  // For begin, (fb) will be null in GX cases or the main framebuffer in soft-render cases.
  void (*begin)(struct bigpc_render_driver *driver,struct bigpc_image *fb);
  void (*end)(struct bigpc_render_driver *driver,uint8_t client_result);
  
  void (*set_scaler)(struct bigpc_render_driver *driver,int scaler);
};

const struct bigpc_render_type *bigpc_render_type_by_index(int p);
const struct bigpc_render_type *bigpc_render_type_by_name(const char *name,int namec);

#endif

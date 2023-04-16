#include "macwm.h"
#include "opt/bigpc/bigpc_video.h"

/* Instance definition.
 */

struct bigpc_video_driver_macwm {
  struct bigpc_video_driver hdr;
  struct macwm *macwm;
};

#define DRIVER ((struct bigpc_video_driver_macwm*)driver)

/* Cleanup.
 */

static void _macwm_del(struct bigpc_video_driver *driver) {
  macwm_del(DRIVER->macwm);
}

/* Callback wrappers.
 */

static void _macwm_cb_resize(void *userdata,int w,int h) {
  struct bigpc_video_driver *driver=userdata;
  driver->w=w;
  driver->h=h;
  if (driver->delegate.cb_resize) driver->delegate.cb_resize(driver,w,h);
}

/* Init.
 */

static int _macwm_init(struct bigpc_video_driver *driver,const struct bigpc_video_config *config) {

  // They have to ask for opengl2, or some superset of it.
  switch (config->renderer) {
    case BIGPC_RENDERER_any:
    case BIGPC_RENDERER_gx:
    case BIGPC_RENDERER_opengl2:
      break;
    default: return -1;
  }

  // Delegate hooks are compatible between macwm and bigpc (only there's no "focus" in macwm).
  // We have to intercept resize, in order to retain its value.
  struct macwm_delegate delegate={
    .userdata=driver,
    .close=(void*)driver->delegate.cb_close,
    .resize=_macwm_cb_resize,
    .key=(void*)driver->delegate.cb_key,
    .text=(void*)driver->delegate.cb_text,
    .mbutton=(void*)driver->delegate.cb_mbutton,
    .mmotion=(void*)driver->delegate.cb_mmotion,
    .mwheel=(void*)driver->delegate.cb_mwheel,
  };
  struct macwm_setup setup={
    .w=config->w,
    .h=config->w,
    .fullscreen=config->fullscreen,
    .title=config->title,
    .rendermode=MACWM_RENDERMODE_OPENGL,//TODO Soft and Metal would be nice too.
    .fbw=config->fbw,
    .fbh=config->fbh,
  };

  if (!(DRIVER->macwm=macwm_new(&delegate,&setup))) return -1;

  driver->renderer=BIGPC_RENDERER_opengl2;
  macwm_get_size(&driver->w,&driver->h,DRIVER->macwm);
  driver->fullscreen=macwm_get_fullscreen(DRIVER->macwm);
  
  return 0;
}

/* Update.
 */

static int _macwm_update(struct bigpc_video_driver *driver) {
  return macwm_update(DRIVER->macwm);
}

/* Render framing.
 * TODO permit soft render?
 */

static int _macwm_begin_gx(struct bigpc_video_driver *driver) {
  return macwm_render_begin(DRIVER->macwm);
}

static void _macwm_end(struct bigpc_video_driver *driver) {
  macwm_render_end(DRIVER->macwm);
}

static void _macwm_cancel(struct bigpc_video_driver *driver) {
  macwm_render_end(DRIVER->macwm);
}

/* Cursor, fullscreen.
 */

static void _macwm_show_cursor(struct bigpc_video_driver *driver,int show) {
  //TODO macwm has no public api for cursor yet. i kind of doubt that we'll want it anyway
}

static void _macwm_set_fullscreen(struct bigpc_video_driver *driver,int fullscreen) {
  macwm_set_fullscreen(DRIVER->macwm,fullscreen);
  driver->fullscreen=macwm_get_fullscreen(DRIVER->macwm);
}

/* Type definition.
 */

const struct bigpc_video_type bigpc_video_type_macwm={
  .name="macwm",
  .desc="Window Manager interface for MacOS 10+",
  .objlen=sizeof(struct bigpc_video_driver_macwm),
  .has_wm=1,
  .del=_macwm_del,
  .init=_macwm_init,
  .update=_macwm_update,
  .begin_gx=_macwm_begin_gx,
  .end=_macwm_end,
  .cancel=_macwm_cancel,
  .show_cursor=_macwm_show_cursor,
  .set_fullscreen=_macwm_set_fullscreen,
};

#include "macwm.h"
#include "opt/bigpc/bigpc_video.h"
#include "opt/bigpc/bigpc_image.h"

/* Instance definition.
 */

struct bigpc_video_driver_macwm {
  struct bigpc_video_driver hdr;
  struct macwm *macwm;
  struct bigpc_image *fb; // soft render only
  int pixfmt;
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

  // They have to ask for opengl2, metal, or soft.
  int rendermode;
  switch (config->renderer) {
    case BIGPC_RENDERER_any:
    case BIGPC_RENDERER_gx: // TODO For "any" and "gx", can we detect whether Metal is supported, and use OpenGL if not?
    case BIGPC_RENDERER_metal: {
        rendermode=MACWM_RENDERMODE_METAL;
        driver->renderer=BIGPC_RENDERER_metal;
      } break;
    case BIGPC_RENDERER_opengl2: {
        rendermode=MACWM_RENDERMODE_OPENGL;
        driver->renderer=BIGPC_RENDERER_opengl2;
      } break;
    case BIGPC_RENDERER_soft:
    case BIGPC_RENDERER_rgb32: {
        rendermode=MACWM_RENDERMODE_FRAMEBUFFER;
        driver->renderer=BIGPC_RENDERER_rgb32;
      } break;
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
    .rendermode=rendermode,
    .fbw=config->fbw,
    .fbh=config->fbh,
  };

  if (!(DRIVER->macwm=macwm_new(&delegate,&setup))) return -1;

  macwm_get_size(&driver->w,&driver->h,DRIVER->macwm);
  driver->fullscreen=macwm_get_fullscreen(DRIVER->macwm);
  driver->fbw=config->fbw;
  driver->fbh=config->fbh;
  
  if (driver->renderer==BIGPC_RENDERER_rgb32) {
    if (!(DRIVER->fb=bigpc_image_new_alloc(driver->fbw,driver->fbh,BIGPC_IMAGE_STORAGE_32,DRIVER->pixfmt))) return -1;
  }
  
  return 0;
}

/* Update.
 */

static int _macwm_update(struct bigpc_video_driver *driver) {
  return macwm_update(DRIVER->macwm);
}

/* Render framing.
 */
 
static struct bigpc_image *_macwm_begin_soft(struct bigpc_video_driver *driver) {
  return DRIVER->fb;
}

static int _macwm_begin_gx(struct bigpc_video_driver *driver) {
  if (DRIVER->fb) return -1;
  return macwm_render_begin(DRIVER->macwm);
}

static void _macwm_end(struct bigpc_video_driver *driver) {
  if (DRIVER->fb) {
    macwm_send_framebuffer(DRIVER->macwm,DRIVER->fb->v);
  } else {
    macwm_render_end(DRIVER->macwm);
  }
}

static void _macwm_cancel(struct bigpc_video_driver *driver) {
  if (DRIVER->fb) {
  } else {
    macwm_render_end(DRIVER->macwm);
  }
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
  .begin_soft=_macwm_begin_soft,
  .begin_gx=_macwm_begin_gx,
  .end=_macwm_end,
  .cancel=_macwm_cancel,
  .show_cursor=_macwm_show_cursor,
  .set_fullscreen=_macwm_set_fullscreen,
};

/* Private, extra support for renderer.
 */
 
struct macwm *bigpc_video_get_macwm(struct bigpc_video_driver *driver) {
  if (!driver||(driver->type!=&bigpc_video_type_macwm)) return 0;
  return DRIVER->macwm;
}

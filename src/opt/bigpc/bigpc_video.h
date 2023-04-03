/* bigpc_video.h
 * Generic interface for video drivers.
 * This is raw video output or access to an acceleration context, and also window manager events if available.
 */
 
#ifndef BIGPC_VIDEO_H
#define BIGPC_VIDEO_H

struct bigpc_video_driver;
struct bigpc_video_type;
struct bigpc_video_config;
struct bigpc_video_delegate;
struct bigpc_image;

/* Drivers should implement any renderer that they can do naturally.
 * Emulating one in terms of another is not the driver's problem; we'll do that at a higher level.
 * Drivers must be able to take the "any" wildcard.
 */
// Wildcard renderers. You can request these, but they will never be reported as the final choice:
#define BIGPC_RENDERER_any          0 /* Driver's choice. */
#define BIGPC_RENDERER_soft         1 /* Software rendering, any format. */
#define BIGPC_RENDERER_gx           2 /* Any kind of hardware acceleration. */
// Real renderers:
#define BIGPC_RENDERER_rgb32       10 /* Soft: 32-bit RGBX pixels, format not fully specified until you see a framebuffer. */
#define BIGPC_RENDERER_opengl1     40 /* GX: OpenGL 1.x */
#define BIGPC_RENDERER_opengl2     41 /* GX: OpenGL 2.x or greater */
#define BIGPC_RENDERER_metal       42 /* GX: Metal */
#define BIGPC_RENDERER_vulkan      43 /* GX: Vulkan */
#define BIGPC_RENDERER_openvg      44 /* GX: OpenVG */
#define BIGPC_RENDERER_direct3d    45 /* GX: Direct3d */

#define BIGPC_RENDERER_FOR_EACH \
  _(any) \
  _(soft) \
  _(gx) \
  _(rgb32) \
  _(opengl1) \
  _(opengl2) \
  _(metal) \
  _(vulkan) \
  _(openvg) \
  _(direct3d)

/* Video drivers can be output-only, in which case no delegate hooks should ever get called.
 */
struct bigpc_video_delegate {
  void *userdata;
  void (*cb_close)(struct bigpc_video_driver *driver);
  void (*cb_focus)(struct bigpc_video_driver *driver,int focus);
  void (*cb_resize)(struct bigpc_video_driver *driver,int w,int h);
  
  /* (keycode) should be USB-HID, eg 0x00070004 is A.
   * Return nonzero to ackowledge, then driver should not process general text events.
   */
  int (*cb_key)(struct bigpc_video_driver *driver,int keycode,int value);
  
  void (*cb_text)(struct bigpc_video_driver *driver,int codepoint);
  void (*cb_mmotion)(struct bigpc_video_driver *driver,int x,int y);
  void (*cb_mbutton)(struct bigpc_video_driver *driver,int btnid,int value);
  void (*cb_mwheel)(struct bigpc_video_driver *driver,int dx,int dy);
};

struct bigpc_video_config {
  int fbw,fbh; // Your framebuffer size. We'll select a reasonable window size accordingly.
  int renderer; // BIGPC_RENDERER_*, can't change after init.
  int w,h; // Real output size. Usually best to leave unset.
  int fullscreen;
  const char *title;
  const void *iconrgba;
  int iconw,iconh;
};

/* Driver instance.
 ****************************************************/
 
struct bigpc_video_driver {
  const struct bigpc_video_type *type;
  struct bigpc_video_delegate delegate;
  int refc;
  int w,h; // Real output size, not your framebuffer.
  int fullscreen;
  int renderer;
};
 
void bigpc_video_driver_del(struct bigpc_video_driver *driver);

struct bigpc_video_driver *bigpc_video_driver_new(
  const struct bigpc_video_type *type,
  const struct bigpc_video_delegate *delegate,
  const struct bigpc_video_config *config
);

/* Pump the driver for updates.
 * No rendering during this call, just input.
 */
int bigpc_video_driver_update(struct bigpc_video_driver *driver);

/* Frame fences.
 * You must begin with 'begin_soft' or 'begin_gx', whichever is appropriate to the driver's renderer.
 * Then do all your rendering, then finish with 'end' or 'cancel'.
 */
struct bigpc_image *bigpc_video_driver_begin_soft(struct bigpc_video_driver *driver);
int bigpc_video_driver_begin_gx(struct bigpc_video_driver *driver);
void bigpc_video_driver_end(struct bigpc_video_driver *driver);
void bigpc_video_driver_cancel(struct bigpc_video_driver *driver);

// Cursor should be hidden by default.
void bigpc_video_show_cursor(struct bigpc_video_driver *driver,int show);

void bigpc_video_set_fullscreen(struct bigpc_video_driver *driver,int fullscreen);

/* Call when you receive input from outside the window manager.
 * X11 needs this; most systems don't.
 */
void bigpc_video_suppress_screensaver(struct bigpc_video_driver *driver);

/* Driver type.
 ***********************************************************/
 
struct bigpc_video_type {
  const char *name;
  const char *desc;
  int objlen;
  int appointment_only; // Nonzero to ignore unless specifically requested.
  int has_wm; // Nonzero if we provide input.
  void (*del)(struct bigpc_video_driver *driver);
  int (*init)(struct bigpc_video_driver *driver,const struct bigpc_video_config *config);
  int (*update)(struct bigpc_video_driver *driver);
  struct bigpc_image *(*begin_soft)(struct bigpc_video_driver *driver);
  int (*begin_gx)(struct bigpc_video_driver *driver);
  void (*end)(struct bigpc_video_driver *driver);
  void (*cancel)(struct bigpc_video_driver *driver);
  void (*show_cursor)(struct bigpc_video_driver *driver,int show);
  void (*set_fullscreen)(struct bigpc_video_driver *driver,int fullscreen);
  void (*suppress_screensaver)(struct bigpc_video_driver *driver);
};

const struct bigpc_video_type *bigpc_video_type_by_index(int p);
const struct bigpc_video_type *bigpc_video_type_by_name(const char *name,int namec);

#endif

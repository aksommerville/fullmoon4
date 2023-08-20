/* fmn_glx.h
 * X11 and GLX.
 * Link: -lX11 -lGL -lGLX
 * With FMN_USE_xinerama: -lXinerama
 * TODO: Add an optional flag to build with X11 only, no GLX.
 */
 
#ifndef FMN_GLX_H
#define FMN_GLX_H

struct fmn_glx;

#define FMN_GLX_VIDEO_MODE_AUTO    0
#define FMN_GLX_VIDEO_MODE_OPENGL  1 /* Expose an OpenGL context. */
#define FMN_GLX_VIDEO_MODE_FB_PURE 2 /* Expose a framebuffer and don't touch OpenGL. (scale up in software) */
#define FMN_GLX_VIDEO_MODE_FB_GX   3 /* Expose a framebuffer but use OpenGL to scale up. XXX? */

#define FMN_GLX_FBFMT_ANYTHING   0 /* Will normally be some 32-bit RGB variation. */
#define FMN_GLX_FBFMT_XRGB       1 /* The RGB formats are named big-endianly as if you read them as a 32-bit integer. */
#define FMN_GLX_FBFMT_XBGR       2 /* So on a little-endian host, XBGR means R is the first byte serially. */
#define FMN_GLX_FBFMT_RGBX       3
#define FMN_GLX_FBFMT_BGRX       4
#define FMN_GLX_FBFMT_BGR565LE   5
#define FMN_GLX_FBFMT_BGRX4444BE 6
#define FMN_GLX_FBFMT_BGR332     7
#define FMN_GLX_FBFMT_Y1         8
#define FMN_GLX_FBFMT_Y8         9

struct fmn_glx_delegate {
  void *userdata;
  
  void (*close)(void *userdata);
  void (*focus)(void *userdata,int focus);
  void (*resize)(void *userdata,int w,int h);
  
  int (*key)(void *userdata,int keycode,int value);
  void (*text)(void *userdata,int codepoint);
  
  // Leave null if you don't want these, then we don't have to ask X for them:
  void (*mmotion)(void *userdata,int x,int y);
  void (*mbutton)(void *userdata,int btnid,int value);
  void (*mwheel)(void *userdata,int dx,int dy);
};

struct fmn_glx_setup {
  const char *title;
  const void *iconrgba;
  int iconw,iconh;
  int w,h;
  int fbw,fbh;
  int fullscreen;
  int video_mode;
  int fbfmt;
  int scale_limit; // <1=unlimited. Only used by FMN_GLX_VIDEO_MODE_FB_PURE.
  const char *display;
};

void fmn_glx_del(struct fmn_glx *fmn_glx);

struct fmn_glx *fmn_glx_new(
  const struct fmn_glx_delegate *delegate,
  const struct fmn_glx_setup *setup
);

void *fmn_glx_get_userdata(const struct fmn_glx *fmn_glx);
void fmn_glx_get_size(int *w,int *h,const struct fmn_glx *fmn_glx);
void fmn_glx_get_fb_size(int *w,int *h,const struct fmn_glx *fmn_glx);
int fmn_glx_get_fullscreen(const struct fmn_glx *fmn_glx);
int fmn_glx_get_video_mode(const struct fmn_glx *fmn_glx);
int fmn_glx_get_fbfmt(const struct fmn_glx *fmn_glx);

int fmn_glx_update(struct fmn_glx *fmn_glx);

void fmn_glx_set_fullscreen(struct fmn_glx *fmn_glx,int state);
void fmn_glx_suppress_screensaver(struct fmn_glx *fmn_glx);

int fmn_glx_begin_gx(struct fmn_glx *fmn_glx);
void fmn_glx_end_gx(struct fmn_glx *fmn_glx);
void *fmn_glx_begin_fb(struct fmn_glx *fmn_glx);
void fmn_glx_end_fb(struct fmn_glx *fmn_glx,void *fb);

// Cursor is initially hidden.
void fmn_glx_show_cursor(struct fmn_glx *fmn_glx,int show);

void fmn_glx_coord_fb_from_win(int *x,int *y,const struct fmn_glx *fmn_glx);
void fmn_glx_coord_win_from_fb(int *x,int *y,const struct fmn_glx *fmn_glx);

// We take care of this for you, but just in case:
int fmn_glx_codepoint_from_keysym(int keysym);
int fmn_glx_usb_usage_from_keysym(int keysym);

// Currently all supported formats are stored LRTB, so you could get by with just stride.
int fmn_glx_fbfmt_measure_stride(int fbfmt,int w);
int fmn_glx_fbfmt_measure_buffer(int fbfmt,int w,int h);

int fmn_glx_video_mode_is_gx(int video_mode);
int fmn_glx_video_mode_is_fb(int video_mode);

void fmn_glx_set_final_filter(struct fmn_glx *glx,int linear_filter); // for FMN_GLX_VIDEO_MODE_FB_GX only

#endif

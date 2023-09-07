#include "mswm_internal.h"
#include "app/fmn_platform.h"

struct bigpc_video_driver *mswm_global_driver=0;

/* Delete.
 */

static void _mswm_del(struct bigpc_video_driver *driver) {
  
  if (driver==mswm_global_driver) mswm_global_driver=0;

  glDeleteTextures(1,&DRIVER->texid);

  if (DRIVER->appicon) DestroyIcon(DRIVER->appicon);

  if (DRIVER->hglrc) {
    wglMakeCurrent(DRIVER->hdc,0);
    wglDeleteContext(DRIVER->hglrc);
  }
  if (DRIVER->hdc) {
    DeleteDC(DRIVER->hdc);
  }
  if (DRIVER->hwnd) {
    DestroyWindow(DRIVER->hwnd);
  }

  if (DRIVER->wndclass_atom) {
    UnregisterClass(MSWM_WINDOW_CLASS_NAME,DRIVER->instance);
  }
  
  bigpc_image_del(DRIVER->fb);
}

/* Setup window class.
 */

static int mswm_populate_wndclass(struct bigpc_video_driver *driver) {
  DRIVER->wndclass.cbSize=sizeof(WNDCLASSEX);
  DRIVER->wndclass.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
  DRIVER->wndclass.lpfnWndProc=(WNDPROC)mswm_cb_msg;
  DRIVER->wndclass.hInstance=DRIVER->instance;
  DRIVER->wndclass.lpszClassName=MSWM_WINDOW_CLASS_NAME;
  return 0;
}

static int mswm_register_wndclass(struct bigpc_video_driver *driver) {
  DRIVER->wndclass_atom=RegisterClassEx(&DRIVER->wndclass);
  if (!DRIVER->wndclass_atom) {
    int errcode=GetLastError();
    fprintf(stderr,"RegisterClassEx(\"%s\"): Error %d\n",MSWM_WINDOW_CLASS_NAME,errcode);
    return -1;
  }
  return 0;
}

/* Init.
 */

static int _mswm_init(struct bigpc_video_driver *driver,const struct bigpc_video_config *config) {
  if (mswm_global_driver) return -1;
  mswm_global_driver=driver;
  
  DRIVER->translate_events=1;
  DRIVER->dstdirty=1;

  if (!(DRIVER->instance=GetModuleHandle(0))) return -1;

  if (mswm_populate_wndclass(driver)<0) return -1;
  if (mswm_register_wndclass(driver)<0) return -1;
  
  driver->renderer=BIGPC_RENDERER_rgb32;
  driver->fbw=320;
  driver->fbh=192;
  
  int w=0,h=0;
  if (config) {
    w=config->w;
    h=config->h;
  }
  if (!w&&!h) {
    w=640;
    h=384;
  } else {
    if (w<100) w=100;
    else if (w>2000) w=2000;
    if (h<100) h=100;
    else if (h>2000) h=2000;
  }

  /* CreateWindow() takes the outer bounds, including the frame.
   * (w,h) provided to us are the desired inner bounds, ie the part that we control.
   */
  RECT bounds={
    .left=0,
    .top=0,
    .right=w,
    .bottom=h,
  };
  AdjustWindowRect(&bounds,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,0);
  int outerw=bounds.right-bounds.left;
  int outerh=bounds.bottom-bounds.top;
  driver->w=w;
  driver->h=h;
  int x=CW_USEDEFAULT,y=0;
  
  if (!(DRIVER->fb=bigpc_image_new_alloc(driver->fbw,driver->fbh,BIGPC_IMAGE_STORAGE_32,BIGPC_IMAGE_PIXFMT_ABGR))) return -1;
  
  DRIVER->hwnd=CreateWindow(
    MSWM_WINDOW_CLASS_NAME,
    (config&&config->title)?config->title:"",
    (0&&config&&config->fullscreen)?(WS_POPUP):(WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS),
    x,y,outerw,outerh,
    0,0,DRIVER->instance,0
  );
  if (!DRIVER->hwnd) {
    fprintf(stderr,"Failed to create window.\n");
    return -1;
  }

  HANDLE hIcon=LoadImage(0,"icon.ico",IMAGE_ICON,0,0,LR_DEFAULTSIZE|LR_LOADFROMFILE);
  if (hIcon) {
    SendMessage(DRIVER->hwnd,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
    SendMessage(DRIVER->hwnd,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
    SendMessage(GetWindow(DRIVER->hwnd,GW_OWNER),WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
    SendMessage(GetWindow(DRIVER->hwnd,GW_OWNER),WM_SETICON,ICON_BIG,(LPARAM)hIcon);
  }

  if (0&&config&&config->fullscreen) {
    driver->fullscreen=1;
    ShowWindow(DRIVER->hwnd,SW_MAXIMIZE);
  } else {
    ShowWindow(DRIVER->hwnd,SW_SHOW);
  }
  UpdateWindow(DRIVER->hwnd);

  FlashWindow(DRIVER->hwnd,0);

  ShowCursor(0);
  return 0;
}

/* Recalculate output bounds if dirty.
 */

static void mswm_require_output_bounds(struct bigpc_video_driver *driver) {
  if (!DRIVER->dstdirty) return;
  DRIVER->dstdirty=0;
  if ((driver->w<1)||(driver->h<1)||(driver->fbw<1)||(driver->fbh<1)) {
    DRIVER->dstl=-1.0f;
    DRIVER->dstb=-1.0f;
    DRIVER->dstr=1.0f;
    DRIVER->dstt=1.0f;
    return;
  }
  int scale;
  int wforh=(driver->fbw*driver->h)/driver->fbh;
  if (wforh<=driver->w) {
    DRIVER->dstr=(GLfloat)wforh/(GLfloat)driver->w;
    DRIVER->dstt=1.0f;
    scale=wforh/driver->fbw;
  } else {
    int hforw=(driver->w*driver->fbh)/driver->fbw;
    DRIVER->dstr=1.0f;
    DRIVER->dstt=(GLfloat)hforw/(GLfloat)driver->h;
    scale=hforw/driver->fbh;
  }
  DRIVER->dstl=-DRIVER->dstr;
  DRIVER->dstb=-DRIVER->dstt;
  glBindTexture(GL_TEXTURE_2D,DRIVER->texid);
  if (scale<4) {
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  } else {
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  }
}

/* Frame control.
 */

static struct bigpc_image *_mswm_begin_soft(struct bigpc_video_driver *driver) {
  return DRIVER->fb;
}

static void _mswm_end(struct bigpc_video_driver *driver) {
  mswm_require_output_bounds(driver);
  glViewport(0,0,driver->w,driver->h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if ((DRIVER->dstl>-1.0f)||(DRIVER->dstb>-1.0f)) {
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D,DRIVER->texid);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,DRIVER->fb->w,DRIVER->fb->h,0,GL_RGBA,GL_UNSIGNED_BYTE,DRIVER->fb->v);
  glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2i(0,0); glVertex2f(DRIVER->dstl,DRIVER->dstt);
    glTexCoord2i(0,1); glVertex2f(DRIVER->dstl,DRIVER->dstb);
    glTexCoord2i(1,0); glVertex2f(DRIVER->dstr,DRIVER->dstt);
    glTexCoord2i(1,1); glVertex2f(DRIVER->dstr,DRIVER->dstb);
  glEnd();
  glFlush();
  SwapBuffers(DRIVER->hdc);
}

static void _mswm_cancel(struct bigpc_video_driver *driver) {
}

/* Cursor.
 */

static void _mswm_show_cursor(struct bigpc_video_driver *driver,int show) {
}

/* Fullscreen.
 */

static void mswm_enter_fullscreen(struct bigpc_video_driver *driver) {
  DRIVER->fsrestore.length=sizeof(WINDOWPLACEMENT);
  GetWindowPlacement(DRIVER->hwnd,&DRIVER->fsrestore);

  int fullscreenWidth=GetDeviceCaps(DRIVER->hdc,DESKTOPHORZRES);
  int fullscreenHeight=GetDeviceCaps(DRIVER->hdc,DESKTOPVERTRES);

  SetWindowLongPtr(DRIVER->hwnd,GWL_EXSTYLE,WS_EX_APPWINDOW|WS_EX_TOPMOST);
  SetWindowLongPtr(DRIVER->hwnd,GWL_STYLE,WS_POPUP|WS_VISIBLE);
  SetWindowPos(DRIVER->hwnd,HWND_TOPMOST,0,0,fullscreenWidth,fullscreenHeight,SWP_SHOWWINDOW);
  ShowWindow(DRIVER->hwnd,SW_MAXIMIZE);
  driver->fullscreen=1;
}

static void mswm_exit_fullscreen(struct bigpc_video_driver *driver) {
  SetWindowLong(DRIVER->hwnd,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS);

  /* If we started up in fullscreen mode, we don't know the appropriate size to restore to.
   * Make something up.
   */
  if (!DRIVER->fsrestore.length) {
    DRIVER->fsrestore.length=sizeof(WINDOWPLACEMENT);
    DRIVER->fsrestore.flags=WPF_SETMINPOSITION;
    DRIVER->fsrestore.showCmd=SW_SHOW;
    DRIVER->fsrestore.rcNormalPosition.left=100;
    DRIVER->fsrestore.rcNormalPosition.top=100;
    DRIVER->fsrestore.rcNormalPosition.right=100+(driver->fbw*2);
    DRIVER->fsrestore.rcNormalPosition.bottom=100+(driver->fbh*2);
    AdjustWindowRect(&DRIVER->fsrestore.rcNormalPosition,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,0);
  }

  SetWindowLongPtr(DRIVER->hwnd,GWL_EXSTYLE,WS_EX_LEFT);
  SetWindowLongPtr(DRIVER->hwnd,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE);
  SetWindowPlacement(DRIVER->hwnd,&DRIVER->fsrestore);
  SetWindowPos(DRIVER->hwnd,HWND_NOTOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_FRAMECHANGED);
  ShowWindow(DRIVER->hwnd,SW_RESTORE);

  driver->fullscreen=0;
}
 
static void _mswm_set_fullscreen(struct bigpc_video_driver *driver,int fullscreen) {
  if (!DRIVER->window_setup_complete) return;
  if (fullscreen) {
    if (driver->fullscreen) return;
    mswm_enter_fullscreen(driver);
  } else {
    if (!driver->fullscreen) return;
    mswm_exit_fullscreen(driver);
  }
  mswm_autorelease(driver);
}

/* Set scaler.
 */
 
static void _mswm_set_scaler(struct bigpc_video_driver *driver,int scaler) {
  glBindTexture(GL_TEXTURE_2D,DRIVER->texid);
  int v=(scaler==FMN_SCALER_BLURRY)?GL_LINEAR:GL_NEAREST;
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,v);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,v);
}

/* Type definition.
 */
 
const struct bigpc_video_type bigpc_video_type_mswm={
  .name="mswm",
  .desc="Video for Windows",
  .objlen=sizeof(struct bigpc_video_driver_mswm),
  .has_wm=1,
  .del=_mswm_del,
  .init=_mswm_init,
  .update=mswm_update,
  .begin_soft=_mswm_begin_soft,
  //.begin_gx=_mswm_begin_gx,
  .end=_mswm_end,
  .cancel=_mswm_cancel,
  .show_cursor=_mswm_show_cursor,
  .set_fullscreen=_mswm_set_fullscreen,
  .set_scaler=_mswm_set_scaler,
};

/* Extra support for friend classes.
 */
 
HWND mswm_get_window_handle() {
  struct bigpc_video_driver *driver=mswm_global_driver;
  if (!driver) return 0;
  return DRIVER->hwnd;
}

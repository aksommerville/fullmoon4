#include "fmn_glx_internal.h"

/* Delete.
 */

void fmn_glx_del(struct fmn_glx *fmn_glx) {
  if (!fmn_glx) return;
  
  if (fmn_glx->image) XDestroyImage(fmn_glx->image);
  if (fmn_glx->gc) XFreeGC(fmn_glx->dpy,fmn_glx->gc);
  if (fmn_glx->dpy) XCloseDisplay(fmn_glx->dpy);
  if (fmn_glx->fb) free(fmn_glx->fb);
  if (fmn_glx->fbcvt) free(fmn_glx->fbcvt);
  
  free(fmn_glx);
}

/* New.
 */

struct fmn_glx *fmn_glx_new(
  const struct fmn_glx_delegate *delegate,
  const struct fmn_glx_setup *setup
) {
  if (!delegate||!setup) return 0;
  struct fmn_glx *fmn_glx=calloc(1,sizeof(struct fmn_glx));
  if (!fmn_glx) return 0;
  
  fmn_glx->delegate=*delegate;
  
  if (fmn_glx_init_start(fmn_glx,setup)<0) {
    fmn_glx_del(fmn_glx);
    return 0;
  }
  
  int err=-1;
  switch (fmn_glx->video_mode) {
    case FMN_GLX_VIDEO_MODE_OPENGL: err=fmn_glx_init_opengl(fmn_glx,setup); break;
    case FMN_GLX_VIDEO_MODE_FB_PURE: err=fmn_glx_init_fb_pure(fmn_glx,setup); break;
    case FMN_GLX_VIDEO_MODE_FB_GX: err=fmn_glx_init_fb_gx(fmn_glx,setup); break;
  }
  if (err<0) {
    fmn_glx_del(fmn_glx);
    return 0;
  }
  
  if (fmn_glx_init_finish(fmn_glx,setup)<0) {
    fmn_glx_del(fmn_glx);
    return 0;
  }
  
  return fmn_glx;
}

/* Trivial accessors.
 */

void *fmn_glx_get_userdata(const struct fmn_glx *fmn_glx) {
  if (!fmn_glx) return 0;
  return fmn_glx->delegate.userdata;
}

void fmn_glx_get_size(int *w,int *h,const struct fmn_glx *fmn_glx) {
  *w=fmn_glx->w;
  *h=fmn_glx->h;
}

void fmn_glx_get_fb_size(int *w,int *h,const struct fmn_glx *fmn_glx) {
  *w=fmn_glx->fbw;
  *h=fmn_glx->fbh;
}

int fmn_glx_get_fullscreen(const struct fmn_glx *fmn_glx) {
  return fmn_glx->fullscreen;
}

int fmn_glx_get_video_mode(const struct fmn_glx *fmn_glx) {
  return fmn_glx->video_mode;
}

int fmn_glx_get_fbfmt(const struct fmn_glx *fmn_glx) {
  return fmn_glx->fbfmt;
}

/* Fullscreen.
 */

void fmn_glx_set_fullscreen(struct fmn_glx *fmn_glx,int state) {
  state=state?1:0;
  if (state==fmn_glx->fullscreen) return;
  XEvent evt={
    .xclient={
      .type=ClientMessage,
      .message_type=fmn_glx->atom__NET_WM_STATE,
      .send_event=1,
      .format=32,
      .window=fmn_glx->win,
      .data={.l={
        state,
        fmn_glx->atom__NET_WM_STATE_FULLSCREEN,
      }},
    }
  };
  XSendEvent(fmn_glx->dpy,RootWindow(fmn_glx->dpy,fmn_glx->screen),0,SubstructureNotifyMask|SubstructureRedirectMask,&evt);
  XFlush(fmn_glx->dpy);
  fmn_glx->fullscreen=state;
}

/* Screensaver.
 */
 
void fmn_glx_suppress_screensaver(struct fmn_glx *fmn_glx) {
  if (fmn_glx->screensaver_suppressed) return;
  XForceScreenSaver(fmn_glx->dpy,ScreenSaverReset);
  fmn_glx->screensaver_suppressed=1;
}

/* Cursor.
 */
 
void fmn_glx_show_cursor(struct fmn_glx *fmn_glx,int show) {
  if (!fmn_glx) return;
  if (show) {
    if (fmn_glx->cursor_visible) return;
    XDefineCursor(fmn_glx->dpy,fmn_glx->win,0);
    fmn_glx->cursor_visible=1;
  } else {
    if (!fmn_glx->cursor_visible) return;
    XColor color;
    Pixmap pixmap=XCreateBitmapFromData(fmn_glx->dpy,fmn_glx->win,"\0\0\0\0\0\0\0\0",1,1);
    Cursor cursor=XCreatePixmapCursor(fmn_glx->dpy,pixmap,pixmap,&color,&color,0,0);
    XDefineCursor(fmn_glx->dpy,fmn_glx->win,cursor);
    XFreeCursor(fmn_glx->dpy,cursor);
    XFreePixmap(fmn_glx->dpy,pixmap);
    fmn_glx->cursor_visible=0;
  }
}

/* GX frame control.
 */

int fmn_glx_begin_gx(struct fmn_glx *fmn_glx) {
  if (!fmn_glx->ctx) return -1;
  fmn_glx->screensaver_suppressed=0;
  glXMakeCurrent(fmn_glx->dpy,fmn_glx->win,fmn_glx->ctx);
  glViewport(0,0,fmn_glx->w,fmn_glx->h);
  return 0;
}

void fmn_glx_end_gx(struct fmn_glx *fmn_glx) {
  if (!fmn_glx->ctx) return;
  glXSwapBuffers(fmn_glx->dpy,fmn_glx->win);
}

/* Recalculate (scale,dstx,dsty,dstw,dsth) if dirty.
 */
 
static void fmn_glx_require_render_bounds_continuous(struct fmn_glx *fmn_glx) {
  if (!fmn_glx->dstdirty) return;
  fmn_glx->dstdirty=0;
  
  int wforh=(fmn_glx->h*fmn_glx->fbw)/fmn_glx->fbh;
  if (wforh<=fmn_glx->w) {
    fmn_glx->dstw=wforh;
    fmn_glx->dsth=fmn_glx->h;
  } else {
    fmn_glx->dstw=fmn_glx->w;
    fmn_glx->dsth=(fmn_glx->w*fmn_glx->fbh)/fmn_glx->fbw;
  }
  if (fmn_glx->dstw<1) fmn_glx->dstw=1;
  if (fmn_glx->dsth<1) fmn_glx->dsth=1;
  
  // If we are close to the output size but not exact, fudge it a little.
  // It's slightly more efficient to cover the entire client area, lets us avoid a glClear().
  // But unreasonable to expect the user to set it precisely to the pixel.
  // Note that this is in absolute pixels, not a percentage: We're talking about the user's experience of sizing a window.
  // (as opposed to "how much aspect distortion is tolerable?", we prefer to leave that undecided).
  const int snap=10;
  if ((fmn_glx->dstw>=fmn_glx->w-snap)&&(fmn_glx->dsth>=fmn_glx->h-snap)) {
    fmn_glx->dstw=fmn_glx->w;
    fmn_glx->dsth=fmn_glx->h;
  }
  
  fmn_glx->dstx=(fmn_glx->w>>1)-(fmn_glx->dstw>>1);
  fmn_glx->dsty=(fmn_glx->h>>1)-(fmn_glx->dsth>>1);
}
 
static int fmn_glx_require_render_bounds_discrete(struct fmn_glx *fmn_glx) {
  if (!fmn_glx->dstdirty) return 0;
  fmn_glx->dstdirty=0;
  
  int xscale=fmn_glx->w/fmn_glx->fbw;
  int yscale=fmn_glx->h/fmn_glx->fbh;
  fmn_glx->scale=(xscale<yscale)?xscale:yscale;
  if (fmn_glx->scale<1) fmn_glx->scale=1;
  else if ((fmn_glx->scale_limit>0)&&(fmn_glx->scale>fmn_glx->scale_limit)) fmn_glx->scale=fmn_glx->scale_limit;
  
  fmn_glx->dstw=fmn_glx->fbw*fmn_glx->scale;
  fmn_glx->dsth=fmn_glx->fbh*fmn_glx->scale;
  fmn_glx->dstx=(fmn_glx->w>>1)-(fmn_glx->dstw>>1);
  fmn_glx->dsty=(fmn_glx->h>>1)-(fmn_glx->dsth>>1);
  
  return 1;
}

/* FMN_GLX_VIDEO_MODE_FB_GX: Convert client buffer into conversion buffer if necessary.
 * Returns the final RGBA framebuffer.
 */
 
static void *fmn_glx_prepare_gx_framebuffer(struct fmn_glx *fmn_glx) {
  if (!fmn_glx->fbcvt) return fmn_glx->fb;
  switch (fmn_glx->fbfmt) {
    #if BYTE_ORDER==LITTLE_ENDIAN
      case FMN_GLX_FBFMT_XRGB: fmn_glx_fbcvt_rgb(fmn_glx->fbcvt,0,8,16,fmn_glx->fb,16,8,0,fmn_glx->fbw,fmn_glx->fbh); break;
      case FMN_GLX_FBFMT_XBGR: fmn_glx_fbcvt_rgb(fmn_glx->fbcvt,0,8,16,fmn_glx->fb,0,8,16,fmn_glx->fbw,fmn_glx->fbh); break;
      case FMN_GLX_FBFMT_RGBX: fmn_glx_fbcvt_rgb(fmn_glx->fbcvt,0,8,16,fmn_glx->fb,24,16,8,fmn_glx->fbw,fmn_glx->fbh); break;
      case FMN_GLX_FBFMT_BGRX: fmn_glx_fbcvt_rgb(fmn_glx->fbcvt,0,8,16,fmn_glx->fb,8,16,24,fmn_glx->fbw,fmn_glx->fbh); break;
    #else
      case FMN_GLX_FBFMT_XRGB: fmn_glx_fbcvt_rgb(fmn_glx->fbcvt,24,16,8,fmn_glx->fb,16,8,0,fmn_glx->fbw,fmn_glx->fbh); break;
      case FMN_GLX_FBFMT_XBGR: fmn_glx_fbcvt_rgb(fmn_glx->fbcvt,24,16,8,fmn_glx->fb,0,8,16,fmn_glx->fbw,fmn_glx->fbh); break;
      case FMN_GLX_FBFMT_RGBX: fmn_glx_fbcvt_rgb(fmn_glx->fbcvt,24,16,8,fmn_glx->fb,24,16,8,fmn_glx->fbw,fmn_glx->fbh); break;
      case FMN_GLX_FBFMT_BGRX: fmn_glx_fbcvt_rgb(fmn_glx->fbcvt,24,16,8,fmn_glx->fb,8,16,24,fmn_glx->fbw,fmn_glx->fbh); break;
    #endif
    case FMN_GLX_FBFMT_BGR565LE: fmn_glx_fbcvt_rgb_bgr565le(fmn_glx->fbcvt,fmn_glx->fb,fmn_glx->fbw,fmn_glx->fbh); break;
    case FMN_GLX_FBFMT_BGRX4444BE: fmn_glx_fbcvt_rgb_bgrx4444be(fmn_glx->fbcvt,fmn_glx->fb,fmn_glx->fbw,fmn_glx->fbh); break;
    case FMN_GLX_FBFMT_BGR332: fmn_glx_fbcvt_rgb_bgr332(fmn_glx->fbcvt,fmn_glx->fb,fmn_glx->fbw,fmn_glx->fbh); break;
    case FMN_GLX_FBFMT_Y1: fmn_glx_fbcvt_rgb_y1(fmn_glx->fbcvt,fmn_glx->fb,fmn_glx->fbw,fmn_glx->fbh); break;
    case FMN_GLX_FBFMT_Y8: fmn_glx_fbcvt_rgb_y8(fmn_glx->fbcvt,fmn_glx->fb,fmn_glx->fbw,fmn_glx->fbh); break;
  }
  return fmn_glx->fbcvt;
}

/* FMN_GLX_VIDEO_MODE_FB_GX: Main rendering.
 * Context must be enabled, and texture uploaded.
 */
 
static void fmn_glx_render_gx_framebuffer(struct fmn_glx *fmn_glx) {
  if ((fmn_glx->dstw<fmn_glx->w)||(fmn_glx->dsth<fmn_glx->h)) {
    glViewport(0,0,fmn_glx->w,fmn_glx->h);
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  glViewport(fmn_glx->dstx,fmn_glx->dsty,fmn_glx->dstw,fmn_glx->dsth);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_TRIANGLE_STRIP);
    glColor4ub(0xff,0xff,0xff,0xff);
    glTexCoord2i(0,0); glVertex2i(-1, 1);
    glTexCoord2i(0,1); glVertex2i(-1,-1);
    glTexCoord2i(1,0); glVertex2i( 1, 1);
    glTexCoord2i(1,1); glVertex2i( 1,-1);
  glEnd();
}

/* FMN_GLX_VIDEO_MODE_FB_PURE: Reallocate scale-up image.
 */
 
static int fmn_glx_require_image(struct fmn_glx *fmn_glx) {

  // It is very possible for dst bounds to change position but not size.
  // If that was the case, we don't need to do anything here.
  if (fmn_glx->image&&(fmn_glx->image->width==fmn_glx->dstw)&&(fmn_glx->image->height==fmn_glx->dsth)) return 0;
  
  if (fmn_glx->image) {
    XDestroyImage(fmn_glx->image);
    fmn_glx->image=0;
  }
  void *pixels=malloc(fmn_glx->dstw*4*fmn_glx->dsth);
  if (!pixels) return -1;
  if (!(fmn_glx->image=XCreateImage(
    fmn_glx->dpy,DefaultVisual(fmn_glx->dpy,fmn_glx->screen),24,ZPixmap,0,pixels,fmn_glx->dstw,fmn_glx->dsth,32,fmn_glx->dstw*4
  ))) {
    free(pixels);
    return -1;
  }
    
  // And recalculate channel shifts...
  if (!fmn_glx->image->red_mask||!fmn_glx->image->green_mask||!fmn_glx->image->blue_mask) return -1;
  uint32_t m;
  fmn_glx->rshift=0; m=fmn_glx->image->red_mask;   for (;!(m&1);m>>=1,fmn_glx->rshift++) ; if (m!=0xff) return -1;
  fmn_glx->gshift=0; m=fmn_glx->image->green_mask; for (;!(m&1);m>>=1,fmn_glx->gshift++) ; if (m!=0xff) return -1;
  fmn_glx->bshift=0; m=fmn_glx->image->blue_mask;  for (;!(m&1);m>>=1,fmn_glx->bshift++) ; if (m!=0xff) return -1;
  switch (fmn_glx->fbfmt) {
    case FMN_GLX_FBFMT_XRGB: fmn_glx->scale_reformat=(fmn_glx->rshift!=16)||(fmn_glx->gshift!=8)||(fmn_glx->bshift!=0); break;
    case FMN_GLX_FBFMT_XBGR: fmn_glx->scale_reformat=(fmn_glx->rshift!=0)||(fmn_glx->gshift!=8)||(fmn_glx->bshift!=16); break;
    case FMN_GLX_FBFMT_RGBX: fmn_glx->scale_reformat=(fmn_glx->rshift!=24)||(fmn_glx->gshift!=16)||(fmn_glx->bshift!=8); break;
    case FMN_GLX_FBFMT_BGRX: fmn_glx->scale_reformat=(fmn_glx->rshift!=8)||(fmn_glx->gshift!=16)||(fmn_glx->bshift!=24); break;
    default: fmn_glx->scale_reformat=1; break;
  }
  
  return 0;
}

/* FMN_GLX_VIDEO_MODE_FB_PURE: Scale and convert (fb) into (image).
 */
 
static void fmn_glx_fb_scale(struct fmn_glx *fmn_glx) {

  // These can't be wrong, due to prior assertions.
  // But it would be disastrous if they are, so hey why not be sure.
  if (fmn_glx->image->width!=fmn_glx->fbw*fmn_glx->scale) return;
  if (fmn_glx->image->height!=fmn_glx->fbh*fmn_glx->scale) return;
  
  if (fmn_glx->scale_reformat) switch (fmn_glx->fbfmt) {
    case FMN_GLX_FBFMT_XRGB: {
        fmn_glx_scale_swizzle(
          fmn_glx->image->data,fmn_glx->fb,
          fmn_glx->fbw,fmn_glx->fbh,fmn_glx->scale,
          fmn_glx->rshift,fmn_glx->gshift,fmn_glx->bshift,16,8,0
        );
      } break;
    case FMN_GLX_FBFMT_XBGR: {
        fmn_glx_scale_swizzle(
          fmn_glx->image->data,fmn_glx->fb,
          fmn_glx->fbw,fmn_glx->fbh,fmn_glx->scale,
          fmn_glx->rshift,fmn_glx->gshift,fmn_glx->bshift,0,8,16
        );
      } break;
    case FMN_GLX_FBFMT_RGBX: {
        fmn_glx_scale_swizzle(
          fmn_glx->image->data,fmn_glx->fb,
          fmn_glx->fbw,fmn_glx->fbh,fmn_glx->scale,
          fmn_glx->rshift,fmn_glx->gshift,fmn_glx->bshift,24,16,8
        );
      } break;
    case FMN_GLX_FBFMT_BGRX: {
        fmn_glx_scale_swizzle(
          fmn_glx->image->data,fmn_glx->fb,
          fmn_glx->fbw,fmn_glx->fbh,fmn_glx->scale,
          fmn_glx->rshift,fmn_glx->gshift,fmn_glx->bshift,8,16,24
        );
      } break;
    case FMN_GLX_FBFMT_BGR565LE: {
        fmn_glx_scale_bgr565le(
          fmn_glx->image->data,fmn_glx->fb,
          fmn_glx->fbw,fmn_glx->fbh,fmn_glx->scale,
          fmn_glx->rshift,fmn_glx->gshift,fmn_glx->bshift
        );
      } break;
    case FMN_GLX_FBFMT_BGRX4444BE: {
        fmn_glx_scale_bgrx4444be(
          fmn_glx->image->data,fmn_glx->fb,
          fmn_glx->fbw,fmn_glx->fbh,fmn_glx->scale,
          fmn_glx->rshift,fmn_glx->gshift,fmn_glx->bshift
        );
      } break;
    case FMN_GLX_FBFMT_BGR332: {
        fmn_glx_scale_bgr332(
          fmn_glx->image->data,fmn_glx->fb,
          fmn_glx->fbw,fmn_glx->fbh,fmn_glx->scale,
          fmn_glx->rshift,fmn_glx->gshift,fmn_glx->bshift
        );
      } break;
    case FMN_GLX_FBFMT_Y1: {
        fmn_glx_scale_y1(
          fmn_glx->image->data,fmn_glx->fb,
          fmn_glx->fbw,fmn_glx->fbh,fmn_glx->scale
        );
      } break;
    case FMN_GLX_FBFMT_Y8: {
        fmn_glx_scale_y8(
          fmn_glx->image->data,fmn_glx->fb,
          fmn_glx->fbw,fmn_glx->fbh,fmn_glx->scale
        );
      } break;
  } else fmn_glx_scale_same32(fmn_glx->image->data,fmn_glx->fb,fmn_glx->fbw,fmn_glx->fbh,fmn_glx->scale);
}

/* FB frame control. (both pure and gx-backed)
 */
 
void *fmn_glx_begin_fb(struct fmn_glx *fmn_glx) {
  fmn_glx->screensaver_suppressed=0;
  return fmn_glx->fb;
}

void fmn_glx_end_fb(struct fmn_glx *fmn_glx,void *fb) {
  if (fb!=fmn_glx->fb) return;
  if ((fmn_glx->w<1)||(fmn_glx->h<1)) return;
  switch (fmn_glx->video_mode) {
  
    case FMN_GLX_VIDEO_MODE_FB_PURE: {
        if (fmn_glx_require_render_bounds_discrete(fmn_glx)||!fmn_glx->image) {
          if (fmn_glx_require_image(fmn_glx)<0) return;
          XFillRectangle(fmn_glx->dpy,fmn_glx->win,fmn_glx->gc,0,0,fmn_glx->dstx,fmn_glx->h);
          XFillRectangle(fmn_glx->dpy,fmn_glx->win,fmn_glx->gc,0,0,fmn_glx->w,fmn_glx->dsty);
          XFillRectangle(fmn_glx->dpy,fmn_glx->win,fmn_glx->gc,fmn_glx->dstx,fmn_glx->dsty+fmn_glx->dsth,fmn_glx->w,fmn_glx->h);
          XFillRectangle(fmn_glx->dpy,fmn_glx->win,fmn_glx->gc,fmn_glx->dstx+fmn_glx->dstw,fmn_glx->dsty,fmn_glx->w,fmn_glx->h);
        }
        fmn_glx_fb_scale(fmn_glx);
        XPutImage(fmn_glx->dpy,fmn_glx->win,fmn_glx->gc,fmn_glx->image,0,0,fmn_glx->dstx,fmn_glx->dsty,fmn_glx->image->width,fmn_glx->image->height);
      } break;
      
    case FMN_GLX_VIDEO_MODE_FB_GX: {
        fmn_glx_require_render_bounds_continuous(fmn_glx);
        glXMakeCurrent(fmn_glx->dpy,fmn_glx->win,fmn_glx->ctx);
        void *rgba=fmn_glx_prepare_gx_framebuffer(fmn_glx);
        if (rgba) {
          glBindTexture(GL_TEXTURE_2D,fmn_glx->fbtexid);
          glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,fmn_glx->fbw,fmn_glx->fbh,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
          fmn_glx_render_gx_framebuffer(fmn_glx);
          glXSwapBuffers(fmn_glx->dpy,fmn_glx->win);
        }
      } break;
  }
}

/* Convert coordinates.
 */
 
void fmn_glx_coord_fb_from_win(int *x,int *y,const struct fmn_glx *fmn_glx) {
  if ((fmn_glx->dstw<1)||(fmn_glx->dsth<1)||(fmn_glx->fbw<1)||(fmn_glx->fbh<1)) return;
  (*x)=(((*x)-fmn_glx->dstx)*fmn_glx->fbw)/fmn_glx->dstw;
  (*y)=(((*y)-fmn_glx->dsty)*fmn_glx->fbh)/fmn_glx->dsth;
}

void fmn_glx_coord_win_from_fb(int *x,int *y,const struct fmn_glx *fmn_glx) {
  if ((fmn_glx->dstw<1)||(fmn_glx->dsth<1)||(fmn_glx->fbw<1)||(fmn_glx->fbh<1)) return;
  *x=((*x)*fmn_glx->dstw)/fmn_glx->fbw+fmn_glx->dstx;
  *y=((*y)*fmn_glx->dsth)/fmn_glx->fbh+fmn_glx->dsty;
}

/* Scale-up filter.
 */
 
void fmn_glx_set_final_filter(struct fmn_glx *glx,int linear_filter) {
  if (!glx->fbtexid) return;
  glBindTexture(GL_TEXTURE_2D,glx->fbtexid);
  int v=linear_filter?GL_LINEAR:GL_NEAREST;
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,v);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,v);
}

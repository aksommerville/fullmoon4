#include "fmn_glx_internal.h"

/* Key press, release, or repeat.
 */
 
static int fmn_glx_evt_key(struct fmn_glx *fmn_glx,XKeyEvent *evt,int value) {

  /* Pass the raw keystroke. */
  if (fmn_glx->delegate.key) {
    KeySym keysym=XkbKeycodeToKeysym(fmn_glx->dpy,evt->keycode,0,0);
    if (keysym) {
      int keycode=fmn_glx_usb_usage_from_keysym((int)keysym);
      if (keycode) {
        int err=fmn_glx->delegate.key(fmn_glx->delegate.userdata,keycode,value);
        if (err) return err; // Stop here if acknowledged.
      }
    }
  }
  
  /* Pass text if press or repeat, and text can be acquired. */
  if (fmn_glx->delegate.text) {
    int shift=(evt->state&ShiftMask)?1:0;
    KeySym tkeysym=XkbKeycodeToKeysym(fmn_glx->dpy,evt->keycode,0,shift);
    if (shift&&!tkeysym) { // If pressing shift makes this key "not a key anymore", fuck that and pretend shift is off
      tkeysym=XkbKeycodeToKeysym(fmn_glx->dpy,evt->keycode,0,0);
    }
    if (tkeysym) {
      int codepoint=fmn_glx_codepoint_from_keysym(tkeysym);
      if (codepoint && (evt->type == KeyPress || evt->type == KeyRepeat)) {
        fmn_glx->delegate.text(fmn_glx->delegate.userdata,codepoint);
      }
    }
  }
  
  return 0;
}

/* Mouse events.
 */
 
static int fmn_glx_evt_mbtn(struct fmn_glx *fmn_glx,XButtonEvent *evt,int value) {
  //fprintf(stderr,"%s state=0x%08x\n",__func__,evt->state);
  
  // I swear X11 used to automatically report the wheel as (6,7) while shift held, and (4,5) otherwise.
  // After switching to GNOME 3, seems it is only ever (4,5).
  #define SHIFTABLE(v) (evt->state&ShiftMask)?v:0,(evt->state&ShiftMask)?0:v
  
  switch (evt->button) {
    case 1: if (fmn_glx->delegate.mbutton) fmn_glx->delegate.mbutton(fmn_glx->delegate.userdata,1,value); break;
    case 2: if (fmn_glx->delegate.mbutton) fmn_glx->delegate.mbutton(fmn_glx->delegate.userdata,3,value); break;
    case 3: if (fmn_glx->delegate.mbutton) fmn_glx->delegate.mbutton(fmn_glx->delegate.userdata,2,value); break;
    case 4: if (value&&fmn_glx->delegate.mwheel) fmn_glx->delegate.mwheel(fmn_glx->delegate.userdata,SHIFTABLE(-1)); break;
    case 5: if (value&&fmn_glx->delegate.mwheel) fmn_glx->delegate.mwheel(fmn_glx->delegate.userdata,SHIFTABLE(1)); break;
    case 6: if (value&&fmn_glx->delegate.mwheel) fmn_glx->delegate.mwheel(fmn_glx->delegate.userdata,-1,0); break;
    case 7: if (value&&fmn_glx->delegate.mwheel) fmn_glx->delegate.mwheel(fmn_glx->delegate.userdata,1,0); break;
  }
  #undef SHIFTABLE
  return 0;
}

static int fmn_glx_evt_mmotion(struct fmn_glx *fmn_glx,XMotionEvent *evt) {
  if (fmn_glx->delegate.mmotion) {
    fmn_glx->delegate.mmotion(fmn_glx->delegate.userdata,evt->x,evt->y);
  }
  return 0;
}

/* Client message.
 */
 
static int fmn_glx_evt_client(struct fmn_glx *fmn_glx,XClientMessageEvent *evt) {
  if (evt->message_type==fmn_glx->atom_WM_PROTOCOLS) {
    if (evt->format==32) {
      if (evt->data.l[0]==fmn_glx->atom_WM_DELETE_WINDOW) {
        if (fmn_glx->delegate.close) {
          fmn_glx->delegate.close(fmn_glx->delegate.userdata);
        }
      }
    }
  }
  return 0;
}

/* Configuration event (eg resize).
 */
 
static int fmn_glx_evt_configure(struct fmn_glx *fmn_glx,XConfigureEvent *evt) {
  int nw=evt->width,nh=evt->height;
  if ((nw!=fmn_glx->w)||(nh!=fmn_glx->h)) {
    fmn_glx->dstdirty=1;
    fmn_glx->w=nw;
    fmn_glx->h=nh;
    if (fmn_glx->delegate.resize) {
      fmn_glx->delegate.resize(fmn_glx->delegate.userdata,nw,nh);
    }
  }
  return 0;
}

/* Focus.
 */
 
static int fmn_glx_evt_focus(struct fmn_glx *fmn_glx,XFocusInEvent *evt,int value) {
  if (value==fmn_glx->focus) return 0;
  fmn_glx->focus=value;
  if (fmn_glx->delegate.focus) {
    fmn_glx->delegate.focus(fmn_glx->delegate.userdata,value);
  }
  return 0;
}

/* Process one event.
 */
 
static int fmn_glx_receive_event(struct fmn_glx *fmn_glx,XEvent *evt) {
  if (!evt) return -1;
  switch (evt->type) {
  
    case KeyPress: return fmn_glx_evt_key(fmn_glx,&evt->xkey,1);
    case KeyRelease: return fmn_glx_evt_key(fmn_glx,&evt->xkey,0);
    case KeyRepeat: return fmn_glx_evt_key(fmn_glx,&evt->xkey,2);
    
    case ButtonPress: return fmn_glx_evt_mbtn(fmn_glx,&evt->xbutton,1);
    case ButtonRelease: return fmn_glx_evt_mbtn(fmn_glx,&evt->xbutton,0);
    case MotionNotify: return fmn_glx_evt_mmotion(fmn_glx,&evt->xmotion);
    
    case ClientMessage: return fmn_glx_evt_client(fmn_glx,&evt->xclient);
    
    case ConfigureNotify: return fmn_glx_evt_configure(fmn_glx,&evt->xconfigure);
    
    case FocusIn: return fmn_glx_evt_focus(fmn_glx,&evt->xfocus,1);
    case FocusOut: return fmn_glx_evt_focus(fmn_glx,&evt->xfocus,0);
    
  }
  return 0;
}

/* Update.
 */
 
int fmn_glx_update(struct fmn_glx *fmn_glx) {
  int evtc=XEventsQueued(fmn_glx->dpy,QueuedAfterFlush);
  while (evtc-->0) {
    XEvent evt={0};
    XNextEvent(fmn_glx->dpy,&evt);
    if ((evtc>0)&&(evt.type==KeyRelease)) {
      XEvent next={0};
      XNextEvent(fmn_glx->dpy,&next);
      evtc--;
      if ((next.type==KeyPress)&&(evt.xkey.keycode==next.xkey.keycode)&&(evt.xkey.time>=next.xkey.time-FMN_GLX_KEY_REPEAT_INTERVAL)) {
        evt.type=KeyRepeat;
        if (fmn_glx_receive_event(fmn_glx,&evt)<0) return -1;
      } else {
        if (fmn_glx_receive_event(fmn_glx,&evt)<0) return -1;
        if (fmn_glx_receive_event(fmn_glx,&next)<0) return -1;
      }
    } else {
      if (fmn_glx_receive_event(fmn_glx,&evt)<0) return -1;
    }
  }
  return 0;
}

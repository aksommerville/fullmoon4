#ifndef MSWM_INTERNAL_H
#define MSWM_INTERNAL_H

#include "opt/bigpc/bigpc_video.h"
#include "opt/bigpc/bigpc_image.h"
#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>

#define MSWM_WINDOW_CLASS_NAME "com_aksommerville_fullmoon"

#ifndef WM_MOUSEHWHEEL
  #define WM_MOUSEHWHEEL 0x020e
#endif

#define MSWM_AUTORELEASE_LIMIT 4

struct bigpc_video_driver_mswm {
  struct bigpc_video_driver hdr;
  int window_setup_complete;
  int translate_events;
  HINSTANCE instance;
  WNDCLASSEX wndclass;
  ATOM wndclass_atom;
  HWND hwnd;
  HDC hdc;
  HGLRC hglrc;
  int winw,winh;
  int fullscreen;
  int showcursor;
  WINDOWPLACEMENT fsrestore;
  HICON appicon;
  struct bigpc_image *fb;
  GLuint texid;
  
  /* A stupid bug: When I switch fullscreen on or off, any held keys get forgotten or something.
   * I don't have a Windows installation, running this in Wine, and it's likely just a Wine problem.
   * But anyway, I'll fix by recording all held keys and auto-releasing them on fullscreen changes.
   */
  int autorelease[MSWM_AUTORELEASE_LIMIT];
};

#define DRIVER ((struct bigpc_video_driver_mswm*)driver)

extern struct bigpc_video_driver *mswm_global_driver;

LRESULT mswm_cb_msg(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);
int mswm_update(struct bigpc_video_driver *driver);
int mswm_setup_window(struct bigpc_video_driver *driver);

int mswm_usage_from_keysym(int keysym);

void mswm_autorelease_add(struct bigpc_video_driver *driver,int usage);
void mswm_autorelease_remove(struct bigpc_video_driver *driver,int usage);
void mswm_autorelease(struct bigpc_video_driver *driver);

#endif

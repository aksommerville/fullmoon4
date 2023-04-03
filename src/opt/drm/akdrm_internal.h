#ifndef AKDRM_INTERNAL_H
#define AKDRM_INTERNAL_H

#include "akdrm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <endian.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>

// Sanity limits.
#define AKDRM_RATE_MIN 10
#define AKDRM_RATE_MAX 1000
#define AKDRM_SIZE_MIN 1
#define AKDRM_SIZE_MAX 16384

struct akdrm {
  int fd;
  int w,h;
  int video_mode;
  int fbw,fbh;
  int fbfmt;
  void *clientfb;
  
  uint32_t connid,encid,crtcid;
  drmModeCrtcPtr crtc_restore;
  drmModeModeInfo mode;
  
  struct gbm_device *gbmdevice;
  struct gbm_surface *gbmsurface;
  EGLDisplay egldisplay;
  EGLContext eglcontext;
  EGLSurface eglsurface;
  
  int flip_pending;
};

// akdrm_io.c
int akdrm_open(struct akdrm *akdrm,const struct akdrm_config *config);

int akdrm_init_gx(struct akdrm *akdrm);
int akdrm_gx_begin(struct akdrm *akdrm);
void akdrm_gx_end(struct akdrm *akdrm);
void akdrm_gx_render_fb(struct akdrm *akdrm); // AKDRM_VIDEO_MODE_FB_GX only

int akdrm_init_fb(struct akdrm *akdrm);
int akdrm_fb_swap(struct akdrm *akdrm);

#endif

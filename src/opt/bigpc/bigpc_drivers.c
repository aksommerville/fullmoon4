#include "bigpc_internal.h"

/* Pop the next token from a comma-delimited list.
 * Always returns <0 or >srcp.
 */
 
static int bigpc_next_comma_token(void *dstpp,int *dstcp,const char *src,int srcp) {
  while (src[srcp]&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (!src[srcp]) return -1;
  const char *dst=src+srcp;
  int dstc=0;
  while (src[srcp]&&(src[srcp]!=',')) { srcp++; dstc++; }
  while (dstc&&((unsigned char)dst[dstc-1]<=0x20)) dstc--;
  *(const void**)dstpp=dst;
  *dstcp=dstc;
  if (src[srcp]==',') return srcp+1;
  if (dstc) return srcp;
  return 0;
}

/* Try initializing video with a given driver type.
 */
 
static int bigpc_video_try_init(const struct bigpc_video_type *type) {
  struct bigpc_video_delegate delegate={
    .userdata=0,
    .cb_close=bigpc_cb_close,
    .cb_focus=bigpc_cb_focus,
    .cb_resize=bigpc_cb_resize,
    .cb_key=bigpc_cb_key,
    .cb_text=bigpc_cb_text,
    .cb_mmotion=bigpc_cb_mmotion,
    .cb_mbutton=bigpc_cb_mbutton,
    .cb_mwheel=bigpc_cb_mwheel,
  };
  struct bigpc_video_config config={
    .fbw=FMN_COLC*bigpc.config.tilesize,
    .fbh=FMN_ROWC*bigpc.config.tilesize,
    .renderer=bigpc.config.video_renderer,
    .w=0,
    .h=0,
    .fullscreen=bigpc.config.video_fullscreen,
    .title="Full Moon",
    .iconrgba=0,//TODO app icon
    .iconw=0,
    .iconh=0,
  };
  if (!(bigpc.video=bigpc_video_driver_new(type,&delegate,&config))) {
    fprintf(stderr,"%s: Error starting up video driver '%s'.\n",bigpc.exename,type->name);
    return -2;
  }
  fprintf(stderr,"%s: Using video driver '%s'.\n",bigpc.exename,type->name);
  return 0;
}

/* Init video.
 */

int bigpc_video_init() {

  // Select and initialize a video driver.
  if (bigpc.config.video_drivers) {
    int p=0; while (1) {
      const char *name=0;
      int namec=0;
      if ((p=bigpc_next_comma_token(&name,&namec,bigpc.config.video_drivers,p))<0) break;
      const struct bigpc_video_type *type=bigpc_video_type_by_name(name,namec);
      if (!type) {
        fprintf(stderr,"%s: Video driver '%.*s' not found.\n",bigpc.exename,namec,name);
        continue;
      }
      if (bigpc_video_try_init(type)>=0) break;
    }
  } else {
    int p=0; while (1) {
      const struct bigpc_video_type *type=bigpc_video_type_by_index(p++);
      if (!type) break;
      if (type->appointment_only) continue;
      if (bigpc_video_try_init(type)>=0) break;
    }
  }
  if (!bigpc.video) {
    fprintf(stderr,"%s: Failed to initialize a video driver.\n",bigpc.exename);
    return -2;
  }
  
  // TODO Renderer setup?
  
  return 0;
}

/* Try initializing audio driver of the given type.
 */
 
static int bigpc_audio_try_init(const struct bigpc_audio_type *type) {
  struct bigpc_audio_delegate delegate={
    .userdata=0,
    .cb_pcm_out=bigpc_cb_pcm_out,
  };
  struct bigpc_audio_config config={
    .rate=bigpc.config.audio_rate,
    .chanc=bigpc.config.audio_chanc,
    .format=bigpc.config.audio_format,
    .device=0,//TODO configurable
  };
  if (!(bigpc.audio=bigpc_audio_driver_new(type,&delegate,&config))) {
    fprintf(stderr,"%s: Failed to initialize audio driver '%s'\n",bigpc.exename,type->name);
    return -2;
  }
  fprintf(stderr,
    "%s: Using audio driver '%s' rate=%d chanc=%d format=%d\n",
    bigpc.exename,type->name,
    bigpc.audio->rate,bigpc.audio->chanc,bigpc.audio->format
  );
  return 0;
}

/* Init audio.
 */

int bigpc_audio_init() {

  // Select and initialize an audio driver.
  if (bigpc.config.audio_drivers) {
    int p=0; while (1) {
      const char *name=0;
      int namec=0;
      if ((p=bigpc_next_comma_token(&name,&namec,bigpc.config.audio_drivers,p))<0) break;
      const struct bigpc_audio_type *type=bigpc_audio_type_by_name(name,namec);
      if (!type) {
        fprintf(stderr,"%s: Audio driver '%.*s' not found.\n",bigpc.exename,namec,name);
        continue;
      }
      if (bigpc_audio_try_init(type)>=0) break;
    }
  } else {
    int p=0; while (1) {
      const struct bigpc_audio_type *type=bigpc_audio_type_by_index(p++);
      if (!type) break;
      if (type->appointment_only) continue;
      if (bigpc_audio_try_init(type)>=0) break;
    }
  }
  if (!bigpc.audio) {
    fprintf(stderr,"%s: Failed to initialize an audio driver.\n",bigpc.exename);
    return -2;
  }
  
  // TODO Synthesizer.

  return 0;
}

/* Initialize one input driver of a given type.
 */
 
static int bigpc_input_try_init(const struct bigpc_input_type *type) {
  if (bigpc.inputc>=bigpc.inputa) {
    int na=bigpc.inputa+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(bigpc.inputv,sizeof(void*)*na);
    if (!nv) return -1;
    bigpc.inputv=nv;
    bigpc.inputa=na;
  }
  struct bigpc_input_delegate delegate={
    .userdata=0,
    .cb_connect=bigpc_cb_connect,
    .cb_disconnect=bigpc_cb_disconnect,
    .cb_event=bigpc_cb_event,
  };
  struct bigpc_input_driver *driver=bigpc_input_driver_new(type,&delegate);
  if (!driver) {
    fprintf(stderr,"%s: Failed to initialize input driver '%s'\n",bigpc.exename,type->name);
    return -2;
  }
  bigpc.inputv[bigpc.inputc++]=driver;
  fprintf(stderr,"%s: Using input driver '%s'\n",bigpc.exename,type->name);
  return 0;
}

/* Init input.
 */

int bigpc_input_init() {
  
  // Similar to audio and video, except we don't stop after one success, and don't require that one exist after.
  if (bigpc.config.input_drivers) {
    int p=0; while (1) {
      const char *name=0;
      int namec=0;
      if ((p=bigpc_next_comma_token(&name,&namec,bigpc.config.input_drivers,p))<0) break;
      const struct bigpc_input_type *type=bigpc_input_type_by_name(name,namec);
      if (!type) {
        fprintf(stderr,"%s: Input driver '%.*s' not found.\n",bigpc.exename,namec,name);
        continue;
      }
      bigpc_input_try_init(type);
    }
  } else {
    int p=0; while (1) {
      const struct bigpc_input_type *type=bigpc_input_type_by_index(p++);
      if (!type) break;
      if (type->appointment_only) continue;
      bigpc_input_try_init(type);
    }
  }
  
  // TODO Input manager.
  
  return 0;
}

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
 
// Generated from appicon.png
extern const int fmn_appicon_w;
extern const int fmn_appicon_h;
extern const unsigned char fmn_appicon[];
 
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
    .iconrgba=fmn_appicon,
    .iconw=fmn_appicon_w,
    .iconh=fmn_appicon_h,
  };
  if (!(bigpc.video=bigpc_video_driver_new(type,&delegate,&config))) {
    fprintf(stderr,"%s: Error starting up video driver '%s'.\n",bigpc.exename,type->name);
    return -2;
  }
  fprintf(stderr,"%s: Using video driver '%s'.\n",bigpc.exename,type->name);
  return 0;
}

/* Initialize renderer.
 */
 
static int bigpc_render_try_init(const struct bigpc_render_type *type) {
  if (!(bigpc.render=bigpc_render_new(type,bigpc.video))) {
    fprintf(stderr,"%s: Error initializing renderer '%s'.\n",bigpc.exename,type->name);
    return -2;
  }
  fprintf(stderr,"%s: Using renderer '%s'.\n",bigpc.exename,type->name);
  bigpc.render->w=bigpc.video->w;
  bigpc.render->h=bigpc.video->h;
  bigpc.render->datafile=bigpc.datafile;
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
  
  // Initialize renderer, same idea as video.
  if (bigpc.config.render_drivers) {
    int p=0; while (1) {
      const char *name=0;
      int namec=0;
      if ((p=bigpc_next_comma_token(&name,&namec,bigpc.config.render_drivers,p))<0) break;
      const struct bigpc_render_type *type=bigpc_render_type_by_name(name,namec);
      if (!type) {
        fprintf(stderr,"%s: Renderer '%.*s' not found.\n",bigpc.exename,namec,name);
        continue;
      }
      if (bigpc_render_try_init(type)>=0) break;
    }
  } else {
    int p=0; while (1) {
      const struct bigpc_render_type *type=bigpc_render_type_by_index(p++);
      if (!type) break;
      if (type->appointment_only) continue;
      if (bigpc_render_try_init(type)>=0) break;
    }
  }
  if (!bigpc.render) {
    fprintf(stderr,"%s: Failed to initialize a renderer.\n",bigpc.exename);
    return -2;
  }
  
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

/* Initialize synthesizer.
 */
 
static int bigpc_cb_load_instrument(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  if (bigpc_synth_set_instrument(bigpc.synth,id,v,c)<0) return -1;
  return 0;
}
 
static int bigpc_cb_load_sound(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  if (bigpc_synth_set_sound(bigpc.synth,id,v,c)<0) return -1;
  return 0;
}
 
static int bigpc_synth_try_init(const struct bigpc_synth_type *type) {
  struct bigpc_synth_config config={
    .rate=bigpc.audio->rate,
    .chanc=bigpc.audio->chanc,
    .format=bigpc.audio->format,
  };
  if (!(bigpc.synth=bigpc_synth_new(type,&config))) {
    fprintf(stderr,"%s: Failed to initialize synthesizer '%s'\n",bigpc.exename,type->name);
    return -2;
  }
  fprintf(stderr,"%s: Using synthesizer '%s'.\n",bigpc.exename,type->name);
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
  
  // Initialize synthesizer, same idea.
  if (bigpc.config.synth_drivers) {
    int p=0; while (1) {
      const char *name=0;
      int namec=0;
      if ((p=bigpc_next_comma_token(&name,&namec,bigpc.config.synth_drivers,p))<0) break;
      const struct bigpc_synth_type *type=bigpc_synth_type_by_name(name,namec);
      if (!type) {
        fprintf(stderr,"%s: Synthesizer '%.*s' not found.\n",bigpc.exename,namec,name);
        continue;
      }
      if (bigpc_synth_try_init(type)>=0) break;
    }
  } else {
    int p=0; while (1) {
      const struct bigpc_synth_type *type=bigpc_synth_type_by_index(p++);
      if (!type) break;
      if (type->appointment_only) continue;
      if (bigpc_synth_try_init(type)>=0) break;
    }
  }
  if (!bigpc.synth) {
    fprintf(stderr,"%s: Failed to initialize a synthesizer.\n",bigpc.exename);
    return -2;
  }
  
  // Load instrument and sound resources.
  if (
    (fmn_datafile_for_each_of_qualified_type(bigpc.datafile,FMN_RESTYPE_INSTRUMENT,bigpc.synth->type->data_qualifier,bigpc_cb_load_instrument,0)<0)||
    (fmn_datafile_for_each_of_qualified_type(bigpc.datafile,FMN_RESTYPE_SOUND,bigpc.synth->type->data_qualifier,bigpc_cb_load_sound,0)<0)
  ) {
    fprintf(stderr,"%s: Failed to load instruments for synthesizer '%s'.\n",bigpc.exename,bigpc.synth->type->name);
    return -2;
  }

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

/* Input button and action strings.
 */
 
static uint16_t bigpc_btnid_eval(/*void *userdata,*/const char *src,int srcc) {
  // Button ID tags are all upper-case. Evaluate case-insensitively.
  char norm[16];
  if ((srcc<0)||(srcc>sizeof(norm))) return 0;
  int i=srcc; while (i-->0) if ((src[i]>=0x61)&&(src[i]<=0x7a)) norm[i]=src[i]-0x20; else norm[i]=src[i];
  src=norm;
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return FMN_INPUT_##tag;
  BIGPC_FOR_EACH_BTNID
  #undef _
  if (srcc==4) {
    if (!memcmp(src,"HORZ",4)) return FMN_INPUT_LEFT|FMN_INPUT_RIGHT;
    if (!memcmp(src,"VERT",4)) return FMN_INPUT_UP|FMN_INPUT_DOWN;
    if (!memcmp(src,"DPAD",4)) return FMN_INPUT_LEFT|FMN_INPUT_RIGHT|FMN_INPUT_UP|FMN_INPUT_DOWN;
  }
  if ((srcc==6)&&!memcmp(src,"CHOOSE",6)) return FMN_INPUT_MENU; // some time after committing it, i've come to prefer "CHOOSE" over "MENU".
  return 0;
}

static int bigpc_btnid_repr(char *dst,int dsta,/*void *userdata,*/uint16_t btnid) {
  const char *src=0;
  int srcc=0;
  switch (btnid) {
    #define _(tag) case FMN_INPUT_##tag: src=#tag; srcc=sizeof(#tag)-1; break;
    BIGPC_FOR_EACH_BTNID
    #undef _
    case FMN_INPUT_LEFT|FMN_INPUT_RIGHT: src="horz"; srcc=4; break;
    case FMN_INPUT_UP|FMN_INPUT_DOWN: src="vert"; srcc=4; break;
    case FMN_INPUT_LEFT|FMN_INPUT_RIGHT|FMN_INPUT_UP|FMN_INPUT_DOWN: src="dpad"; srcc=4; break;
  }
  if (!src) return -1;
  if (srcc<=dsta) {
    memcpy(dst,src,srcc);
    if (srcc<dsta) dst[srcc]=0;
  }
  return srcc;
}

static uint16_t bigpc_actionid_eval(/*void *userdata,*/const char *src,int srcc) {
  // Action ID tags are all lower-case. Evaluate case-insensitively.
  char norm[32];
  if ((srcc<0)||(srcc>sizeof(norm))) return 0;
  int i=srcc; while (i-->0) if ((src[i]>=0x41)&&(src[i]<=0x5a)) norm[i]=src[i]+0x20; else norm[i]=src[i];
  src=norm;
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return BIGPC_ACTIONID_##tag;
  BIGPC_FOR_EACH_ACTIONID
  #undef _
  //TODO actionid aliases?
  return 0;
}

static int bigpc_actionid_repr(char *dst,int dsta,/*void *userdata,*/uint16_t actionid) {
  const char *src=0;
  int srcc=0;
  switch (actionid) {
    #define _(tag) case BIGPC_ACTIONID_##tag: src=#tag; srcc=sizeof(#tag)-1; break;
    BIGPC_FOR_EACH_ACTIONID
    #undef _
  }
  if (!src) return -1;
  if (srcc<=dsta) {
    memcpy(dst,src,srcc);
    if (srcc<dsta) dst[srcc]=0;
  }
  return srcc;
}

/* Init input.
 */
 
static void bigpc_input_config_cb_error(const char *msg,int msgc,int lineno,void *userdata) {
  const char *path=userdata;
  fprintf(stderr,"%s:%d: %.*s\n",path,lineno,msgc,msg);
}

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
  
  // Initialize the mapper.
  const struct inmgr_delegate inmgr_delegate={
    .userdata=0,
    .all_btnid=FMN_INPUT_LEFT|FMN_INPUT_RIGHT|FMN_INPUT_UP|FMN_INPUT_DOWN|FMN_INPUT_USE|FMN_INPUT_MENU,
    .state=bigpc_cb_state_change,
    .action=bigpc_cb_action,
    .btnid_eval=bigpc_btnid_eval,
    .btnid_repr=bigpc_btnid_repr,
    .actionid_eval=bigpc_actionid_eval,
    .actionid_repr=bigpc_actionid_repr,
  };
  if (!(bigpc.inmgr=inmgr_new(&inmgr_delegate))) return -1;
  if (bigpc.config.input_path) {
    char *src=0;
    int srcc=fmn_file_read(&src,bigpc.config.input_path);
    if (srcc>=0) {
      int err=inmgr_receive_config(bigpc.inmgr,src,srcc,bigpc_input_config_cb_error,bigpc.config.input_path);
      free(src);
      if (err<0) return -2;
      fprintf(stderr,"%s: Acquired input config.\n",bigpc.config.input_path);
    } else {
      fprintf(stderr,"%s: Failed to read input config. Proceeding with defaults only.\n",bigpc.config.input_path);
    }
  } else {
    fprintf(stderr,"%s: Input config path not provided. We will use defaults and will not be able to save changes.\n",bigpc.exename);
  }
  
  return 0;
}

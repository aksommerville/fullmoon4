#include "bigpc_internal.h"

/* Cleanup configuration.
 */
 
void bigpc_config_cleanup(struct bigpc_config *config) {
  if (config->video_drivers) free(config->video_drivers);
  if (config->audio_drivers) free(config->audio_drivers);
  if (config->input_drivers) free(config->input_drivers);
  if (config->synth_drivers) free(config->synth_drivers);
  if (config->render_drivers) free(config->render_drivers);
  if (config->data_path) free(config->data_path);
  if (config->log_path) free(config->log_path);
  if (config->savedgame_path) free(config->savedgame_path);
}

/* Evaluate primitives.
 */
 
static int bigpc_bool_eval(const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (!srcc) return 0;
  #define _(s,v) if ((srcc==sizeof(s)-1)&&!memcmp(src,s,srcc)) return v;
  _("0",0)
  _("1",1)
  _("false",0)
  _("true",1)
  _("off",0)
  _("on",1)
  #undef _
  return -1;
}

static int bigpc_int_eval(int *dst,const char *src,int srcc) {
  if (!src||!dst) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int positive=1,srcp=0;
  if (srcp>=srcc) return -1;
  if (src[srcp]=='-') {
    positive=0;
    if (++srcp>=srcc) return -1;
  } else if (src[srcp]=='+') {
    if (++srcp>=srcc) return -1;
  }
  *dst=0;
  for (;srcp<srcc;srcp++) {
    int digit=src[srcp]-'0';
    if ((digit<0)||(digit>9)) return -1;
    if (positive) {
      if (*dst>INT_MAX/10) return -1;
      (*dst)*=10;
      if (*dst>INT_MAX-digit) return -1;
      (*dst)+=digit;
    } else {
      if (*dst<INT_MIN/10) return -1;
      (*dst)*=10;
      if (*dst<INT_MIN+digit) return -1;
      (*dst)-=digit;
    }
  }
  return 0;
}

static int bigpc_video_renderer_eval(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return BIGPC_RENDERER_##tag;
  BIGPC_RENDERER_FOR_EACH
  #undef _
  return -1;
}

static int bigpc_audio_format_eval(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return BIGPC_AUDIO_FORMAT_##tag;
  BIGPC_AUDIO_FORMAT_FOR_EACH
  #undef _
  return -1;
}

static int bigpc_lang_eval(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc!=2) return -1;
  if ((src[0]<'a')||(src[0]>'z')) return -1;
  if ((src[1]<'a')||(src[1]>'z')) return -1;
  return (src[0]<<8)|src[1];
}

/* Argv.
 */
 
int bigpc_configure_argv(int argc,char **argv) {
  if (argc<1) return 0;
  if (argv[0]&&argv[0][0]) bigpc.exename=argv[0];
  int argi=1,err;
  while (argi<argc) {
    const char *arg=argv[argi++];
    
    // Ignore null and empty.
    if (!arg) continue;
    if (!arg[0]) continue;
    
    // Anything without a dash is a positional argument, which we don't support any.
    if (arg[0]!='-') {
      fprintf(stderr,"%s: Unexpected argument '%s'\n",bigpc.exename,arg);
      return -2;
    }
    
    // Single dash alone reserved.
    if (!arg[1]) {
      fprintf(stderr,"%s: Unexpected argument '%s'\n",bigpc.exename,arg);
      return -2;
    }
    
    // Short options are single characters, no argument. (implicitly, an argument of "1")
    if (arg[1]!='-') {
      int i=1; for (;arg[i];i++) {
        if ((err=bigpc_configure_kv(arg+i,1,"1",1))<0) {
          if (err!=-2) fprintf(stderr,"%s: Error processing short option '%c'\n",bigpc.exename,arg[i]);
          return -2;
        }
      }
      continue;
    }
    
    // Double dash alone reserved.
    if (!arg[2]) {
      fprintf(stderr,"%s: Unexpected argument '%s'\n",bigpc.exename,arg);
      return -2;
    }
    
    // Long options can be "--key" (="1"), "--no-key" (="0"), "--key=value" or "--key" "value".
    const char *k=arg+2;
    while (k[0]=='-') k++; // strip extra leading dashes, whatever
    int kc=0;
    while (k[kc]&&(k[kc]!='=')) kc++;
    const char *v=0;
    if (k[kc]=='=') v=k+kc+1;
    else if ((kc>=3)&&!memcmp(k,"no-",3)) { k+=3; kc-=3; v="0"; }
    else if ((argi<argc)&&argv[argi]&&argv[argi][0]&&(argv[argi][0]!='-')) v=argv[argi++];
    else v="1";
    if ((err=bigpc_configure_kv(k,kc,v,-1))<0) {
      if (err!=-2) fprintf(stderr,"%s: Error processing long option '%.*s' = '%s'\n",bigpc.exename,kc,k,v);
      return -2;
    }
  }
  return 0;
}

/* --help
 */
 
static void bigpc_print_help(const char *topic,int topicc) {
  fprintf(stderr,
    "\n"
    "Usage: %s [OPTIONS]\n"
    "\n"
  ,bigpc.exename);
  fprintf(stderr,
    "Usually best to leave all options unset.\n"
    "\n"
    "OPTIONS:\n"
    "  --help                Print this message.\n"
    "  --video=LIST          Use first available video driver (see below).\n"
    "  --fullscreen=0|1      [0] Start in fullscreen.\n"
    "  --video-renderer=?    Don't use.\n"
    "  --tilesize=1..64      [16] Best not to touch.\n"
    "  --input=LIST          Use all specified input drivers (see below).\n"
    "  --audio=LIST          Use first available audio driver (see below).\n"
    "  --audio-rate=HZ       [44100] Audio output rate.\n"
    "  --audio-chanc=INT     [1] Audio channel count, typically 1 or 2.\n"
    "  --audio-format=?      [s16n] Audio sample format: s16n,f32n\n"
    "  --synth=LIST          Use first available synthesizer (see below).\n"
    "  --render=LIST         Use first available renderer (see below).\n"
    "  --data=PATH           Path to data archive.\n"
    "  --log=PATH_PREFIX     Business log.\n"
    "  --save=PATH           Saved game.\n"
    "  --lang=iso631         Two-character language code. Prefer LANG env var.\n"
    "\n"
  );
  
  int i;
  fprintf(stderr,"Video drivers:\n");
  const struct bigpc_video_type *video;
  for (i=0;video=bigpc_video_type_by_index(i);i++) fprintf(stderr,"  %-15s %s\n",video->name,video->desc);
  fprintf(stderr,"\n");
  fprintf(stderr,"Renderers:\n");
  const struct bigpc_render_type *render;
  for (i=0;render=bigpc_render_type_by_index(i);i++) fprintf(stderr,"  %-15s %s\n",render->name,render->desc);
  fprintf(stderr,"\n");
  fprintf(stderr,"Audio drivers:\n");
  const struct bigpc_audio_type *audio;
  for (i=0;audio=bigpc_audio_type_by_index(i);i++) fprintf(stderr,"  %-15s %s\n",audio->name,audio->desc);
  fprintf(stderr,"\n");
  fprintf(stderr,"Synthesizers:\n");
  const struct bigpc_synth_type *synth;
  for (i=0;synth=bigpc_synth_type_by_index(i);i++) fprintf(stderr,"  %-15s %s\n",synth->name,synth->desc);
  fprintf(stderr,"\n");
  fprintf(stderr,"Input drivers (may select more than one):\n");
  const struct bigpc_input_type *input;
  for (i=0;input=bigpc_input_type_by_index(i);i++) fprintf(stderr,"  %-15s %s\n",input->name,input->desc);
  fprintf(stderr,"\n");
}

/* Key=value.
 */
 
int bigpc_configure_kv(const char *k,int kc,const char *v,int vc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  
  if ((kc==4)&&!memcmp(k,"help",4)) {
    bigpc_print_help(v,vc);
    return -2;
  }
  
  #define STRINGOPT(arg,fld) if ((kc==sizeof(arg)-1)&&!memcmp(k,arg,kc)) { \
    if (bigpc.config.fld) { \
      free(bigpc.config.fld); \
      bigpc.config.fld=0; \
    } \
    if (vc) { \
      if (!(bigpc.config.fld=malloc(vc+1))) return -1; \
      memcpy(bigpc.config.fld,v,vc); \
      bigpc.config.fld[vc]=0; \
    } \
    return 0; \
  }
  
  #define BOOLOPT(arg,fld) if ((kc==sizeof(arg)-1)&&!memcmp(k,arg,kc)) { \
    if ((bigpc.config.fld=bigpc_bool_eval(v,vc))<0) { \
      fprintf(stderr,"%s: Expected boolean for option '%.*s', found '%.*s'\n",bigpc.exename,kc,k,vc,v); \
      return -2; \
    } \
    return 0; \
  }
  
  #define INTOPT(arg,fld,lo,hi) if ((kc==sizeof(arg)-1)&&!memcmp(k,arg,kc)) { \
    int n; \
    if (bigpc_int_eval(&n,v,vc)<0) { \
      fprintf(stderr,"%s: Expected integer for option '%.*s', found '%.*s'\n",bigpc.exename,kc,k,vc,v); \
      return -2; \
    } \
    if ((n<lo)||(n>hi)) { \
      fprintf(stderr,"%s: Value %d out of range (%d..%d) for option '%.*s'\n",bigpc.exename,n,lo,hi,kc,k); \
      return -2; \
    } \
    bigpc.config.fld=n; \
    return 0; \
  }
  
  #define ENUMOPT(arg,fld,eval) if ((kc==sizeof(arg)-1)&&!memcmp(k,arg,kc)) { \
    if ((bigpc.config.fld=eval(v,vc))<0) { \
      fprintf(stderr,"%s: Unknown value '%.*s' for option '%.*s'\n",bigpc.exename,vc,v,kc,k); \
      return -2; \
    } \
    return 0; \
  }
  
  STRINGOPT("video",video_drivers)
  BOOLOPT("fullscreen",video_fullscreen)
  ENUMOPT("video-renderer",video_renderer,bigpc_video_renderer_eval)
  INTOPT("tilesize",tilesize,1,64)
  
  STRINGOPT("input",input_drivers)
  
  STRINGOPT("audio",audio_drivers)
  INTOPT("audio-rate",audio_rate,200,200000)
  INTOPT("audio-chanc",audio_chanc,1,8)
  ENUMOPT("audio-format",audio_format,bigpc_audio_format_eval)
  
  STRINGOPT("synth",synth_drivers)
  
  STRINGOPT("render",render_drivers)
  
  STRINGOPT("data",data_path)
  STRINGOPT("log",log_path)
  STRINGOPT("save",savedgame_path)
  
  ENUMOPT("lang",lang,bigpc_lang_eval)
  
  // Remember to update bigpc_print_help (just above) if you change anything.
  
  #undef STRINGOPT
  #undef BOOLOPT
  #undef INTOPT
  #undef ENUMOPT
  fprintf(stderr,"%s: Unexpected option '%.*s'\n",bigpc.exename,kc,k);
  return -2;
}

/* Initial defaults.
 */
 
void bigpc_config_init() {
  bigpc.config.video_fullscreen=1;
}

/* Guess language from environment.
 */
 
static uint16_t bigpc_guess_language() {

  // First two characters of LANG are usually what we're after.
  const char *LANG=getenv("LANG");
  if (LANG&&LANG[0]&&LANG[1]) {
    char a=LANG[0]; if ((a>='A')&&(a<='Z')) a+=0x20;
    char b=LANG[1]; if ((b>='A')&&(b<='Z')) b+=0x20;
    if ((a>='a')&&(a<='z')&&(b>='a')&&(b<='z')) return (a<<8)|b;
  }

  // English is Full Moon's native language, and also the most widely spoken language on Earth*. Good for a default.
  // [*] TODO Confirm preferred language on other planets.
  return ('e'<<8)|'n';
}

/* Finish configuration.
 */
 
int bigpc_config_ready() {
  
  if (!bigpc.config.tilesize) bigpc.config.tilesize=16;
  
  if (!bigpc.config.audio_rate) bigpc.config.audio_rate=44100;
  if (!bigpc.config.audio_chanc) bigpc.config.audio_chanc=1;
  if (!bigpc.config.audio_format) bigpc.config.audio_format=BIGPC_AUDIO_FORMAT_s16n;
  
  // Choice of renderer is messy: Is it video's problem or is it its own thing?
  // I didn't answer that question clearly at first, and have wobbled back and forth on it since.
  // It's ok to leave (render_drivers) unset when (video_renderer) is set; we'll try everything and only accept the matching ones.
  // But the opposite case -- (render_drivers) set but (video_renderer) unset -- we need to infer a value for video_renderer.
  // And to make things worse, (render_drivers) is a list. We will proceed sensibly only if it's just one item.
  if (bigpc.config.render_drivers&&!bigpc.config.video_renderer) {
    const struct bigpc_render_type *type=bigpc_render_type_by_name(bigpc.config.render_drivers,-1);
    if (type) {
      bigpc.config.video_renderer=type->video_renderer_id;
    }
  }
  
  if (!bigpc.config.lang) bigpc.config.lang=bigpc_guess_language();
  
  return 0;
}

/* Guess data_path.
 */
 
int bigpc_config_guess_data_path() {
  if (bigpc.config.data_path) return 0;
  
  // If exename contains a slash, try its dirname plus "/data".
  int eslashp=-1;
  int i=0;
  for (;bigpc.exename[i];i++) {
    if (bigpc.exename[i]=='/') eslashp=i;
  }
  if (eslashp>=0) {

    // Little different for Macs: If dir ends "/Contents/MacOS", try ".../Contents/Resources/data"
    if ((eslashp>=15)&&!memcmp(bigpc.exename+eslashp-15,"/Contents/MacOS",15)) {
      eslashp-=5;
      int pathlen=eslashp+19;
      if (!(bigpc.config.data_path=malloc(pathlen+1))) return -1;
      memcpy(bigpc.config.data_path,bigpc.exename,eslashp);
      memcpy(bigpc.config.data_path+eslashp,"Resources/data-demo",20);
      fprintf(stderr,"%s: '--data=PATH' not provided. Guessing '%s'\n",bigpc.exename,bigpc.config.data_path);
      return 0;
    }
  
    int pathlen=eslashp+10;
    if (!(bigpc.config.data_path=malloc(pathlen+1))) return -1;
    memcpy(bigpc.config.data_path,bigpc.exename,eslashp);
    memcpy(bigpc.config.data_path+eslashp,"/data-full",11);
    fprintf(stderr,"%s: '--data=PATH' not provided. Guessing '%s'\n",bigpc.exename,bigpc.config.data_path);
    return 0;
  }
  
  // Doubtless plenty of other guesses we could make, but I'm leaving it here for now.
  return -1;
}

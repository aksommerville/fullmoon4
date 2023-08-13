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
  if (config->input_path) free(config->input_path);
  if (config->settings_path) free(config->settings_path);
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
    "  --data=PATH           [EXE/data] Path to data archive.\n"
    "  --log=PATH_PREFIX     Business log.\n"
    "  --save=PATH           [EXE/save] Saved game.\n"
    "  --lang=iso631         Two-character language code. Prefer LANG env var.\n"
    "  --input-config=PATH   [EXE/input] Mappings for input devices.\n"
    "  --settings=PATH       [EXE/settings] Persistent settings.\n"
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
  STRINGOPT("input-config",input_path)
  STRINGOPT("settings",settings_path)
  
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

/* Directory containing our executable, including the trailing slash.
 * "./" by default. Never fails.
 * Since this is definitely contained by (bigpc.exename) or a static string, we return it directly.
 * "exe_dirname" for the honest-to-goodness executable.
 * "app_dirname" is the ".app" bundle instead on MacOS, or the same as "exe_dirname" in the civilized world.
 */
 
static int bigpc_config_get_exe_dirname(void *dstpp_const) {
  if (!bigpc.exename||!bigpc.exename[0]) {
    *(void**)dstpp_const="./";
    return 2;
  }
  int slashp=-1,i=0;
  for (;bigpc.exename[i];i++) {
    if ((bigpc.exename[i]=='/')||(bigpc.exename[i]=='\\')) slashp=i;
  }
  if (slashp<0) {
    *(void**)dstpp_const="./";
    return 2;
  }
  *(const void**)dstpp_const=bigpc.exename;
  return slashp+1;
}

static int bigpc_config_get_app_dirname(void *dstpp_const) {
  const char *path=0;
  int pathc=bigpc_config_get_exe_dirname(&path);
  *(const void**)dstpp_const=path;
  
  // If it ends ".app/Contents/MacOS/", trim that and then trim back to the next separator.
  // Presumably it's "FullMoon.app", but users are free to rename that.
  // (if they rename to something that doesn't end ".app", they get what they deserve).
  if ((pathc>=20)&&!memcmp(path+pathc-20,".app/Contents/MacOS/",20)) {
    pathc-=20;
    while (pathc&&(path[pathc-1]!='/')) pathc--;
    if (!pathc) {
      *(void**)dstpp_const="./";
      return 2;
    }
  }
  
  return pathc;
}

/* Concatenate path, only if they fit, including the uncounted terminator.
 */
 
static int bigpc_concat_paths(char *dst,int dsta,const char *pfx,int pfxc,const char *sfx,int sfxc) {
  if (!pfx) pfxc=0; else if (pfxc<0) { pfxc=0; while (pfx[pfxc]) pfxc++; }
  if (!sfx) sfxc=0; else if (sfxc<0) { sfxc=0; while (sfx[sfxc]) sfxc++; }
  if (!pfxc) {
    if (sfxc>=dsta) return 0;
    memcpy(dst,sfx,sfxc);
    dst[sfxc]=0;
    return sfxc;
  }
  if (!sfxc) {
    if (pfxc>=dsta) return 0;
    memcpy(dst,pfx,pfxc);
    dst[pfxc]=0;
    return pfxc;
  }
  int dstc=pfxc+sfxc;
  int need_separator=(pfx[pfxc-1]!='/')&&(sfx[0]!='/');
  if (need_separator) dstc++;
  if (dstc>=dsta) return 0;
  memcpy(dst,pfx,pfxc);
  if (need_separator) {
    dst[pfxc]='/';
    memcpy(dst+pfxc+1,sfx,sfxc);
  } else memcpy(dst+pfxc,sfx,sfxc);
  dst[dstc]=0;
  return dstc;
}

/* Concatenate path and copy to a fresh buffer.
 * Optionally, proceed only if the file exists.
 */
 
static char *bigpc_alloconcat(const char *pfx,int pfxc,const char *sfx,int sfxc,char require_type) {
  char tmp[1024];
  int tmpc=bigpc_concat_paths(tmp,sizeof(tmp),pfx,pfxc,sfx,sfxc);
  if ((tmpc<1)||(tmpc>=sizeof(tmp))) return 0;
  if (require_type&&(fmn_file_get_type(tmp)!=require_type)) return 0;
  char *dst=malloc(tmpc+1);
  if (!dst) return 0;
  memcpy(dst,tmp,tmpc+1);
  return dst;
}

/* Create directory if it doesn't exist.
 */
 
static int bigpc_require_directory(const char *path) {
  switch (fmn_file_get_type(path)) {
    case 0: return fmn_mkdir(path);
    case 'd': return 0;
    default: return -1;
  }
}

/* Directory optionally containing config files and such.
 * Only reports success if it fit in (dsta).
 */
 
static int bigpc_config_get_config_dirname(char *dst,int dsta) {
  if (!dst||(dsta<1)) return 0;
  int dstc;
  
  const char *home=getenv("HOME");
  if (home&&home[0]) {
    if (((dstc=bigpc_concat_paths(dst,dsta,home,-1,".fullmoon/",-1))>0)&&(bigpc_require_directory(dst)>=0)) return dstc;
  }
  
  return 0;
}

/* Guess data_path.
 * Logs all errors.
 */
 
static char *bigpc_config_guess_data_path() {
  char *result;
  const char *dir=0;
  int dirc=bigpc_config_get_exe_dirname(&dir);
  if (result=bigpc_alloconcat(dir,dirc,"data",4,'f')) return result;
  if (result=bigpc_alloconcat(dir,dirc,"data-full",9,'f')) return result;
  if (result=bigpc_alloconcat(dir,dirc,"data-demo",9,'f')) return result;
  if ((dirc>=16)&&!memcmp(dir+dirc-16,"/Contents/MacOS/",16)) { // Macs: Replace "/Contents/MacOS/" with "/Contents/Resources/" and try there.
    int contentsc=dirc-6;
    if (result=bigpc_alloconcat(dir,contentsc,"Resources/data",14,'f')) return result;
    if (result=bigpc_alloconcat(dir,contentsc,"Resources/data-full",19,'f')) return result;
    if (result=bigpc_alloconcat(dir,contentsc,"Resources/data-demo",19,'f')) return result;
  }
  fprintf(stderr,"%s: Failed to locate data file. Please launch with '--data=PATH'.\n",bigpc.exename);
  return 0;
}

/* Guess optional paths.
 * Errors do not necessarily log; null result is ok.
 */
 
static char *bigpc_config_guess_input_path() {
  const char *dir=0;
  int dirc=bigpc_config_get_app_dirname(&dir);
  return bigpc_alloconcat(dir,dirc,"input",5,0);
}

static char *bigpc_config_guess_settings_path() {
  const char *dir=0;
  int dirc=bigpc_config_get_app_dirname(&dir);
  return bigpc_alloconcat(dir,dirc,"settings",8,0);
}

static char *bigpc_config_guess_savedgame_path() {
  const char *dir=0;
  int dirc=bigpc_config_get_app_dirname(&dir);
  return bigpc_alloconcat(dir,dirc,"save",4,0);
}

static char *bigpc_config_guess_log_path() {
  /* Business logging, in general I only enable during play tests. If we do set something here, the actual file will have a timestamp appended. *
  const char *dir=0;
  int dirc=bigpc_config_get_app_dirname(&dir);
  return bigpc_alloconcat(dir,dirc,"log",3);
  /**/
  return 0;
}

/* Guess paths if they weren't provided at command line.
 */
 
static int bigpc_config_require_paths() {

  // (data_path) is required. Fail if we can't find it.
  if (bigpc.config.data_path) ;
  else if (!(bigpc.config.data_path=bigpc_config_guess_data_path())) return -2;
  
  // All others are optional, and won't involve much guesswork. We create them if they don't exist.
  if (!bigpc.config.input_path) bigpc.config.input_path=bigpc_config_guess_input_path();
  if (!bigpc.config.settings_path) bigpc.config.settings_path=bigpc_config_guess_settings_path();
  if (!bigpc.config.savedgame_path) bigpc.config.savedgame_path=bigpc_config_guess_savedgame_path();
  if (!bigpc.config.log_path) ; bigpc.config.log_path=bigpc_config_guess_log_path();
  
  return 0;
}

/* Finish configuration.
 */
 
int bigpc_config_ready() {

  int err=bigpc_config_require_paths();
  if (err<0) return err;
  
  //TODO if there is a settings file, read it now.
  
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

/* Encode and decode settings.
 */
 
static int bigpc_settings_encode(void *dstpp) {
  char tmp[256];
  int tmpc=snprintf(tmp,sizeof(tmp),
    "fullscreen=%d\nmusic=%d\nlang=%c%c\n",
    bigpc.settings.fullscreen_enable,
    bigpc.settings.music_enable,
    bigpc.settings.language>>8,bigpc.settings.language&0xff
  );
  if ((tmpc<0)||(tmpc>=sizeof(tmp))) return -1;
  char *nv=malloc(tmpc+1);
  if (!nv) return -1;
  memcpy(nv,tmp,tmpc+1);
  *(void**)dstpp=nv;
  return tmpc;
}

static int bigpc_settings_eval_bool(const char *src,int srcc) {
  for (;srcc-->0;src++) {
    if (*src!='0') return 1;
  }
  return 0;
}

static int bigpc_settings_decode_line(struct fmn_platform_settings *settings,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp>=srcc) return 0;
  const char *k=src+srcp;
  int kc=0;
  while ((srcp<srcc)&&(src[srcp++]!='=')) kc++;
  while (kc&&((unsigned char)k[kc-1]<=0x20)) kc--;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *v=src+srcp;
  int vc=srcc-srcp;
  while (vc&&((unsigned char)v[vc-1]<=0x20)) vc--;
  
  if ((kc==10)&&!memcmp(k,"fullscreen",10)) {
    if (bigpc.video->type->set_fullscreen) {
      settings->fullscreen_enable=bigpc_settings_eval_bool(v,vc);
      if (settings->fullscreen_enable!=bigpc.video->fullscreen) {
        bigpc.video->type->set_fullscreen(bigpc.video,settings->fullscreen_enable);
      }
    }
    return 0;
  }
  
  if ((kc==5)&&!memcmp(k,"music",5)) {
    if (bigpc.synth->type->enable_music) {
      settings->music_enable=bigpc_settings_eval_bool(v,vc);
      if (settings->music_enable!=bigpc.synth->music_enable) {
        bigpc.synth->type->enable_music(bigpc.synth,settings->music_enable);
      }
    }
    return 0;
  }
  
  if ((kc==4)&&!memcmp(k,"lang",4)) {
    if ((vc==2)&&(v[0]>='a')&&(v[0]<='z')&&(v[1]>='a')&&(v[1]<='z')) {
      uint16_t lang=(v[0]<<8)|v[1];
      if (lang!=settings->language) {
        int i=bigpc.langc;
        while (i-->0) {
          if (bigpc.langv[i]==lang) {
            settings->language=lang;
            bigpc.config.lang=lang;
            fmn_language_changed();
            break;
          }
        }
      }
    }
    return 0;
  }
  
  fprintf(stderr,"%s: Unexpected settings key '%.*s'\n",bigpc.config.settings_path,kc,k);
  return 0;
}

static int bigpc_settings_decode(struct fmn_platform_settings *settings,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    const char *line=src+srcp;
    int linec=0,comment=0;
    while ((srcp<srcc)&&(src[srcp++]!=0x0a)) {
      if (src[srcp-1]=='#') comment=1;
      else if (!comment) linec++;
    }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    if (!linec) continue;
    int err=bigpc_settings_decode_line(settings,line,linec);
    if (err<0) return err;
  }
  return 0;
}

/* Initialize app-visible settings from live state.
 */
 
void bigpc_settings_init() {
  bigpc.settings.fullscreen_available=bigpc.video->type->set_fullscreen?1:0;
  bigpc.settings.fullscreen_enable=bigpc.video->fullscreen?1:0;
  bigpc.settings.music_available=bigpc.synth->type->enable_music?1:0;
  bigpc.settings.music_enable=bigpc.synth->music_enable?1:0;
  bigpc.settings.language=bigpc.config.lang;
  
  if (bigpc.config.settings_path) {
    char *serial=0;
    int serialc=fmn_file_read(&serial,bigpc.config.settings_path);
    if (serialc<0) {
      fprintf(stderr,"%s: Failed to read file, using default settings.\n",bigpc.config.settings_path);
    } else {
      if (bigpc_settings_decode(&bigpc.settings,serial,serialc)<0) {
        fprintf(stderr,"%s: Failed to decode or apply settings.\n",bigpc.config.settings_path);
      }
      free(serial);
    }
  }
}

/* Save settings.
 */
 
void bigpc_settings_dirty() {
  bigpc.settings_dirty=1;
}

void bigpc_settings_save_if_dirty() {
  if (!bigpc.settings_dirty) return;
  bigpc.settings_dirty=0;
  if (!bigpc.config.settings_path) return;
  char *serial=0;
  int serialc=bigpc_settings_encode(&serial);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to encode settings!\n",bigpc.exename);
    return;
  }
  if (fmn_file_write(bigpc.config.settings_path,serial,serialc)<0) {
    fprintf(stderr,"%s: Failed to save settings!\n",bigpc.config.settings_path);
  } else {
    fprintf(stderr,"%s: Saved settings\n",bigpc.config.settings_path);
  }
  free(serial);
}

/* Save input configuration.
 */
 
int bigpc_save_input_config() {
  if (!bigpc.config.input_path) return 0;
  char *src=0;
  int srcc=inmgr_generate_config(&src,bigpc.inmgr);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to reencode input config! Changes will not be saved.\n",bigpc.exename);
    return -2;
  }
  int err=fmn_file_write(bigpc.config.input_path,src,srcc);
  free(src);
  if (err<0) {
    fprintf(stderr,"%s: Failed to write input config file! Changes will not be saved.\n",bigpc.config.input_path);
    return -2;
  }
  return 0;
}

#include "fiddle_internal.h"

/* --help
 */
 
static void fiddle_print_help(const char *topic,int topicc) {
  fprintf(stderr,
    "\n"
    "Usage: %s [OPTIONS]\n"
    "\n"
    "OPTIONS:\n"
    "  --help          Print this message and abort.\n"
    "  --port=INT      TCP port to listen on. [1644]\n"
    "  --htdocs=PATH   Source for static files. [src/tool/fiddlec/www]\n"
    "  --data=PATH     Data files.\n"
    "\n"
  ,fiddle.exename);
}

/* Set one config field.
 */
 
static int fiddle_config_kv(const char *k,int kc,const char *v,int vc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  
  if ((kc==4)&&!memcmp(k,"help",4)) {
    fiddle_print_help(v,vc);
    return -2;
  }
  
  #define INTARG(arg,fld,lo,hi) if ((kc==sizeof(arg)-1)&&!memcmp(k,arg,kc)) { \
    int vn; \
    if (sr_int_eval(&vn,v,vc)<2) { \
      fprintf(stderr,"%s: Failed to evaluate '%.*s' as integer for option '%.*s'.\n",fiddle.exename,vc,v,kc,k); \
      return -2; \
    } \
    if ((vn<lo)||(vn>hi)) { \
      fprintf(stderr,"%s: Value %d out of range %d..%d for option '%.*s'.\n",fiddle.exename,vn,lo,hi,kc,k); \
      return -2; \
    } \
    fiddle.fld=vn; \
    return 0; \
  }
  #define STRARG(arg,fld) if ((kc==sizeof(arg)-1)&&!memcmp(k,arg,kc)) { \
    char *nv=0; \
    if (vc) { \
      if (!(nv=malloc(vc+1))) return -1; \
      memcpy(nv,v,vc); \
      nv[vc]=0; \
    } \
    if (fiddle.fld) free(fiddle.fld); \
    fiddle.fld=nv; \
    fiddle.fld##c=vc; \
    return 0; \
  }
  
  INTARG("port",port,1,65535)
  STRARG("htdocs",htdocs)
  STRARG("data",data)
  
  #undef INTARG
  #undef STRARG
  fprintf(stderr,"%s: Unknown option '%.*s'\n",fiddle.exename,kc,k);
  return -2;
}

/* Initial defaults.
 */
 
static int fiddle_config_init(const char *exename) {
  int err;
  if (exename&&exename[0]) fiddle.exename=exename;
  else fiddle.exename="fiddle";
  
  fiddle.port=1644;
  if ((err=fiddle_config_kv("htdocs",6,"src/tool/fiddlec/www",-1))<0) return err;
  
  return 0;
}

/* Finalize config.
 */
 
static int fiddle_config_finish() {
  return 0;
}

/* Read argv. First item should be stripped off.
 */
 
static int fiddle_config_argv(int argc,char **argv) {
  int argi=0,err;
  while (argi<argc) {
    const char *arg=argv[argi++];
    if (!arg||!arg[0]) continue;
    
    // No dashes: Positional args. But we don't define any.
    if (arg[0]!='-') goto _unexpected_;
    
    // Single dash alone: Undefined.
    if (!arg[1]) goto _unexpected_;
    
    // Single dash: One argument "-kVV" or "-k VV".
    if (arg[1]!='-') {
      const char *k=arg+1;
      const char *v=0;
      if (arg[2]) v=arg+2;
      else if ((argi<argc)&&argv[argi]&&argv[argi][0]&&(argv[argi][0]!='-')) v=argv[argi++];
      if ((err=fiddle_config_kv(k,1,v,-1))<0) {
        if (err!=-2) fprintf(stderr,"%s: Unspecified error processing option '%c' = '%s'\n",fiddle.exename,k[0],v?v:"");
        return -2;
      }
      continue;
    }
    
    // Double dash alone: Undefined.
    if (!arg[2]) goto _unexpected_;
    
    // Double dash: One argument "--kk=vv" or "--kk vv".
    const char *k=arg+2;
    while (*k=='-') k++; // or triple dash, or quadruple, ...
    int kc=0;
    while (k[kc]&&(k[kc]!='=')) kc++;
    const char *v=0;
    if (k[kc]=='=') v=k+kc+1;
    else if ((argi<argc)&&argv[argi]&&argv[argi][0]&&(argv[argi][0]!='-')) v=argv[argi++];
    if ((err=fiddle_config_kv(k,kc,v,-1))<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error processing option '%.*s' = '%s'\n",fiddle.exename,kc,k,v?v:"");
      return -2;
    }
    continue;
    
   _unexpected_:
    fprintf(stderr,"%s: Unexpected argument '%s'\n",fiddle.exename,arg);
    return -2;
  }
  return 0;
}

/* Configure app, main entry point.
 */
 
int fiddle_configure(int argc,char **argv) {
  int err;
  if ((err=fiddle_config_init((argc>=1)?argv[0]:""))<0) return err;
  if ((err=fiddle_config_argv(argc-1,argv+1))<0) return err;
  if ((err=fiddle_config_finish())<0) return err;
  return 0;
}

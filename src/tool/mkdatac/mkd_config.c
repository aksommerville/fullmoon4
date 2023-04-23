#include "mkd_internal.h"

/* Cleanup.
 */

void mkd_config_cleanup(struct mkd_config *config) {
  if (!config) return;
  if (config->dstpath) free(config->dstpath);
  if (config->srcpathv) {
    while (config->srcpathc-->0) free(config->srcpathv[config->srcpathc]);
    free(config->srcpathv);
  }
  if (config->argv) {
    struct mkd_arg *arg=config->argv;
    int i=config->argc;
    for (;i-->0;arg++) {
      if (arg->k) free(arg->k);
      if (arg->v) free(arg->v);
    }
    free(config->argv);
  }
}

/* --help
 */
 
static void mkd_config_print_help(struct mkd_config *config,const char *topic,int topicc) {
  fprintf(stderr,
    "Usage: %s --single -oOUTPUT INPUT\n"
    "   Or: %s --archive -oOUTPUT [INPUT...]\n"
    "   Or: %s --showtoc --in=ARCHIVE\n"
    "   Or: %s --extract -oOUTPUT --in=ARCHIVE --type=INT --qualifier=INT --id=INT\n",
    config->exename,config->exename,config->exename,config->exename
  );
}

/* Positional argument.
 * These will go directly into (srcpathv).
 * Other handling is possible if a need arises.
 */
 
static int mkd_config_positional(struct mkd_config *config,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (config->srcpathc>=config->srcpatha) {
    int na=config->srcpatha+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(config->srcpathv,sizeof(void*)*na);
    if (!nv) return -1;
    config->srcpathv=nv;
    config->srcpatha=na;
  }
  char *nv=malloc(srcc+1);
  if (!nv) return -1;
  memcpy(nv,src,srcc);
  nv[srcc]=0;
  config->srcpathv[config->srcpathc++]=nv;
  return 0;
}

/* Key=value argument.
 */
 
static int mkd_config_kv(struct mkd_config *config,const char *k,int kc,const char *v,int vc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  
  // We manage the special argument "help".
  if ((kc==4)&&!memcmp(k,"help",4)) {
    mkd_config_print_help(config,v,vc);
    return -2;
  }
  
  // We also manage "-o" for dstpath.
  if ((kc==1)&&(k[0]=='o')) {
    if (config->dstpath) {
      fprintf(stderr,"%s: Multiple output paths.\n",config->exename);
      return -2;
    }
    if (!(config->dstpath=malloc(vc+1))) return -1;
    memcpy(config->dstpath,v,vc);
    config->dstpath[vc]=0;
    return 0;
  }
  
  // "--in=" is equivalent to positional arguments. Necessary sometimes to avoid getting captured as an argument to some other option.
  if ((kc==2)&&!memcmp(k,"in",2)) {
    return mkd_config_positional(config,v,vc);
  }
  
  // If (k) is a command name and (v) empty, read it as (cmd).
  if (!vc) {
    int setcmd=0;
    #define _(tag) if ((kc==sizeof(#tag)-1)&&!memcmp(k,#tag,kc)) setcmd=MKD_CMD_##tag; else
    MKD_FOR_EACH_CMD ;
    #undef _
    if (setcmd) {
      if (config->cmd==setcmd) return 0;
      if (config->cmd) {
        fprintf(stderr,"%s: Multiple commands.\n",config->exename);
        return -2;
      }
      config->cmd=setcmd;
      return 0;
    }
  }
  
  // Try to evaluate as integer, zero if it fails.
  // Signed decimal integers only.
  int vn=0;
  if (!vc) {
    if ((kc>=3)&&!memcmp(k,"no-",3)) { v="0"; vc=1; vn=0; k+=3; kc-=3; }
    else { v="1"; vc=1; vn=1; }
  } else {
    int vp=0,positive=1;
    if (v[vp]=='-') { positive=0; vp++; }
    else if (v[vp]=='+') vp++;
    if (vp<vc) {
      for (;vp<vc;vp++) {
        int digit=v[vp]-'0';
        if ((digit<0)||(digit>9)) { vn=0; break; }
        if (positive) {
          if (vn>INT_MAX/10) vn=0; else vn*=10;
          if (vn>INT_MAX-digit) vn=0; else vn+=digit;
        } else {
          if (vn<INT_MIN/10) vn=0; else vn*=10;
          if (vn<INT_MIN+digit) vn=0; else vn-=digit;
        }
      }
    }
  }
  
  // Add to list.
  if (config->argc>=config->arga) {
    int na=config->arga+8;
    if (na>INT_MAX/sizeof(struct mkd_arg)) return -1;
    void *nv=realloc(config->argv,sizeof(struct mkd_arg)*na);
    if (!nv) return -1;
    config->argv=nv;
    config->arga=na;
  }
  char *nk=malloc(kc+1);
  if (!nk) return -1;
  char *nv=malloc(vc+1);
  if (!nv) { free(nk); return -1; }
  memcpy(nk,k,kc); nk[kc]=0;
  memcpy(nv,v,vc); nv[vc]=0;
  struct mkd_arg *arg=config->argv+config->argc++;
  arg->k=nk;
  arg->kc=kc;
  arg->v=nv;
  arg->vc=vc;
  arg->vn=vn;
  return 0;
}

/* Receive argv.
 */
 
int mkd_config_argv(struct mkd_config *config,int argc,char **argv) {
  if ((argc>=1)&&argv&&argv[0]&&argv[0][0]) {
    config->exename=argv[0];
  } else {
    config->exename="mkdatac";
  }
  int argi=1,err;
  while (argi<argc) {
    const char *arg=argv[argi++];
    
    // Blank, pretend we didn't see it.
    if (!arg||!arg[0]) continue;
    
    // No dash, "positional". Should be an input path.
    if (arg[0]!='-') {
      if ((err=mkd_config_positional(config,arg,-1))<0) return err;
      continue;
    }
    
    // Single dash alone, reserved.
    if (!arg[1]) {
      fprintf(stderr,"%s: Unexpected argument '%s'\n",config->exename,arg);
      return -2;
    }
    
    // Single dash, one short argument: -kVVVV or -k VVVV
    if (arg[1]!='-') {
      char k=arg[1];
      const char *v=0;
      if (arg[2]) v=arg+2;
      else if ((argi<argc)&&argv[argi]&&(argv[argi][0]!='-')) v=argv[argi++];
      if ((err=mkd_config_kv(config,&k,1,v,-1))<0) return err;
      continue;
    }
    
    // Double dash alone, reserved.
    if (!arg[2]) {
      fprintf(stderr,"%s: Unexpected argument '%s'\n",config->exename,arg);
      return -2;
    }
    
    // Double dash, one long argument: --kk=VV or --kk VV
    const char *k=arg+2;
    while (*k=='-') k++; // ...or triple dash, or quadruple, ....
    int kc=0;
    while (k[kc]&&(k[kc]!='=')) kc++;
    const char *v=0;
    if (k[kc]=='=') v=k+kc+1;
    else if ((argi<argc)&&argv[argi]&&(argv[argi][0]!='-')) v=argv[argi++];
    if ((err=mkd_config_kv(config,k,kc,v,-1))<0) return err;
  }
  return 0;
}

/* Finalize config.
 */
 
int mkd_config_ready(struct mkd_config *config) {
  //TODO
  return 0;
}

/* Get argument.
 */
 
int mkd_config_get_arg_string(void *dstpp,const struct mkd_config *config,const char *k,int kc) {
  if (!k) return -1;
  if (kc<0) { kc=0; while (k[kc]) kc++; }
  const struct mkd_arg *arg=config->argv;
  int i=config->argc;
  for (;i-->0;arg++) {
    if (arg->kc!=kc) continue;
    if (memcmp(arg->k,k,kc)) continue;
    if (dstpp) *(void**)dstpp=arg->v;
    return arg->vc;
  }
  return -1;
}

int mkd_config_get_arg_int(const struct mkd_config *config,const char *k,int kc) {
  if (!k) return 0;
  if (kc<0) { kc=0; while (k[kc]) kc++; }
  const struct mkd_arg *arg=config->argv;
  int i=config->argc;
  for (;i-->0;arg++) {
    if (arg->kc!=kc) continue;
    if (memcmp(arg->k,k,kc)) continue;
    return arg->vn;
  }
  return 0;
}

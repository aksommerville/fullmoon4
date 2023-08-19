#include "fiddle_internal.h"

#if FMN_USE_mswin
  // Fiddle is never going to run in Windows, but it's hard not to build it. Just stub things out until it compiles.
  
int fiddle_spawn_process_sync(const char *cmd,int (*cb)(int status,const char *log,int logc,void *userdata),void *userdata) {
  return -1;
}

#else

#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

extern char **environ;

/* Helper to prepare argv for a forked process.
 */
 
static void fiddle_argv_free(char **argv) {
  if (!argv) return;
  char **i=argv;
  for (;*i;i++) free(*i);
  free(argv);
}

static char *fiddle_argv_eval_1(const char *src,int srcc) {
  char *dst=malloc(srcc+1);
  if (!dst) return 0;
  memcpy(dst,src,srcc);
  dst[srcc]=0;
  return dst;
}

static char *fiddle_argv_eval_0(const char *src,int srcc) {
  int i=srcc; while (i-->0) {
    if (src[i]=='/') return fiddle_argv_eval_1(src,srcc);
  }
  const char *path=getenv("PATH");
  if (path) {
    int pathp=0;
    while (path[pathp]) {
      if (path[pathp]==':') { pathp++; continue; }
      const char *pfx=path+pathp;
      int pfxc=0;
      while (path[pathp]&&(path[pathp++]!=':')) pfxc++;
      if (pfxc) {
        char tmp[256];
        if (pfxc+1+srcc<sizeof(tmp)) {
          memcpy(tmp,pfx,pfxc);
          tmp[pfxc]='/';
          memcpy(tmp+pfxc+1,src,srcc);
          tmp[pfxc+1+srcc]=0;
          struct stat st={0};
          if (stat(tmp,&st)>=0) return strdup(tmp);
        }
      }
    }
  }
  return fiddle_argv_eval_1(src,srcc);
}

static char **fiddle_argv_eval(const char *cmd) {
  int arga=8,argc=0;
  char **argv=malloc(sizeof(void*)*arga);
  if (!argv) return 0;
  #define ABORT { \
    while (argc-->0) free(argv[argc]); \
    free(argv); \
    return 0; \
  }
  #define GROW { \
    if (argc>=arga) { \
      arga+=8; \
      void *nv=realloc(argv,sizeof(void*)*arga); \
      if (!nv) ABORT \
      argv=nv; \
    } \
  }
  
  int cmdp=0;
  while (cmd[cmdp]) {
    if ((unsigned char)cmd[cmdp]<=0x20) { cmdp++; continue; }
    GROW
    const char *src=cmd+cmdp;
    int srcc=0;
    while ((unsigned char)cmd[cmdp]>0x20) { srcc++; cmdp++; }
    if (!argc) {
      if (!(argv[argc]=fiddle_argv_eval_0(src,srcc))) ABORT
    } else {
      if (!(argv[argc]=fiddle_argv_eval_1(src,srcc))) ABORT
    }
    argc++;
  }
  
  if (!argc) ABORT
  GROW
  argv[argc]=0;
  #undef GROW
  #undef ABORT
  return argv;
}

/* Spawn process, call-and-response mode.
 */
 
int fiddle_spawn_process_sync(const char *cmd,int (*cb)(int status,const char *log,int logc,void *userdata),void *userdata) {
  if (!cmd||!cb) return -1;
  
  // Copy command, turn spaces into NULs, and split words.
  char **argv=fiddle_argv_eval(cmd);
  if (!argv) return -1;
  
  // Create a pipe, then a new process.
  int fdv[2]={0};
  if (pipe(fdv)<0) {
    fiddle_argv_free(argv);
    return -1;
  }
  int pid=fork();
  if (pid<0) {
    fiddle_argv_free(argv);
    close(fdv[0]);
    close(fdv[1]);
    return -1;
  }
  
  /* Parent process.
   */
  if (pid) {
    close(fdv[1]);
    
    int dsta=1024,dstc=0;
    char *dst=malloc(dsta);
    if (dst) {
      while (1) {
        if (dsta>=dstc) {
          dsta+=1024;
          void *nv=realloc(dst,dsta);
          if (!nv) break;
          dst=nv;
        }
        int err=read(fdv[0],dst+dstc,dsta-dstc);
        if (err<=0) break;
        dstc+=err;
      }
    }
    
    int status=0;
    int err=waitpid(pid,&status,0);
    status=WEXITSTATUS(status);
    
    err=cb(status,dst,dstc,userdata);
    if (dst) free(dst);
    fiddle_argv_free(argv);
    return err;
  
  /* Child process.
   */
  } else {
    close(fdv[0]);
    dup2(fdv[1],STDOUT_FILENO);
    dup2(fdv[1],STDERR_FILENO);
    close(fdv[1]);
    execve(argv[0],argv,environ);
    exit(1);
  
  }
  return 0;
}

#endif

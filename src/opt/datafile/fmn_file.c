#include "fmn_datafile.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#ifndef O_BINARY
  #define O_BINARY 0
#endif

/* Read file.
 */
 
int fmn_file_read(void *dstpp,const char *path) {
  if (!path||!path[0]) return -1;
  int fd=open(path,O_RDONLY|O_BINARY);
  if (fd<0) return -1;
  off_t flen=lseek(fd,0,SEEK_END);
  if ((flen<0)||(flen>INT_MAX)||lseek(fd,0,SEEK_SET)) {
    close(fd);
    return -1;
  }
  char *dst=malloc(flen?flen:1);
  if (!dst) {
    close(fd);
    return -1;
  }
  int dstc=0;
  while (dstc<flen) {
    int err=read(fd,dst+dstc,flen-dstc);
    if (err<=0) {
      close(fd);
      free(dst);
      return -1;
    }
    dstc+=err;
  }
  close(fd);
  *(void**)dstpp=dst;
  return dstc;
}

/* Write file.
 */
 
int fmn_file_write(const char *path,const void *src,int srcc) {
  if (!path||(srcc<0)||(srcc&&!src)) return -1;
  int fd=open(path,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,0666);
  if (fd<0) return -1;
  int srcp=0;
  while (srcp<srcc) {
    int err=write(fd,(char*)src+srcp,srcc-srcp);
    if (err<=0) {
      close(fd);
      unlink(path);
      return -1;
    }
    srcp+=err;
  }
  close(fd);
  return 0;
}

/* Read directory.
 */
 
int fmn_dir_read(const char *path,int (*cb)(const char *path,const char *base,char type,void *userdata),void *userdata) {
  if (!path||!path[0]||!cb) return -1;
  char subpath[1024];
  int pathc=0;
  while (path[pathc]) pathc++;
  if (pathc>=sizeof(subpath)) return -1;
  memcpy(subpath,path,pathc);
  if (path[pathc-1]!='/') subpath[pathc++]='/';
  DIR *dir=opendir(path);
  if (!dir) return -1;
  struct dirent *de;
  while (de=readdir(dir)) {
    const char *base=de->d_name;
    int basec=0;
    while (base[basec]) basec++;
    if (!basec) continue;
    if ((basec==1)&&(base[0]=='.')) continue;
    if ((basec==2)&&(base[0]=='.')&&(base[1]=='.')) continue;
    
    if (pathc>=sizeof(subpath)-basec) {
      closedir(dir);
      return -1;
    }
    memcpy(subpath+pathc,base,basec+1);
    
    char type=0;
    switch (de->d_type) {
      case DT_REG: type='f'; break;
      case DT_DIR: type='d'; break;
      case DT_LNK: type='l'; break;
      case DT_CHR: type='c'; break;
      case DT_BLK: type='b'; break;
      case DT_SOCK: type='s'; break;
      default: type='?'; break;
    }
    
    int err=cb(subpath,base,type,userdata);
    if (err) {
      closedir(dir);
      return err;
    }
  }
  closedir(dir);
  return 0;
}

/* Get file type.
 */
 
char fmn_file_get_type(const char *path) {
  if (!path||!path[0]) return 0;
  struct stat st={0};
  if (stat(path,&st)<0) return 0;
  if (S_ISREG(st.st_mode)) return 'f';
  if (S_ISDIR(st.st_mode)) return 'd';
  if (S_ISLNK(st.st_mode)) return 'l'; // shouldn't happen; we used stat not lstat
  if (S_ISCHR(st.st_mode)) return 'c';
  if (S_ISBLK(st.st_mode)) return 'b';
  if (S_ISSOCK(st.st_mode)) return 's';
  return '?';
}

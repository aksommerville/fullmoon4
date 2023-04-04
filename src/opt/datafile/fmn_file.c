#include "fmn_datafile.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

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

#include "inotify_internal.h"

/* Delete.
 */
 
static void inotify_watch_cleanup(struct inotify_watch *watch) {
  if (watch->path) free(watch->path);
}
 
void inotify_del(struct inotify *ino) {
  if (!ino) return;
  if (ino->fd>=0) close(ino->fd);
  if (ino->watchv) {
    while (ino->watchc-->0) inotify_watch_cleanup(ino->watchv+ino->watchc);
    free(ino->watchv);
  }
  free(ino);
}

/* New.
 */

struct inotify *inotify_new(int (*cb)(const char *path,void *userdata),void *userdata) {
  if (!cb) return 0;
  struct inotify *inotify=calloc(1,sizeof(struct inotify));
  if (!inotify) return 0;
  if ((inotify->fd=inotify_init())<0) {
    inotify_del(inotify);
    return 0;
  }
  inotify->cb=cb;
  inotify->userdata=userdata;
  inotify->ignore_dot_basenames=1;
  return inotify;
}

/* Trivial accessors.
 */
 
int inotify_get_fd(const struct inotify *ino) {
  if (!ino) return -1;
  return ino->fd;
}

int inotify_watch(struct inotify *ino,const char *path) {
  if (!ino||!path) return -1;
  
  if (ino->watchc>=ino->watcha) {
    int na=ino->watcha+16;
    if (na>INT_MAX/sizeof(struct inotify_watch)) return -1;
    void *nv=realloc(ino->watchv,sizeof(struct inotify_watch)*na);
    if (!nv) return -1;
    ino->watchv=nv;
    ino->watcha=na;
  }
  
  int wd=inotify_add_watch(ino->fd,path,IN_ATTRIB|IN_CREATE|IN_DELETE|IN_MOVED_FROM|IN_MOVED_TO);
  if (wd<0) return -1;
  char *npath=strdup(path);
  if (!npath) {
    inotify_rm_watch(ino->fd,wd);
    return -1;
  }
  
  struct inotify_watch *watch=ino->watchv+ino->watchc++;
  watch->path=npath;
  watch->wd=wd;

  return 0;
}

/* Update.
 */
 
int inotify_update(struct inotify *ino) {
  char buf[1024];
  while (1) {
    int bufc=read(ino->fd,buf,sizeof(buf));
    if (bufc<=0) return -1;
    int bufp=0;
    while (bufp<=sizeof(buf)-sizeof(struct inotify_event)) {
      struct inotify_event *event=(struct inotify_event*)(buf+bufp);
      bufp+=sizeof(struct inotify_event);
      bufp+=event->len;
      
      if (!event->name[0]) continue;
      if (ino->ignore_dot_basenames&&(event->name[0]=='.')) continue;
      
      struct inotify_watch *watch=0;
      struct inotify_watch *q=ino->watchv;
      int i=ino->watchc;
      for (;i-->0;q++) if (q->wd==event->wd) {
        watch=q;
        break;
      }
      if (!watch) continue;
      
      char path[1024];
      int pathc=snprintf(path,sizeof(path),"%s/%.*s",watch->path,event->len,event->name);
      if ((pathc<1)||(pathc>=sizeof(path))) continue;
      int err=ino->cb(path,ino->userdata);
      if (err) return err;
    }
    if (bufc<sizeof(buf)) break;
  }
  return 0;
}

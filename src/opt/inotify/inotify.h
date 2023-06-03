/* inotify.h
 */
 
#ifndef INOTIFY_H
#define INTOFIY_H

struct inotify;

void inotify_del(struct inotify *ino);

struct inotify *inotify_new(int (*cb)(const char *path,void *userdata),void *userdata);

int inotify_get_fd(const struct inotify *ino);

/* Watch for changes to files in this directory. Not recursive.
 */
int inotify_watch(struct inotify *ino,const char *path);

/* Call when my fd polls for reading.
 */
int inotify_update(struct inotify *ino);

#endif

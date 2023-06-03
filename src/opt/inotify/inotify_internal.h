#ifndef INOTIFY_INTERNAL_H
#define INOTIFY_INTERNAL_H

#include "inotify.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>

struct inotify {
  int fd;
  int (*cb)(const char *path,void *userdata);
  void *userdata;
  struct inotify_watch {
    char *path;
    int wd;
  } *watchv;
  int watchc,watcha;
  int ignore_dot_basenames; // always true for now. would be trivial to expose
};

#endif

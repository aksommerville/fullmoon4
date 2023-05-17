#ifndef FIDDLE_INTERNAL_H
#define FIDDLE_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include "opt/http/http.h"

extern struct fiddle {
  const char *exename;
  int port;
  struct http_context *http;
  volatile int sigc;
} fiddle;

int fiddle_httpcb_default(struct http_xfer *req,struct http_xfer *rsp,void *userdata);

#endif

#include "fiddle_internal.h"

extern struct http_xfer *fiddle_held_xfer;

/* Default HTTP service handler.
 */
 
int fiddle_httpcb_default(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  const char *method=http_method_repr(http_xfer_get_method(req));
  const char *path=0;
  int pathc=http_xfer_get_path(&path,req);
  fprintf(stderr,"%s: %s %.*s\n",fiddle.exename,method,pathc,path);
  if (fiddle_held_xfer) return http_xfer_set_status(rsp,404,"Invalid method or path");
  fprintf(stderr,"TYPE A RESPONSE\n");
  if (http_xfer_ref(rsp)<0) return -1;
  fiddle_held_xfer=rsp;
  return http_xfer_hold(rsp);
}

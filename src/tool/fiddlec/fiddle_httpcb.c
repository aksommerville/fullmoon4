#include "fiddle_internal.h"

/* Default HTTP service handler.
 */
 
int fiddle_httpcb_default(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  const char *method=http_method_repr(http_xfer_get_method(req));
  const char *path=0;
  int pathc=http_xfer_get_path_verbatim(&path,req);
  fprintf(stderr,"%s: %s %.*s => 404\n",fiddle.exename,method,pathc,path);
  return http_xfer_set_status(rsp,404,"Invalid method or path");
}

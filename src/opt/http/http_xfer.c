#include "http_internal.h"

/* Delete.
 */
 
void http_xfer_del(struct http_xfer *xfer) {
  if (!xfer) return;
  if (xfer->refc-->1) return;
  
  free(xfer);
}

/* Retain.
 */
 
int http_xfer_ref(struct http_xfer *xfer) {
  if (!xfer) return -1;
  if (xfer->refc<1) return -1;
  if (xfer->refc>=INT_MAX) return -1;
  xfer->refc++;
  return 0;
}

/* New.
 */
 
struct http_xfer *http_xfer_new(struct http_context *context) {
  if (!context) return 0;
  struct http_xfer *xfer=calloc(1,sizeof(struct http_xfer));
  if (!xfer) return 0;
  
  xfer->refc=1;
  xfer->context=context;
  
  return xfer;
}

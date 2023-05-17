/* http_xfer.h
 * Private.
 */
 
#ifndef HTTP_XFER_H
#define HTTP_XFER_H

struct http_xfer {
  int refc;
  struct http_context *context;
};
 
void http_xfer_del(struct http_xfer *xfer);
int http_xfer_ref(struct http_xfer *xfer);

struct http_xfer *http_xfer_new(struct http_context *context);

#endif

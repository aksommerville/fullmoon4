#include "http_internal.h"

/* File in error or hangup state.
 */
 
static int http_update_fd_error(struct http_context *context,int fd) {
  fprintf(stderr,"%s %d\n",__func__,fd);//TODO
  return 0;
}

/* File readable.
 */
 
static int http_update_fd_read(struct http_context *context,int fd) {
  fprintf(stderr,"%s %d\n",__func__,fd);//TODO
  return 0;
}

/* File writeable.
 */
 
static int http_update_fd_write(struct http_context *context,int fd) {
  fprintf(stderr,"%s %d\n",__func__,fd);//TODO
  return 0;
}

/* Identify all files that need updated.
 */
 
static int http_context_pollfdv_rebuild(struct http_context *context) {
  context->pollfdc=0;
  int i;
  
  struct http_server **server=context->serverv;
  for (i=context->serverc;i-->0;server++) {
    if ((*server)->fd<0) continue;
    struct pollfd *pollfd=http_context_pollfdv_require(context);
    if (!pollfd) return -1;
    pollfd->fd=(*server)->fd;
    pollfd->events=POLLIN|POLLERR|POLLHUP;
  }
  
  //TODO streams

  return 0;
}

/* Update, main entry point.
 */
 
int http_update(struct http_context *context,int toms) {

  //TODO Enforce timeouts.
  //TODO Drop defunct objects.

  if (http_context_pollfdv_rebuild(context)<0) return -1;
  if (!context->pollfdc) {
    if (toms<0) {
      while (1) usleep(1000000);
      return -1;
    } else if (!toms) {
      return 0;
    } else {
      usleep(toms/1000);
      return 0;
    }
  }
  
  int err=poll(context->pollfdv,context->pollfdc,toms);
  if (err<=0) return err;
  
  const struct pollfd *pollfd=context->pollfdv;
  int i=context->pollfdc;
  for (;i-->0;pollfd++) {
    if (pollfd->revents&(POLLERR|POLLHUP)) {
      if (http_update_fd_error(context,pollfd->fd)<0) return -1;
    } else if (pollfd->revents&POLLIN) {
      if (http_update_fd_read(context,pollfd->fd)<0) return -1;
    } else if (pollfd->revents&POLLOUT) {
      if (http_update_fd_write(context,pollfd->fd)<0) return -1;
    }
  }
  
  return 0;
}

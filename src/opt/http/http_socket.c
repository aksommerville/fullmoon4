#include "http_internal.h"
#include <fcntl.h>
#include <unistd.h>

/* Delete.
 */
 
void http_socket_del(struct http_socket *socket) {
  if (!socket) return;
  if (socket->refc-->1) return;
  
  if (socket->fd>=0) close(socket->fd);
  
  free(socket);
}

/* Retain.
 */
 
int http_socket_ref(struct http_socket *socket) {
  if (!socket) return -1;
  if (socket->refc<1) return -1;
  if (socket->refc>=INT_MAX) return -1;
  socket->refc++;
  return 0;
}

/* New.
 */
 
struct http_socket *http_socket_new(struct http_context *context) {
  if (!context) return 0;
  struct http_socket *socket=calloc(1,sizeof(struct http_socket));
  if (!socket) return 0;
  
  socket->refc=1;
  socket->context=context;
  socket->fd=-1;
  
  return socket;
}

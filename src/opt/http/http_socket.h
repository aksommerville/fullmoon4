/* http_socket.h
 * Private.
 */
 
#ifndef HTTP_SOCKET_H
#define HTTP_SOCKET_H

struct http_socket {
  int refc;
  struct http_context *context;
  int fd;
};
 
void http_socket_del(struct http_socket *socket);
int http_socket_ref(struct http_socket *socket);

struct http_socket *http_socket_new(struct http_context *context);

#endif

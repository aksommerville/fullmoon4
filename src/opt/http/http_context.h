/* http_context.h
 * Private.
 */
 
#ifndef HTTP_CONTEXT_H
#define HTTP_CONTEXT_H

struct http_context {
  int refc;
  struct pollfd *pollfdv;
  int pollfdc,pollfda;
  struct http_listener **listenerv;
  int listenerc,listenera;
  struct http_server **serverv;
  int serverc,servera;
};

struct pollfd *http_context_pollfdv_require(struct http_context *context);
struct http_listener *http_context_add_new_listener(struct http_context *context);
void http_context_remove_listener(struct http_context *context,struct http_listener *listener);
struct http_server *http_context_add_new_server(struct http_context *context);
void http_context_remove_server(struct http_context *context,struct http_server *server);

#endif

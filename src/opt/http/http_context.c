#include "http_internal.h"

/* Delete.
 */
 
void http_context_del(struct http_context *context) {
  if (!context) return;
  if (context->refc-->1) return;
  
  if (context->pollfdv) free(context->pollfdv);
  
  if (context->listenerv) {
    while (context->listenerc-->0) http_listener_del(context->listenerv[context->listenerc]);
    free(context->listenerv);
  }
  
  if (context->serverv) {
    while (context->serverc-->0) http_server_del(context->serverv[context->serverc]);
    free(context->serverv);
  }
  
  free(context);
}

/* Retain.
 */
 
int http_context_ref(struct http_context *context) {
  if (!context) return -1;
  if (context->refc<1) return -1;
  if (context->refc==INT_MAX) return -1;
  context->refc++;
  return 0;
}

/* New.
 */
 
struct http_context *http_context_new() {
  struct http_context *context=calloc(1,sizeof(struct http_context));
  if (!context) return 0;
  
  context->refc=1;
  
  return context;
}

/* Grow pollfdv.
 */
 
struct pollfd *http_context_pollfdv_require(struct http_context *context) {
  if (context->pollfdc>=context->pollfda) {
    int na=context->pollfda+16;
    if (na>INT_MAX/sizeof(struct pollfd)) return 0;
    void *nv=realloc(context->pollfdv,sizeof(struct pollfd)*na);
    if (!nv) return 0;
    context->pollfdv=nv;
    context->pollfda=na;
  }
  struct pollfd *pollfd=context->pollfdv+context->pollfdc++;
  memset(pollfd,0,sizeof(struct pollfd));
  return pollfd;
}

/* Attach a new unconfigured listener.
 */
 
struct http_listener *http_context_add_new_listener(struct http_context *context) {
  if (!context) return 0;
  if (context->listenerc>=context->listenera) {
    int na=context->listenera+16;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(context->listenerv,sizeof(void*)*na);
    if (!nv) return 0;
    context->listenerv=nv;
    context->listenera=na;
  }
  struct http_listener *listener=http_listener_new(context);
  if (!listener) return 0;
  context->listenerv[context->listenerc++]=listener;
  return listener;
}

/* Find and remove listener.
 */
 
void http_context_remove_listener(struct http_context *context,struct http_listener *listener) {
  if (!context||!listener) return;
  int i=context->listenerc;
  while (i-->0) {
    if (context->listenerv[i]!=listener) continue;
    context->listenerc--;
    memmove(context->listenerv+i,context->listenerv+i+1,sizeof(void*)*(context->listenerc-i));
    http_listener_del(listener);
    return;
  }
}

/* Attach a new unconfigured server.
 */
 
struct http_server *http_context_add_new_server(struct http_context *context) {
  if (!context) return 0;
  if (context->serverc>=context->servera) {
    int na=context->servera+4;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(context->serverv,sizeof(void*)*na);
    if (!nv) return 0;
    context->serverv=nv;
    context->servera=na;
  }
  struct http_server *server=http_server_new(context);
  if (!server) return 0;
  context->serverv[context->serverc++]=server;
  return server;
}

/* Find and remove server.
 */
 
void http_context_remove_server(struct http_context *context,struct http_server *server) {
  if (!context||!server) return;
  int i=context->serverc;
  while (i-->0) {
    if (context->serverv[i]!=server) continue;
    context->serverc--;
    memmove(context->serverv+i,context->serverv+i+1,sizeof(void*)*(context->serverc-i));
    http_server_del(server);
    return;
  }
}

/* Create server, public convenience.
 */
 
struct http_server *http_serve(struct http_context *context,int port) {
  if (port<1) return 0;
  struct http_server *server=http_context_add_new_server(context);
  if (!server) return 0;
  
  if (
    (http_server_init_tcp_stream(server)<0)||
    (http_server_bind(server,"localhost",port)<0)||
    (http_server_listen(server,10)<0)
  ) {
    http_context_remove_server(context,server);
    return 0;
  }

  return server;
}

/* Create listener, public convenience.
 */

struct http_listener *http_listen(
  struct http_context *context,
  int method,
  const char *path,
  int (*cb)(struct http_xfer *req,struct http_xfer *rsp,void *userdata),
  void *userdata
) {
  if ((method<0)||!cb) return 0;
  struct http_listener *listener=http_context_add_new_listener(context);
  if (!listener) return 0;
  
  listener->method=method;
  listener->cb_serve=cb;
  listener->userdata=userdata;
  
  if (http_listener_set_path(listener,path,-1)<0) {
    http_context_remove_listener(context,listener);
    return 0;
  }
  
  return listener;
}

/* Create websocket listener, public convenience.
 */

struct http_listener *http_listen_websocket(
  struct http_context *context,
  const char *path,
  int (*cb_connect)(struct http_socket *socket,void *userdata),
  int (*cb_disconnect)(struct http_socket *socket,void *userdata),
  int (*cb_message)(struct http_socket *socket,int type,const void *v,int c,void *userdata),
  void *userdata
) {
  struct http_listener *listener=http_context_add_new_listener(context);
  if (!listener) return 0;
  
  listener->method=HTTP_METHOD_WEBSOCKET;
  listener->cb_connect=cb_connect;
  listener->cb_disconnect=cb_disconnect;
  listener->cb_message=cb_message;
  listener->userdata=userdata;
  
  if (http_listener_set_path(listener,path,-1)<0) {
    http_context_remove_listener(context,listener);
    return 0;
  }
  
  return listener;
}

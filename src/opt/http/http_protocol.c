/* http_protocol.c
 * This is where we do the real "HTTP" stuff.
 */

#include "http_internal.h"

/* Encode a request or response onto the socket's write buffer.
 */
 
int http_socket_encode_xfer(struct http_socket *socket,struct http_xfer *xfer) {
  if (!xfer->linec&&(http_xfer_set_line(xfer,"HTTP/1.1 200 OK",-1)<0)) return -1;
  if (http_socket_wbuf_append(socket,xfer->line,xfer->linec)<0) return -1;
  if (http_socket_wbuf_append(socket,"\r\n",2)<0) return -1;
  const struct http_dict_entry *entry=xfer->headers.v;
  int i=xfer->headers.c;
  for (;i-->0;entry++) {
    if (http_socket_wbuf_appendf(socket,"%.*s: %.*s\r\n",entry->kc,entry->k,entry->vc,entry->v)<0) return -1;
  }
  if (xfer->bodyc||(xfer==socket->rsp)) {
    if (http_socket_wbuf_appendf(socket,"Content-Length: %d\r\n",xfer->bodyc)<0) return -1;
  }
  if (http_socket_wbuf_append(socket,"\r\n",2)<0) return -1;
  if (http_socket_wbuf_append(socket,xfer->body,xfer->bodyc)<0) return -1;
  xfer->state=HTTP_XFER_STATE_SEND;
  return 0;
}

/* End of body.
 */
 
static int http_xfer_received(struct http_socket *socket,struct http_xfer *xfer) {
  xfer->state=HTTP_XFER_STATE_READY;
  
  //TODO if it is a response, deliver it to app
  
  struct http_listener *listener=http_context_find_listener_for_request(xfer->context,xfer);
  
  // Listener not found. It's a bit overkill to create the response object, but it feels cleaner this way.
  if (!listener) {
    if (socket->rsp) http_xfer_del(socket->rsp);
    if (!(socket->rsp=http_xfer_new(socket->context))) return -1;
    if (http_xfer_set_line(socket->rsp,"HTTP/1.1 404 Not found",-1)<0) return -1;
    return http_socket_encode_xfer(socket,socket->rsp);
  }
  
  // Regular HTTP service.
  if (listener->cb_serve) {
    if (socket->rsp) http_xfer_del(socket->rsp);
    if (!(socket->rsp=http_xfer_new(socket->context))) return -1;
    if (listener->cb_serve(xfer,socket->rsp,listener->userdata)<0) {
      http_dict_clear(&socket->rsp->headers);
      socket->rsp->bodyc=0;
      if (http_xfer_set_line(socket->rsp,"HTTP/1.1 500 Internal error",-1)<0) return -1;
      return http_socket_encode_xfer(socket,socket->rsp);
    }
    if (socket->rsp->state==HTTP_XFER_STATE_DEFERRED) {
      return 0;
    }
    return http_socket_encode_xfer(socket,socket->rsp);
  }
  
  // Upgrade to WebSocket.
  if (listener->cb_connect||listener->cb_message) {
    fprintf(stderr,"*** TODO WebSocket ***\n");//TODO
    return -1;
  }
  
  // Misconfigured listener.
  if (socket->rsp) http_xfer_del(socket->rsp);
  if (!(socket->rsp=http_xfer_new(socket->context))) return -1;
  if (http_xfer_set_line(socket->rsp,"HTTP/1.1 500 Internal error",-1)<0) return -1;
  return http_socket_encode_xfer(socket,socket->rsp);
}

/* Body.
 */
 
static int http_xfer_receive_body(struct http_socket *socket,struct http_xfer *xfer,const char *src,int srcc) {

  // If chunked and we don't have a chunk length, read it.
  if (xfer->chunked&&!xfer->body_pendingc) {
    if ((srcc=http_measure_line(src,srcc))<=0) return 0;
    int i=0; for (;i<srcc;i++) {
      char digit=src[i];
           if ((digit>='0')&&(digit<='9')) digit-='0';
      else if ((digit>='a')&&(digit<='f')) digit=digit-'a'+10;
      else if ((digit>='A')&&(digit<='F')) digit=digit-'A'+10;
      else if ((unsigned char)digit<=0x20) continue;
      else return -1;
      if (xfer->body_pendingc&0x78000000) return -1;
      xfer->body_pendingc<<=4;
      xfer->body_pendingc|=digit;
    }
    if (!xfer->body_pendingc) {
      if (http_xfer_received(socket,xfer)<0) return -1;
    }
    return srcc;
  }
  
  // Within chunk, or full body, append it.
  int cpc=xfer->body_pendingc;
  if (cpc>srcc) cpc=srcc;
  if (http_xfer_append_body(xfer,src,cpc)<0) return -1;
  if (xfer->body_pendingc-=cpc) return cpc;
  if (!xfer->chunked) {
    if (http_xfer_received(socket,xfer)<0) return -1;
    return cpc;
  }
  return cpc;
}

/* End of headers. Begin body or finalize.
 */
 
static int http_xfer_begin_body(struct http_socket *socket,struct http_xfer *xfer) {
  
  /* If it's HEAD, there will be no body. Otherwise allow that there may be.
   */
  switch (http_xfer_get_method(xfer)) {
    case HTTP_METHOD_HEAD: return http_xfer_received(socket,xfer);
  }
  
  /* If there is a Content-Length, prepare to read exactly that much.
   */
  int cl=0;
  if (http_xfer_get_header_int(&cl,xfer,"Content-Length",14)>=0) {
    if (cl<=0) return http_xfer_received(socket,xfer);
    xfer->body_pendingc=cl;
    xfer->chunked=0;
    xfer->state=HTTP_XFER_STATE_RCV_BODY;
    return 0;
  }
  
  /* Check for chunked bodies.
   */
  const char *encoding=0;
  int encodingc=http_xfer_get_header(&encoding,xfer,"Transfer-Encoding",16);
  if ((encodingc==7)&&!memcmp(encoding,"chunked",7)) {
    xfer->body_pendingc=0;
    xfer->chunked=1;
    xfer->state=HTTP_XFER_STATE_RCV_BODY;
    return 0;
  }
  
  // OK, assume there is no body.
  return http_xfer_received(socket,xfer);
}

/* Header.
 */
 
static int http_xfer_receive_header(struct http_socket *socket,struct http_xfer *xfer,const char *src,int srcc) {
  if ((srcc=http_measure_line(src,srcc))<2) return 0;
  
  const char *k=src;
  int kc=0;
  while ((kc<srcc)&&(k[kc]!=':')) kc++;
  if (kc>=srcc) { // empty line, end of headers
    if (http_xfer_begin_body(socket,xfer)<0) return -1;
    return srcc;
  }
  const char *v=k+kc+1;
  int vc=srcc-kc-1;
  while (kc&&((unsigned char)k[kc-1]<=0x20)) kc--;
  while (kc&&((unsigned char)k[0]<=0x20)) { k++; kc--; }
  while (vc&&((unsigned char)v[vc-1]<=0x20)) vc--;
  while (vc&&((unsigned char)v[0]<=0x20)) { v++; vc--; }
  
  if (http_dict_set(&xfer->headers,k,kc,v,vc)<0) return -1;
  
  return srcc;
}

/* Request-Line or Status-Line, incoming.
 * We also consume leading space, just to be safe.
 */
 
static int http_xfer_receive_line(struct http_socket *socket,struct http_xfer *xfer,const char *src,int srcc) {
  int spcc=http_measure_space(src,srcc);
  if (spcc>0) return spcc;
  if ((srcc=http_measure_line(src,srcc))<2) return 0;
  if (http_xfer_set_line(xfer,src,srcc)<0) return -1;
  xfer->state=HTTP_XFER_STATE_RCV_HEADER;
  return srcc;
}

/* Receive input into an xfer.
 * Either a request and we are a server, or a response and we are a client.
 */
 
static int http_protocol_http_input(struct http_socket *socket,struct http_xfer *xfer,const char *src,int srcc) {
  switch (xfer->state) {
    case HTTP_XFER_STATE_RCV_LINE: return http_xfer_receive_line(socket,xfer,src,srcc);
    case HTTP_XFER_STATE_RCV_HEADER: return http_xfer_receive_header(socket,xfer,src,srcc);
    case HTTP_XFER_STATE_RCV_BODY: return http_xfer_receive_body(socket,xfer,src,srcc);
  }
  return 0;
}

/* Digest input, return length consumed or zero if not ready.
 */
 
static int http_socket_digest_input_1(struct http_socket *socket,const char *src,int srcc) {

  /* If protocol is UNSET, drop everything, make a request container, and enter HTTP_SERVER protocol.
   */
  if (socket->protocol==HTTP_PROTOCOL_UNSET) {
    http_xfer_del(socket->req); socket->req=0;
    http_xfer_del(socket->rsp); socket->rsp=0;
    http_listener_del(socket->listener); socket->listener=0;
    socket->protocol=HTTP_PROTOCOL_HTTP_SERVER;
  }
  
  /* HTTP_SERVER: Create the request if we don't have it yet, and dispatch on its state.
   */
  if (socket->protocol==HTTP_PROTOCOL_HTTP_SERVER) {
    if (!socket->req) {
      if (!(socket->req=http_xfer_new(socket->context))) return -1;
      socket->req->state=HTTP_XFER_STATE_RCV_LINE;
    }
    return http_protocol_http_input(socket,socket->req,src,srcc);
  }
  
  /* HTTP_CLIENT: Create the response if we don't have it yet, and dispatch on its state.
   */
  if (socket->protocol==HTTP_PROTOCOL_HTTP_CLIENT) {
    if (!socket->rsp) {
      if (!(socket->rsp=http_xfer_new(socket->context))) return -1;
      socket->rsp->state=HTTP_XFER_STATE_RCV_LINE;
    }
    return http_protocol_http_input(socket,socket->rsp,src,srcc);
  }
  
  // TODO HTTP_PROTOCOL_WEBSOCKET

  return 0;
}

/* Digest input.
 */

int http_socket_digest_input(struct http_socket *socket) {
  if (!socket) return -1;
  while (socket->rbufc>0) {
    int err=http_socket_digest_input_1(socket,socket->rbuf+socket->rbufp,socket->rbufc);
    if (err<=0) return err;
    if (socket->rbufc-=err) socket->rbufp+=err;
    else socket->rbufp=0;
  }
  return 0;
}

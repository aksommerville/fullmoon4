#include "fiddle_internal.h"

/* Connect.
 */
 
int fiddle_wscb_connect(struct http_socket *socket,void *userdata) {
  if (fiddle.websocketc>=fiddle.websocketa) {
    int na=fiddle.websocketa+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(fiddle.websocketv,sizeof(void*));
    if (!nv) return -1;
    fiddle.websocketv=nv;
    fiddle.websocketa=na;
  }
  if (http_socket_ref(socket)<0) return -1;
  fiddle.websocketv[fiddle.websocketc++]=socket;
  return 0;
}

/* Disconnect.
 */
 
int fiddle_wscb_disconnect(struct http_socket *socket,void *userdata) {
  int i=fiddle.websocketc;
  while (i-->0) {
    if (fiddle.websocketv[i]==socket) {
      fiddle.websocketc--;
      memmove(fiddle.websocketv+i,fiddle.websocketv+i+1,sizeof(void*)*(fiddle.websocketc-i));
      http_socket_del(socket);
      // keep going, in case it got added twice by accident.
    }
  }
  return 0;
}

/* Incoming message.
 */
 
int fiddle_wscb_message(struct http_socket *socket,int type,const void *v,int c,void *userdata) {
  switch (type) {
    case 1: break; // text
    case 2: { // binary
        const uint8_t *V=v;
        if (c==4) {
          fiddle_synth_event(V[0],V[1],V[2],V[3]);
        }
      } break;
    case 8: break; // farewell
  }
  return 0;
}

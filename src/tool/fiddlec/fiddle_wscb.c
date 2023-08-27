#include "fiddle_internal.h"
#include "opt/midi/midi.h"

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
  
  if (socket->req) {
    const char *path=0;
    int pathc=http_xfer_get_path(&path,socket->req);
    if ((pathc==10)&&!memcmp(path,"/websocket",10)) http_socket_set_userdata(socket,(void*)FIDDLE_SOCKET_MODE_FIDDLE);
    else if ((pathc==5)&&!memcmp(path,"/midi",5)) http_socket_set_userdata(socket,(void*)FIDDLE_SOCKET_MODE_MIDI_IN);
    else fprintf(stderr,"%s: Unexpected path '%.*s'\n",__func__,pathc,path);
  } else fprintf(stderr,"%s: Path not available\n",__func__);
  
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
  const void *mode=http_socket_get_userdata(socket);
  
  if (mode==FIDDLE_SOCKET_MODE_FIDDLE) {
    switch (type) {
      case 1: break; // text
      case 2: { // binary: Digested MIDI-ish events. (chid,opcode,a,b)
          const uint8_t *V=v;
          if (c==4) {
            fiddle_synth_event(V[0],V[1],V[2],V[3]);
          }
        } break;
      case 8: break; // farewell
    }
    
  } else if (mode==FIDDLE_SOCKET_MODE_MIDI_IN) {
    // Plain MIDI socket can pack events together, but each packet must end on an event boundary.
    struct midi_stream_reader reader={0};
    midi_stream_reader_more(&reader,v,c);
    struct midi_event event;
    while (midi_stream_reader_next(&event,&reader)>0) {
      fiddle_synth_event(event.chid,event.opcode,event.a,event.b);
    }
  
  }
  return 0;
}

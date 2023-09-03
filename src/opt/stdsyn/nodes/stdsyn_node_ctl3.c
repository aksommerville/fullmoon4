/* stdsyn_node_ctl3.c
 * Channel controller for instruments with shared prefix and suffix, and a private mixer.
 */

#include "../fmn_stdsyn_internal.h"
#include "../bits/stdsyn_pipe.h"

struct stdsyn_node_ctl3 {
  struct stdsyn_node hdr;
  struct stdsyn_pipe prefix;
  struct stdsyn_pipe_config voice;
  struct stdsyn_pipe suffix;
  struct stdsyn_ctl3_voice {
    struct stdsyn_pipe pipe;
    uint8_t noteid;
  } *voicev;
  int voicec,voicea;
  float master;
  float trim;
  float pan;
  int draining;
  float prebuf[STDSYN_BUFFER_SIZE];
  float mixbuf[STDSYN_BUFFER_SIZE];
};

#define NODE ((struct stdsyn_node_ctl3*)node)

/* Cleanup.
 */
 
static void stdsyn_ctl3_voice_cleanup(struct stdsyn_ctl3_voice *voice) {
  stdsyn_pipe_cleanup(&voice->pipe);
}
 
static void _ctl3_del(struct stdsyn_node *node) {
  stdsyn_pipe_cleanup(&NODE->prefix);
  stdsyn_pipe_config_cleanup(&NODE->voice);
  stdsyn_pipe_cleanup(&NODE->suffix);
  if (NODE->voicev) {
    while (NODE->voicec-->0) stdsyn_ctl3_voice_cleanup(NODE->voicev+NODE->voicec);
    free(NODE->voicev);
  }
}

/* Note On.
 */
 
static void _ctl3_note_on(struct stdsyn_node *node,uint8_t noteid,uint8_t velocity) {
  //fprintf(stderr,"%s %02x %02x\n",__func__,noteid,velocity);
  if (NODE->voicec>=NODE->voicea) {
    int na=NODE->voicea+8;
    if (na>INT_MAX/sizeof(struct stdsyn_ctl3_voice)) return;
    void *nv=realloc(NODE->voicev,sizeof(struct stdsyn_ctl3_voice)*na);
    if (!nv) return;
    NODE->voicev=nv;
    NODE->voicea=na;
  }
  struct stdsyn_ctl3_voice *voice=NODE->voicev+NODE->voicec++;
  memset(voice,0,sizeof(struct stdsyn_ctl3_voice));
  voice->noteid=noteid;
  //TODO trim and master
  if (stdsyn_pipe_init(&voice->pipe,&NODE->voice,node->driver,noteid,velocity)<0) {
    stdsyn_ctl3_voice_cleanup(voice);
    NODE->voicec--;
    return;
  }
}

/* Note Off.
 */
 
static void _ctl3_note_off(struct stdsyn_node *node,uint8_t noteid,uint8_t velocity) {
  int i=NODE->voicec;
  struct stdsyn_ctl3_voice *voice=NODE->voicev+i-1;
  for (;i-->0;voice--) {
    if (voice->noteid!=noteid) continue;
    stdsyn_pipe_release(&voice->pipe,velocity);
    voice->noteid=0xff;
  }
}

/* Event.
 */
 
static int _ctl3_event(struct stdsyn_node *node,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  //fprintf(stderr,"%s %p %02x %02x %02x %02x\n",__func__,node,chid,opcode,a,b);
  switch (opcode) {
    case MIDI_OPCODE_NOTE_OFF: _ctl3_note_off(node,a,b); return 1;
    case MIDI_OPCODE_NOTE_ON: _ctl3_note_on(node,a,b); return 1;
    case MIDI_OPCODE_CONTROL: switch (a) {
        case MIDI_CTL_VOLUME_MSB: NODE->trim=b/127.0f; return 1;
        case MIDI_CTL_PAN_MSB: NODE->pan=(b-0x40)/64.0f; return 1;
      } break;
    case MIDI_OPCODE_WHEEL: return 1; //TODO?
  }
  return 0;
}

/* Update.
 */
 
static void ctl3_update_pipes(struct stdsyn_node *node,int c) {
  float *buf0=stdsyn_node_get_buffer(node,0);
  memset(buf0,0,sizeof(float)*c);
  stdsyn_pipe_update(&NODE->prefix,c);
  //int first=1;
  int i=NODE->voicec;
  struct stdsyn_ctl3_voice *voice=NODE->voicev+i-1;
  for (;i-->0;voice--) {
    if (voice->pipe.defunct) continue;
    //if (first) { memcpy(NODE->prebuf,buf0,sizeof(float)*c); first=0; }
    //else memcpy(buf0,NODE->prebuf,sizeof(float)*c);
    stdsyn_pipe_update(&voice->pipe,c);
  }
  stdsyn_pipe_update(&NODE->suffix,c);
  
  // Apply trim.
  float *v=buf0;
  for (i=c;i-->0;v++) (*v)*=NODE->trim;
  //TODO pan?
}
 
static void _ctl3_update_overwrite(float *v,int c,struct stdsyn_node *node) {
  ctl3_update_pipes(node,c);
  memcpy(v,stdsyn_node_get_buffer(node,0),sizeof(float)*c);
}

static void _ctl3_update_add(float *v,int c,struct stdsyn_node *node) {
  ctl3_update_pipes(node,c);
  const float *src=stdsyn_node_get_buffer(node,0);
  for (;c-->0;v++,src++) (*v)+=*src;
}

/* Post update.
 */
 
static void _ctl3_lfupdate(struct stdsyn_node *node) {
  int running=0;
  int i=NODE->voicec;
  struct stdsyn_ctl3_voice *voice=NODE->voicev+i-1;
  for (;i-->0;voice--) {
    if (!voice->pipe.defunct) {
      running=1;
      continue;
    }
    stdsyn_ctl3_voice_cleanup(voice);
    NODE->voicec--;
    memmove(voice,voice+1,sizeof(struct stdsyn_ctl3_voice)*(NODE->voicec-i));
  }
  if (NODE->draining&&!running) node->defunct=1;
}

/* Release all.
 */
 
static void _ctl3_release(struct stdsyn_node *node,uint8_t velocity) {
  int i=NODE->voicec;
  struct stdsyn_ctl3_voice *voice=NODE->voicev+i-1;
  for (;i-->0;voice--) {
    stdsyn_pipe_release(&voice->pipe,velocity);
  }
  NODE->draining=1;
}

/* Init.
 */
 
static int _ctl3_init(struct stdsyn_node *node,uint8_t velocity,const void *argv,int argc) {
  if (node->chanc!=1) return -1; //TODO support stereo
  if (node->overwrite) node->update=_ctl3_update_overwrite;
  else node->update=_ctl3_update_add;
  node->lfupdate=_ctl3_lfupdate;
  node->event=_ctl3_event;
  node->release=_ctl3_release;
  NODE->master=1.0f;
  NODE->trim=1.0f;
  NODE->pan=0.0f;
  return 0;
}

/* Apply instrument.
 */
 
static int _ctl3_apply_instrument(struct stdsyn_node *node,const struct stdsyn_instrument *ins) {
  NODE->master=ins->master;
  if (ins->prefix) {
    if (stdsyn_pipe_init(&NODE->prefix,ins->prefix,node->driver,0x40,0x40)<0) return -1;
  }
  if (ins->voice) {
    if (stdsyn_pipe_config_copy(&NODE->voice,ins->voice)<0) return -1;
  }
  if (ins->suffix) {
    if (stdsyn_pipe_init(&NODE->suffix,ins->suffix,node->driver,0x40,0x40)<0) return -1;
  }
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_ctl3={
  .name="ctl3",
  .objlen=sizeof(struct stdsyn_node_ctl3),
  .del=_ctl3_del,
  .init=_ctl3_init,
  .apply_instrument=_ctl3_apply_instrument,
};

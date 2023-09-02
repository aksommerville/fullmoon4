/* stdsyn_node_ctlm.c
 * Channel controller for minsyn-format instruments.
 * These are brutally simple instruments with just an envelope-driven 2-wave mixer and level envelope.
 * Voices are a separate type.
 */

#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_ctlm {
  struct stdsyn_node hdr;
  struct stdsyn_wave *wave;
  struct stdsyn_wave *mixwave;
  struct stdsyn_env env;
  struct stdsyn_env mixenv;
};

#define NODE ((struct stdsyn_node_ctlm*)node)

/* Cleanup.
 */
 
static void _ctlm_del(struct stdsyn_node *node) {
  stdsyn_wave_del(NODE->wave);
  stdsyn_wave_del(NODE->mixwave);
}

/* Note On.
 */
 
static void _ctlm_note_on(struct stdsyn_node *node,uint8_t noteid,uint8_t velocity) {
  struct stdsyn_node *voice=stdsyn_node_new(node->driver,&stdsyn_node_type_minsyn,1,1,noteid,velocity);
  fprintf(stderr,"%s %02x %02x, voice=%p\n",__func__,noteid,velocity,voice);
  if (stdsyn_node_minsyn_setup(voice,NODE->wave,NODE->mixwave,&NODE->env,&NODE->mixenv)<0) {
    fprintf(stderr,"Failed to instantiate minsyn voice.\n");
    return;
  }
  voice->chid=node->chid;
  if (stdsyn_node_mixer_add_voice(NDRIVER->main,voice)<0) {
    fprintf(stderr,"Failed to attach minsyn voice to main.\n");
    stdsyn_node_del(voice);
    return;
  }
  stdsyn_node_del(voice);
}

/* Event.
 */
 
static int _ctlm_event(struct stdsyn_node *node,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  //fprintf(stderr,"%s %p %02x %02x %02x %02x\n",__func__,node,chid,opcode,a,b);
  switch (opcode) {
    case MIDI_OPCODE_NOTE_OFF: return 0; // Let the mixer handle these.
    case MIDI_OPCODE_NOTE_ON: _ctlm_note_on(node,a,b); return 1;
    case MIDI_OPCODE_CONTROL: switch (a) {
        //TODO level
        //TODO pan?
      } break;
    case MIDI_OPCODE_WHEEL: return 1; //TODO?
  }
  return 0;
}

/* Init.
 */
 
static int _ctlm_init(struct stdsyn_node *node,uint8_t velocity) {
  node->event=_ctlm_event;
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_ctlm={
  .name="ctlm",
  .objlen=sizeof(struct stdsyn_node_ctlm),
  .del=_ctlm_del,
  .init=_ctlm_init,
};

/* Decode.
 */
 
int stdsyn_node_ctlm_decode(struct stdsyn_node *node,const void *src,int srcc) {
  if (!node||(node->type!=&stdsyn_node_type_ctlm)) return -1;
  if (!src||(srcc<1)) return -1;
  const uint8_t *SRC=src;
  uint8_t features=SRC[0];
  if (features&0xc0) return -1;
  int srcp=1;
  
  if (features&0x01) { // main harmonics
    if (srcp>srcc-1) return -1;
    uint8_t coefc=SRC[srcp++];
    if (srcp>srcc-coefc) return -1;
    if (!(NODE->wave=stdsyn_wave_from_harmonics(SRC+srcp,coefc))) return -1;
    srcp+=coefc;
  } else {
    if (!(NODE->wave=stdsyn_wave_from_harmonics("\xff",1))) return -1;
  }
  
  switch (features&0x06) { // env
    case 0: stdsyn_env_default(&NODE->env); break;
    case 0x02: {
        if (srcp>srcc-5) return -1;
        stdsyn_env_decode_lo(&NODE->env,SRC+srcp);
        stdsyn_env_decode_hi(&NODE->env,0);
        srcp+=5;
      } break;
    case 0x04: {
        if (srcp>srcc-5) return -1;
        stdsyn_env_decode_hi(&NODE->env,SRC+srcp);
        stdsyn_env_decode_lo(&NODE->env,0);
        srcp+=5;
      } break;
    case 0x06: {
        if (srcp>srcc-10) return -1;
        stdsyn_env_decode_lo(&NODE->env,SRC+srcp);
        srcp+=5;
        stdsyn_env_decode_hi(&NODE->env,SRC+srcp);
        srcp+=5;
      } break;
  }

  if (features&0x08) { // mix harmonics
    if (srcp>srcc-1) return -1;
    uint8_t coefc=SRC[srcp++];
    if (srcp>srcc-coefc) return -1;
    if (!(NODE->mixwave=stdsyn_wave_from_harmonics(SRC+srcp,coefc))) return -1;
    srcp+=coefc;
  } else {
    // null is ok, for no mixing
  }
  
  switch (features&0x30) { // mix env
    case 0: stdsyn_env_default(&NODE->env); break;
    case 0x10: {
        if (srcp>srcc-5) return -1;
        stdsyn_env_decode_lo(&NODE->mixenv,SRC+srcp);
        stdsyn_env_decode_hi(&NODE->mixenv,0);
        srcp+=5;
      } break;
    case 0x20: {
        if (srcp>srcc-5) return -1;
        stdsyn_env_decode_hi(&NODE->mixenv,SRC+srcp);
        stdsyn_env_decode_lo(&NODE->mixenv,0);
        srcp+=5;
      } break;
    case 0x30: {
        if (srcp>srcc-10) return -1;
        stdsyn_env_decode_lo(&NODE->mixenv,SRC+srcp);
        srcp+=5;
        stdsyn_env_decode_hi(&NODE->mixenv,SRC+srcp);
        srcp+=5;
      } break;
  }

  return 0;
}

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
  float trim;
  float pan;
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
  //fprintf(stderr,"%s %02x %02x, voice=%p\n",__func__,noteid,velocity,voice);
  if (stdsyn_node_minsyn_setup(voice,NODE->wave,NODE->mixwave,&NODE->env,&NODE->mixenv,NODE->trim,NODE->pan)<0) {
    fprintf(stderr,"Failed to instantiate minsyn voice.\n");
    stdsyn_node_del(voice);
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
        case MIDI_CTL_VOLUME_MSB: NODE->trim=b/127.0f; return 1;
        case MIDI_CTL_PAN_MSB: NODE->pan=(b-0x40)/64.0f; return 1;
      } break;
    case MIDI_OPCODE_WHEEL: return 1; //TODO?
  }
  return 0;
}

/* Init.
 */
 
static int _ctlm_init(struct stdsyn_node *node,uint8_t velocity) {
  node->event=_ctlm_event;
  NODE->trim=1.0f;
  NODE->pan=0.0f;
  return 0;
}

/* Apply instrument.
 */
 
static int _ctlm_apply_instrument(struct stdsyn_node *node,const struct stdsyn_instrument *ins) {
  if (NODE->wave) { stdsyn_wave_del(NODE->wave); NODE->wave=0; }
  if (NODE->mixwave) { stdsyn_wave_del(NODE->mixwave); NODE->mixwave=0; }
  if (ins->wave&&(stdsyn_wave_ref(ins->wave)<0)) return -1;
  NODE->wave=ins->wave;
  if (ins->mixwave&&(stdsyn_wave_ref(ins->mixwave)<0)) return -1;
  NODE->mixwave=ins->mixwave;
  NODE->env=ins->env;
  NODE->mixenv=ins->mixenv;
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_ctlm={
  .name="ctlm",
  .objlen=sizeof(struct stdsyn_node_ctlm),
  .del=_ctlm_del,
  .init=_ctlm_init,
  .apply_instrument=_ctlm_apply_instrument,
};

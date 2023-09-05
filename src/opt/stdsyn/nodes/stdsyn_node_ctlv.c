/* stdsyn_node_ctlv.c
 * Channel controller for instruments with independent voices, just oscillator+envelope for each.
 */

#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_ctlv {
  struct stdsyn_node hdr;
  struct stdsyn_wave *carrier;
  int fmabs;
  float fmrate;
  struct stdsyn_env fmenv;
  struct stdsyn_env env;
  float master;
  float trim;
  float pan;
};

#define NODE ((struct stdsyn_node_ctlv*)node)

/* Cleanup.
 */
 
static void _ctlv_del(struct stdsyn_node *node) {
  stdsyn_wave_del(NODE->carrier);
}

/* Note On.
 */
 
static void _ctlv_note_on(struct stdsyn_node *node,uint8_t noteid,uint8_t velocity) {
  //fprintf(stderr,"%s %02x %02x ; fmabs=%d fmrate=%f\n",__func__,noteid,velocity,NODE->fmabs,NODE->fmrate);
  struct stdsyn_node *voice=stdsyn_node_new(node->driver,&stdsyn_node_type_basic,1,1,noteid,velocity,0,0);
  if (stdsyn_node_basic_setup_fm(voice,NODE->carrier,NODE->fmabs,NODE->fmrate,&NODE->fmenv,&NODE->env,NODE->trim*NODE->master,NODE->pan)<0) {
    stdsyn_node_del(voice);
    return;
  }
  voice->chid=node->chid;
  if (stdsyn_node_mixer_add_voice(NDRIVER->main,voice)<0) {
  }
  stdsyn_node_del(voice);
}

/* Event.
 */
 
static int _ctlv_event(struct stdsyn_node *node,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  //fprintf(stderr,"%s %p %02x %02x %02x %02x\n",__func__,node,chid,opcode,a,b);
  switch (opcode) {
    case MIDI_OPCODE_NOTE_OFF: return 0; // Let the mixer handle these.
    case MIDI_OPCODE_NOTE_ON: _ctlv_note_on(node,a,b); return 1;
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
 
static int _ctlv_init(struct stdsyn_node *node,uint8_t velocity,const void *argv,int argc) {
  node->event=_ctlv_event;
  NODE->master=1.0f;
  NODE->trim=1.0f;
  NODE->pan=0.0f;
  if (!(NODE->carrier=stdsyn_wave_from_harmonics((void*)"\xff",1))) return -1;
  return 0;
}

/* Apply instrument.
 */
 
static int _ctlv_apply_instrument(struct stdsyn_node *node,const struct stdsyn_instrument *ins) {
  NODE->fmabs=ins->fmabs;
  NODE->fmrate=ins->fmrate;
  NODE->fmenv=ins->fmenv;
  NODE->env=ins->env;
  NODE->master=ins->master;
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_ctlv={
  .name="ctlv",
  .objlen=sizeof(struct stdsyn_node_ctlv),
  .del=_ctlv_del,
  .init=_ctlv_init,
  .apply_instrument=_ctlv_apply_instrument,
};

#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_mixer {
  struct stdsyn_node hdr;
  
  /* Channels for control purposes.
   * This is the event bus, visible to MIDI.
   */
  struct stdsyn_node *chanv[16];
  int fqpidv[16];
};

#define NODE ((struct stdsyn_node_mixer*)node)

/* Cleanup.
 */
 
static void _mixer_del(struct stdsyn_node *node) {
  int i=16; while (i-->0) stdsyn_node_del(NODE->chanv[i]);
}

/* Update.
 */
 
static void _mixer_update_mono(float *v,int c,struct stdsyn_node *node) {
  memset(v,0,sizeof(float)*c);
  int i=node->srcc;
  struct stdsyn_node **p=node->srcv+i-1;
  for (;i-->0;p--) {
    // Everything in (srcv) must be mono, non-overwrite, and have an update hook.
    struct stdsyn_node *child=*p;
    child->update(v,c,child);
  }
}

static void _mixer_update_stereo(float *v,int c,struct stdsyn_node *node) {
  /* TODO We do want full stereo support.
   * For now, just getting mono to work and expanding here.
   */
  int framec=c>>1;
  _mixer_update_mono(v,framec,node);
  float *dst=v+c;
  const float *src=v+framec;
  int i=framec; while (i-->0) {
    dst-=2;
    src-=1;
    dst[0]=dst[1]=src[0];
  }
}

/* Low-frequency update.
 */
 
static void _mixer_lfupdate(struct stdsyn_node *node) {
  int i=node->srcc;
  while (i-->0) {
    struct stdsyn_node *child=node->srcv[i];
    if (child->defunct) {
      stdsyn_node_srcv_remove_at(node,i,1);
      continue;
    }
    if (child->lfupdate) child->lfupdate(child);
  }
  for (i=0x10;i-->0;) {
    struct stdsyn_node *child=NODE->chanv[i];
    if (!child) continue;
    if (child->defunct) {
      NODE->chanv[i]=0;
      stdsyn_node_del(child);
      continue;
    }
    if (child->lfupdate) child->lfupdate(child);
  }
}

/* Begin PCM playback.
 */
 
static void mixer_begin_pcm(struct stdsyn_node *node,struct stdsyn_pcm *pcm) {
  struct stdsyn_node *child=stdsyn_node_spawn_source(node,&stdsyn_node_type_pcm,node->chanc,0,0x40,0x40);
  if (!child) return;
  if (!child->update) {
    stdsyn_node_srcv_remove(node,child);
    return;
  }
  stdsyn_node_pcm_set_pcm(child,pcm);
}

/* Release all voices.
 */
 
static void _mixer_release(struct stdsyn_node *node,uint8_t velocity) {
  //fprintf(stderr,"%s\n",__func__);
}

/* System reset.
 */
 
static void _mixer_reset(struct stdsyn_node *node) {
  int i=16; while (i-->0) {
    struct stdsyn_node *chan=NODE->chanv[i];
    if (!chan) continue;
    NODE->chanv[i]=0;
    stdsyn_node_del(chan);
    // No need to release voices or reset the channel -- we're dropping all voices cold too.
  }
  stdsyn_node_srcv_remove_at(node,0,node->srcc);
}

/* Set tempo.
 */
 
static void _mixer_tempo(struct stdsyn_node *node,int frames_per_qnote) {
  //fprintf(stderr,"%s %d f/q\n",__func__,frames_per_qnote);
}

/* End program on a given channel, and generate a new one from fqpidv.
 */
 
static void _mixer_change_program(struct stdsyn_node *node,uint8_t chid) {
  if (chid>=0x10) return;
  struct stdsyn_node *old=NODE->chanv[chid];
  NODE->chanv[chid]=0;
  if (old) {
    if (old->release) old->release(old,0x40);
    stdsyn_node_del(old);
  }
  struct stdsyn_instrument *instrument=stdsyn_res_store_get(&NDRIVER->instruments,NODE->fqpidv[chid]);
  fprintf(stderr,"!!! %s: Instantiate program 0x%x for channel %d. instrument=%p\n",__func__,NODE->fqpidv[chid],chid,instrument);//TODO
}

/* General event.
 */
 
static void _mixer_event(struct stdsyn_node *node,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  //fprintf(stderr,"%s %02x %02x %02x %02x\n",__func__,chid,opcode,a,b);
  
  /* Global events, ie invalid channel.
   */
  if (chid>=0x10) {
    switch (opcode) {
      case 0xff: _mixer_reset(node); break;
    }
    return;
  }
  
  /* Channel 15 is reserved for sound effects; it never has a program installed.
   */
  if (chid==0x0f) {
    if ((opcode!=0x90)&&(opcode!=0x98)) return; // Note On or Note Once
    struct stdsyn_pcm *pcm=stdsyn_res_store_get(&NDRIVER->sounds,a);
    if (pcm) mixer_begin_pcm(node,pcm);
    return;
  }
  
  /* Program Change and Bank Select must be picked off special.
   */
  if (opcode==MIDI_OPCODE_PROGRAM) {
    NODE->fqpidv[chid]=(NODE->fqpidv[chid]&~0x7f)|a;
    _mixer_change_program(node,chid);
    return;
  }
  if (opcode==MIDI_OPCODE_CONTROL) switch (a) {
    case MIDI_CTL_BANK_LSB: NODE->fqpidv[chid]=(NODE->fqpidv[chid]&~0x3f80)|(b<<7); return;
    case MIDI_CTL_BANK_MSB: NODE->fqpidv[chid]=(NODE->fqpidv[chid]&~0x1fe000)|(b<<14); return;
  }
  
  /* If no program was established, drop the event.
   * I'm not sure what MIDI says about Control Changes before a Program Change, but we forbid it.
   * Our policy is every channel you use, you must begin with a Program Change.
   * Also along those lines: There isn't a default program. You can't just fire a Note On to some uninitialized channel (tho MIDI definitely says you can).
   */
  struct stdsyn_node *chan=NODE->chanv[chid];
  if (!chan) return;
  
  /* Everything else is the controller's problem, cheers!
   */
  chan->event(chan,chid,opcode,a,b);
}

/* Init.
 */
 
static int _mixer_init(struct stdsyn_node *node,uint8_t velocity) {

  switch (node->chanc) {
    case 1: node->update=_mixer_update_mono; break;
    case 2: node->update=_mixer_update_stereo; break;
    default: return -1;
  }
  node->lfupdate=_mixer_lfupdate;
  node->release=_mixer_release;
  node->event=_mixer_event;
  node->tempo=_mixer_tempo;

  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_mixer={
  .name="mixer",
  .objlen=sizeof(struct stdsyn_node_mixer),
  .del=_mixer_del,
  .init=_mixer_init,
};

#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_mixer {
  struct stdsyn_node hdr;
  
  /* Channels for control purposes.
   * This is the event bus, visible to MIDI.
   */
  struct stdsyn_node *chanv[16];
  int fqpidv[16];
  
  float buf[STDSYN_BUFFER_SIZE];
};

#define NODE ((struct stdsyn_node_mixer*)node)

/* Cleanup.
 */
 
static void _mixer_del(struct stdsyn_node *node) {
  int i=16; while (i-->0) stdsyn_node_del(NODE->chanv[i]);
}

/* Update mono-mono for an overwrite source.
 */
 
static void mixer_update_mono_mono(float *dst,int c,struct stdsyn_node *node,struct stdsyn_node *source) {
  float *src=NODE->buf;
  source->update(src,c,source);
  for (;c-->0;src++,dst++) (*dst)+=*src;
}

/* Update.
 */
 
static int srccpv=0;
 
static void _mixer_update_mono(float *v,int c,struct stdsyn_node *node) {
  /**
  if (node->srcc!=srccpv) {
    fprintf(stderr," mixer srcc %d\n",node->srcc);
    srccpv=node->srcc;
  }
  /**/
  memset(v,0,sizeof(float)*c);
  int i=node->srcc;
  struct stdsyn_node **p=node->srcv+i-1;
  for (;i-->0;p--) {
    // Everything in (srcv) must be mono, and have an update hook.
    struct stdsyn_node *child=*p;
    if (child->overwrite) mixer_update_mono_mono(v,c,node,child);
    else child->update(v,c,child);
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
  //return; // works, just muting to focus on tuned notes
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
  int i=node->srcc;
  while (i-->0) {
    struct stdsyn_node *child=node->srcv[i];
    if (child->release) child->release(child,0x40);
    else if (child->event) child->event(child,child->chid,MIDI_OPCODE_NOTE_OFF,child->noteid,0x40);
    else child->defunct=1;
  }
  for (i=0x10;i-->0;) {
    struct stdsyn_node *child=NODE->chanv[i];
    if (!child) continue;
    if (child->release) child->release(child,0x40);
    else if (child->event) child->event(child,child->chid,MIDI_OPCODE_NOTE_OFF,child->noteid,0x40);
    else child->defunct=1;
  }
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
  //fprintf(stderr,"!!! %s: Instantiate program 0x%x for channel %d. instrument=%p(%d)\n",__func__,NODE->fqpidv[chid],chid,instrument,instrument?instrument->c:0);//TODO
  if (!instrument||(instrument->c<1)) return;
  struct stdsyn_node *program=stdsyn_node_new_controller(node->driver,node->chanc,0,instrument->v,instrument->c);
  if (!program) {
    fprintf(stderr,"!!! Failed to instantiate instrument %d, c=%d\n",NODE->fqpidv[chid],instrument->c);
    return;
  }
  program->chid=chid;
  NODE->chanv[chid]=program;
  if (program->update) {
    stdsyn_node_srcv_insert(node,-1,program);
  }
}

/* General event.
 */
 
static int _mixer_event(struct stdsyn_node *node,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  //fprintf(stderr,"%s %02x %02x %02x %02x\n",__func__,chid,opcode,a,b);
  
  /* Global events, ie invalid channel.
   */
  if (chid>=0x10) {
    switch (opcode) {
      case 0xff: _mixer_reset(node); break;
    }
    return 1;
  }
  
  /* Channel 15 is reserved for sound effects; it never has a program installed.
   */
  if (chid==0x0f) {
    if ((opcode!=0x90)&&(opcode!=0x98)) return 1; // Note On or Note Once
    struct stdsyn_pcm *pcm=stdsyn_res_store_get(&NDRIVER->sounds,a);
    if (pcm) mixer_begin_pcm(node,pcm);
    return 1;
  }
  
  /* Program Change and Bank Select must be picked off special.
   */
  if (opcode==MIDI_OPCODE_PROGRAM) {
    NODE->fqpidv[chid]=(NODE->fqpidv[chid]&~0x7f)|a;
    _mixer_change_program(node,chid);
    return 1;
  }
  if (opcode==MIDI_OPCODE_CONTROL) switch (a) {
    case MIDI_CTL_BANK_LSB: NODE->fqpidv[chid]=(NODE->fqpidv[chid]&~0x3f80)|(b<<7); return 1;
    case MIDI_CTL_BANK_MSB: NODE->fqpidv[chid]=(NODE->fqpidv[chid]&~0x1fe000)|(b<<14); return 1;
  }
  
  /* If no program was established, drop the event.
   * I'm not sure what MIDI says about Control Changes before a Program Change, but we forbid it.
   * Our policy is every channel you use, you must begin with a Program Change.
   * Also along those lines: There isn't a default program. You can't just fire a Note On to some uninitialized channel (tho MIDI definitely says you can).
   */
  struct stdsyn_node *chan=NODE->chanv[chid];
  if (!chan) return 0;
  
  /* Note Off, give controller first look, but then examine attached voices if it declines.
   */
  if (opcode==MIDI_OPCODE_NOTE_OFF) {
    if (chan->event(chan,chid,opcode,a,b)>0) return 1;
    int i=node->srcc;
    while (i-->0) {
      struct stdsyn_node *voice=node->srcv[i];
      if ((voice->chid==chid)&&(voice->noteid==a)) {
        if (voice->release) voice->release(voice,0x40);
        else if (voice->event) voice->event(voice,chid,opcode,a,b);
        else voice->defunct=1;
        voice->chid=voice->noteid=0xff;
      }
    }
    return 1;
  }
  
  /* Everything else is the controller's problem, cheers!
   */
  return chan->event(chan,chid,opcode,a,b);
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

/* Public. Add a voice node.
 */
 
int stdsyn_node_mixer_add_voice(struct stdsyn_node *node,struct stdsyn_node *voice) {
  if (!node||(node->type!=&stdsyn_node_type_mixer)) return -1;
  if (!voice) return -1;
  if (!voice->update) return -1;
  switch (node->chanc) {
    case 1: {
        if (voice->chanc!=1) return -1;
      } break;
    case 2: {
        if ((voice->chanc<1)||(voice->chanc>2)) return -1;
      } break;
    default: return -1;
  }
  return stdsyn_node_srcv_insert(node,-1,voice);
}

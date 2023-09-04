/* stdsyn_node_fm.c
 * FM oscillator.
 * Takes an encoded pipe command, see PFM_* in etc/doc/instrument-format.md.
 */
 
#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_fm {
  struct stdsyn_node hdr;
  struct stdsyn_wave_runner carrier,modulator;
  struct stdsyn_env env;
  uint8_t rangebufid;
  float halfrangek;
  float buf[STDSYN_BUFFER_SIZE];
};

#define NODE ((struct stdsyn_node_fm*)node)

/* Cleanup.
 */
 
static void _fm_del(struct stdsyn_node *node) {
  stdsyn_wave_runner_cleanup(&NODE->carrier);
  stdsyn_wave_runner_cleanup(&NODE->modulator);
}

/* Update.
 */
 
static void _fm_update(float *v,int c,struct stdsyn_node *node) {
  stdsyn_wave_runner_update(NODE->buf,c,&NODE->modulator);
  float *p=NODE->buf;
  int i=c;
  for (;i-->0;p++) (*p)*=stdsyn_env_update(NODE->env);
  stdsyn_wave_runner_update_mod(v,c,&NODE->carrier,NODE->buf);
}

static void _fm_update_lfo(float *v,int c,struct stdsyn_node *node) {
  stdsyn_wave_runner_update(NODE->buf,c,&NODE->modulator);
  const float *rsrc=stdsyn_node_get_buffer(node,NODE->rangebufid);
  if (rsrc) {
    int i=c;
    float *p=NODE->buf;
    for (;i-->0;rsrc++,p++) {
      float m=((*rsrc)+1.0f)*NODE->halfrangek;
      (*p)*=m;
    }
  }
  stdsyn_wave_runner_update_mod(v,c,&NODE->carrier,NODE->buf);
}

/* Release.
 */
 
static void _fm_release(struct stdsyn_node *node,uint8_t velocity) {
  stdsyn_env_release(&NODE->env);
}

/* Init.
 */
 
static int _fm_init(struct stdsyn_node *node,uint8_t velocity,const void *argv,int argc) {
  const uint8_t *ARGV=argv;
  if (!node->overwrite) return -1;
  if (node->chanc!=1) return -1;
  if (argc<8) return -1;
  
  // We must call ourselves defunct immediately because we never actually terminate, but we do implement 'release'. (for FM env transition)
  node->defunct=1;
  
  struct stdsyn_wave *wave=stdsyn_wave_get_sine();
  if (!wave) return -1;
  if (
    (stdsyn_wave_runner_set_wave(&NODE->carrier,wave)<0)||
    (stdsyn_wave_runner_set_wave(&NODE->modulator,wave)<0)
  ) {
    stdsyn_wave_del(wave);
    return -1;
  }
  stdsyn_wave_del(wave);
  
  float carrate=(ARGV[1]<<8)|ARGV[2];
  float modrate=(ARGV[3]<<8)|ARGV[4];
  switch (ARGV[0]) {
    case 0x06: { // PFM_A_A; modrate is u12.4
        modrate/=16.0f;
      } break;
    case 0x07: { // PFM_R_A; u8.8, u12.4
        carrate/=256.0f;
        carrate*=midi_note_frequency[node->noteid&0x7f];
        modrate/=16.0f;
      } break;
    case 0x08: { // PFM_A_R
        modrate/=256.0f;
        modrate*=carrate;
      } break;
    case 0x09: { // PFM_R_R
        carrate/=256.0f;
        carrate*=midi_note_frequency[node->noteid&0x7f];
        modrate/=256.0f;
        modrate*=carrate;
      } break;
    default: return -1;
  }
  float range=((ARGV[5]<<8)|ARGV[6])/256.0f;
  NODE->halfrangek=range*0.5f;
  stdsyn_wave_runner_set_rate_hz(&NODE->carrier,carrate,node->driver->rate);
  stdsyn_wave_runner_set_rate_hz(&NODE->modulator,modrate,node->driver->rate);
  
  node->update=_fm_update;
  node->release=_fm_release;
  
  if ((ARGV[7]&0xf0)==0xf0) { // Buffer source for range.
    NODE->rangebufid=ARGV[7]&0x03;
    node->update=_fm_update_lfo;
  } else { // Envelope for range.
    if (stdsyn_env_decode(&NODE->env,ARGV+7,argc-7)<0) return -1;
    stdsyn_env_multiply(&NODE->env,range);
    stdsyn_env_reset(&NODE->env,velocity,node->driver->rate);
  }
  
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_fm={
  .name="fm",
  .objlen=sizeof(struct stdsyn_node_fm),
  .del=_fm_del,
  .init=_fm_init,
};

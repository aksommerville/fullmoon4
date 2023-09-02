/* stdsyn_node_minsyn.c
 * Voice node for "ctlm". Minsyn format.
 */
 
#include "../fmn_stdsyn_internal.h"
 
struct stdsyn_node_minsyn {
  struct stdsyn_node hdr;
  struct stdsyn_wave_runner wave;
  struct stdsyn_wave_runner mixwave;
  struct stdsyn_env env;
  struct stdsyn_env mixenv;
  uint8_t velocity;
  uint8_t noteid;
};

#define NODE ((struct stdsyn_node_minsyn*)node)

/* Cleanup.
 */
 
static void _minsyn_del(struct stdsyn_node *node) {
  stdsyn_wave_runner_cleanup(&NODE->wave);
  stdsyn_wave_runner_cleanup(&NODE->mixwave);
}

/* Update.
 */
 
static void _minsyn_update_single(float *v,int c,struct stdsyn_node *node) {
  stdsyn_wave_runner_update(v,c,&NODE->wave);
  for (;c-->0;v++) {
    float level=stdsyn_env_update(NODE->env);
    (*v)*=level;
  }
  if (NODE->env.finished) {
    node->defunct=1;
  }
}

static void _minsyn_update_mix(float *v,int c,struct stdsyn_node *node) {
  for (;c-->0;v++) {
    float level=stdsyn_env_update(NODE->env);
    float mix=stdsyn_env_update(NODE->mixenv);
    float a=stdsyn_wave_runner_step(&NODE->wave);
    float b=stdsyn_wave_runner_step(&NODE->mixwave);
    *v=((a*mix)+(b*(1.0f-mix)))*level;
  }
  if (NODE->env.finished) {
    node->defunct=1;
  }
}

static void _minsyn_update_noop(float *v,int c,struct stdsyn_node *node) {
  memset(v,0,sizeof(float)*c);
  node->defunct=1;
}

/* Release.
 */
 
static void _minsyn_release(struct stdsyn_node *node,uint8_t velocity) {
  //fprintf(stderr,"%s %02x\n",__func__,NODE->noteid);
  stdsyn_env_release(&NODE->env);
  stdsyn_env_release(&NODE->mixenv);
}

/* Init.
 */
 
static int _minsyn_init(struct stdsyn_node *node,uint8_t velocity) {
  if (!node->overwrite) return -1;
  NODE->noteid=node->noteid;
  NODE->velocity=velocity;
  node->update=_minsyn_update_noop;
  node->release=_minsyn_release;
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_minsyn={
  .name="minsyn",
  .objlen=sizeof(struct stdsyn_node_minsyn),
  .del=_minsyn_del,
  .init=_minsyn_init,
};

/* Setup.
 */
 
int stdsyn_node_minsyn_setup(
  struct stdsyn_node *node,
  struct stdsyn_wave *wave,
  struct stdsyn_wave *mixwave,
  const struct stdsyn_env *env,
  const struct stdsyn_env *mixenv,
  float trim,float pan
) {
  if (!node||(node->type!=&stdsyn_node_type_minsyn)) return -1;
  
  if (wave) {
    if (!env) return -1;
    NODE->env=*env;
    if (stdsyn_wave_runner_set_wave(&NODE->wave,wave)<0) return -1;
    //fprintf(stderr,"minsyn %02x attack level %f naturally\n",NODE->noteid,NODE->env.atkvhi);
    stdsyn_env_multiply(&NODE->env,trim);
    //fprintf(stderr,"...after applying trim %f: %f\n",trim,NODE->env.atkvhi);
    stdsyn_env_reset(&NODE->env,NODE->velocity,node->driver->rate);
  }
  
  if (mixwave) {
    if (!mixenv) return -1;
    if (stdsyn_wave_runner_set_wave(&NODE->mixwave,mixwave)<0) return -1;
    NODE->mixenv=*mixenv;
    stdsyn_env_multiply(&NODE->env,trim);
    stdsyn_env_reset(&NODE->mixenv,NODE->velocity,node->driver->rate);
  }
  
  if (!wave) node->update=_minsyn_update_noop;
  else if (mixwave) node->update=_minsyn_update_mix;
  else node->update=_minsyn_update_single;
  
  stdsyn_wave_runner_set_rate_hz(&NODE->wave,midi_note_frequency[NODE->noteid&0x7f],node->driver->rate);
  NODE->mixwave.dp=NODE->wave.dp;
  
  return 0;
}

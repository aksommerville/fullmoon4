/* stdsyn_node_basic.c
 * FM oscillator and level envelope, for one discrete voice.
 */
 
#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_basic {
  struct stdsyn_node hdr;
  struct stdsyn_wave_runner car;
  struct stdsyn_wave_runner mod;
  struct stdsyn_env env;
  struct stdsyn_env range;
  uint8_t noteid;
  uint8_t velocity;
  float buf[STDSYN_BUFFER_SIZE];
};

#define NODE ((struct stdsyn_node_basic*)node)

/* Cleanup.
 */
 
static void _basic_del(struct stdsyn_node *node) {
  stdsyn_wave_runner_cleanup(&NODE->car);
  stdsyn_wave_runner_cleanup(&NODE->mod);
}

/* Update.
 */
 
static void _basic_update_mono_overwrite(float *v,int c,struct stdsyn_node *node) {
  stdsyn_wave_runner_update(NODE->buf,c,&NODE->mod);
  int i=c; 
  float *m=NODE->buf;
  for (;i-->0;m++) (*m)*=stdsyn_env_update(NODE->range);
  stdsyn_wave_runner_update_mod(v,c,&NODE->car,NODE->buf);
  for (;c-->0;v++) {
    float level=stdsyn_env_update(NODE->env);
    (*v)*=level;
  }
  if (NODE->env.finished) node->defunct=1;
}

/* Release.
 */
 
static void _basic_release(struct stdsyn_node *node,uint8_t velocity) {
  stdsyn_env_release(&NODE->env);
  stdsyn_env_release(&NODE->range);
}

/* Init.
 */
 
static int _basic_init(struct stdsyn_node *node,uint8_t velocity) {
  if (node->chanc!=1) return -1;
  if (!node->overwrite) return -1;
  node->update=_basic_update_mono_overwrite;
  node->release=_basic_release;
  NODE->noteid=node->noteid;
  NODE->velocity=velocity;
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_basic={
  .name="basic",
  .objlen=sizeof(struct stdsyn_node_basic),
  .del=_basic_del,
  .init=_basic_init,
};

/* Setup.
 */
 
int stdsyn_node_basic_setup_fm(
  struct stdsyn_node *node,
  struct stdsyn_wave *carrier,
  int abs_mod_rate,
  float rate,
  const struct stdsyn_env *rangeenv,
  const struct stdsyn_env *env,
  float trim,float pan
) {
  if (!node||(node->type!=&stdsyn_node_type_basic)) return -1;
  if (stdsyn_wave_runner_set_wave(&NODE->car,carrier)<0) return -1;
  if (stdsyn_wave_runner_set_wave(&NODE->mod,carrier)<0) return -1;
  stdsyn_wave_runner_set_rate_hz(&NODE->car,midi_note_frequency[NODE->noteid&0x7f],node->driver->rate);
  if (abs_mod_rate) {
    stdsyn_wave_runner_set_rate_hz(&NODE->mod,rate,node->driver->rate);
  } else {
    stdsyn_wave_runner_set_rate_hz(&NODE->mod,rate*midi_note_frequency[NODE->noteid&0x7f],node->driver->rate);
  }
  NODE->range=*rangeenv;
  NODE->env=*env;
  stdsyn_env_multiply(&NODE->env,trim);
  stdsyn_env_reset(&NODE->env,NODE->velocity,node->driver->rate);
  stdsyn_env_reset(&NODE->range,NODE->velocity,node->driver->rate);
  return 0;
}

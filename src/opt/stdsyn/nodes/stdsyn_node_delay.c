/* stdsyn_node_delay.c
 * argv comes straight off the encoded pipe, including opcode:
 *   0x0f DELAY_A (u16 ms,u0.8 dry,u0.8 wet,u0.8 store,u0.8 feedback)
 *   0x10 DELAY_R (u8.8 qnotes,u0.8 dry,u0.8 web,u0.8 store,u0.8 feedback)
 */
 
#include "../fmn_stdsyn_internal.h"
#include <math.h>

/* 64k is about a second and a half, at 44.1 kHz.
 * That's long enough to probably be a mistake.
 */
#define DELAY_SANITY_LIMIT 65536

struct stdsyn_node_delay {
  struct stdsyn_node hdr;
  int period;
  float dry,wet,sto,fbk;
  float *buf;
  int bufp;
};

#define NODE ((struct stdsyn_node_delay*)node)

/* Cleanup.
 */
 
static void _delay_del(struct stdsyn_node *node) {
  if (NODE->buf) free(NODE->buf);
}

/* Update.
 */

static void _delay_update(float *v,int c,struct stdsyn_node *node) {
  for (;c-->0;v++) {
    float in=*v;
    float pv=NODE->buf[NODE->bufp];
    *v=in*NODE->dry+pv*NODE->wet;
    NODE->buf[NODE->bufp]=in*NODE->sto+pv*NODE->fbk;
    NODE->bufp++;
    if (NODE->bufp>=NODE->period) NODE->bufp=0;
  }
}

/* Init.
 */
 
static int _delay_init(struct stdsyn_node *node,uint8_t velocity,const void *argv,int argc) {
  const uint8_t *ARGV=argv;
  if (node->overwrite) return -1;
  if (node->chanc!=1) return -1;
  if (argc!=7) return -1;
  
  int mode=ARGV[0]; // pipe opcode
  float period=(ARGV[1]<<8)|ARGV[2];
  if (mode==15) { // absolute ms
    period=(period*node->driver->rate)/1000.0f;
  } else if (mode==16) { // u8.8 qnotes
    period/=256.0f;
    period*=NDRIVER->tempo;
  } else {
    return -1;
  }
  NODE->period=period;
  if (NODE->period<1) NODE->period=1;
  else if (NODE->period>DELAY_SANITY_LIMIT) NODE->period=DELAY_SANITY_LIMIT;
  if (!(NODE->buf=calloc(sizeof(float),NODE->period))) return -1;
  
  NODE->dry=ARGV[3]/255.0f;
  NODE->wet=ARGV[4]/255.0f;
  NODE->sto=ARGV[5]/255.0f;
  NODE->fbk=ARGV[6]/255.0f;
  
  node->update=_delay_update;
  
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_delay={
  .name="delay",
  .objlen=sizeof(struct stdsyn_node_delay),
  .del=_delay_del,
  .init=_delay_init,
};

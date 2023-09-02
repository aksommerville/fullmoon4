#include "fmn_stdsyn_internal.h"

/* Delete.
 */
 
void stdsyn_instrument_del(struct stdsyn_instrument *ins) {
  if (!ins) return;
  stdsyn_wave_del(ins->wave);
  stdsyn_wave_del(ins->mixwave);
  free(ins);
}

/* Decode stdsyn, individual commands.
 */
 
static int stdsyn_instrument_decode_FM_A_S(struct stdsyn_instrument *ins,const uint8_t *src,int srcc) {
  // | 0x01   | FM_A_S      | (u12.4 rate_hz,u8.8 range)
  if (srcc<4) return -1;
  ins->fmabs=1;
  uint16_t n=(src[0]<<8)|src[1];
  ins->fmrate=n/16.0f;
  n=(src[2]<<8)|src[3];
  float range=n/256.0f;
  stdsyn_env_init_constant(&ins->fmenv,range);
  return 4;
}
 
static int stdsyn_instrument_decode_FM_R_S(struct stdsyn_instrument *ins,const uint8_t *src,int srcc) {
  // | 0x02   | FM_R_S      | (u8.8 rate_mlt,u8.8 range)
  if (srcc<4) return -1;
  ins->fmabs=0;
  uint16_t n=(src[0]<<8)|src[1];
  ins->fmrate=n/256.0f;
  n=(src[2]<<8)|src[3];
  float range=n/256.0f;
  stdsyn_env_init_constant(&ins->fmenv,range);
  return 4;
}
 
static int stdsyn_instrument_decode_FM_A_E(struct stdsyn_instrument *ins,const uint8_t *src,int srcc) {
  // | 0x03   | FM_A_E      | (u12.4 rate_hz,u8.8 range,...range_env)
  if (srcc<4) return -1;
  ins->fmabs=1;
  uint16_t n=(src[0]<<8)|src[1];
  ins->fmrate=n/16.0f;
  n=(src[2]<<8)|src[3];
  float range=n/256.0f;
  int err=stdsyn_env_decode(&ins->fmenv,src+4,srcc-4);
  if (err<0) return err;
  stdsyn_env_multiply(&ins->fmenv,range);
  return 4+err;
}
 
static int stdsyn_instrument_decode_FM_R_E(struct stdsyn_instrument *ins,const uint8_t *src,int srcc) {
  // | 0x04   | FM_R_E      | (u8.8 rate_mlt,u8.8 range,...range_env)
  if (srcc<4) return -1;
  ins->fmabs=0;
  uint16_t n=(src[0]<<8)|src[1];
  ins->fmrate=n/256.0f;
  n=(src[2]<<8)|src[3];
  float range=n/256.0f;
  int err=stdsyn_env_decode(&ins->fmenv,src+4,srcc-4);
  if (err<0) return err;
  stdsyn_env_multiply(&ins->fmenv,range);
  return 4+err;
}
 
static int stdsyn_instrument_decode_STDENV(struct stdsyn_instrument *ins,const uint8_t *src,int srcc) {
  // | 0x05   | STDENV      | (...env)
  return stdsyn_env_decode(&ins->env,src,srcc);
}

/* Decode stdsyn format.
 */
 
static int stdsyn_instrument_decode_stdsyn(struct stdsyn_instrument *ins,const uint8_t *src,int srcc) {
  ins->type=&stdsyn_node_type_ctlv; // TODO 'ctl3' and 'ctlp' will probly also use this eventually
  int srcp=0;
  while (srcp<srcc) {
    int err=0;
    switch (src[srcp++]) {
      case 0x01: err=stdsyn_instrument_decode_FM_A_S(ins,src+srcp,srcc-srcp); break;
      case 0x02: err=stdsyn_instrument_decode_FM_R_S(ins,src+srcp,srcc-srcp); break;
      case 0x03: err=stdsyn_instrument_decode_FM_A_E(ins,src+srcp,srcc-srcp); break;
      case 0x04: err=stdsyn_instrument_decode_FM_R_E(ins,src+srcp,srcc-srcp); break;
      case 0x05: err=stdsyn_instrument_decode_STDENV(ins,src+srcp,srcc-srcp); break;
      case 0xc0: break;
      default: {
          fprintf(stderr,"stdsyn: Unknown instrument opcode 0x%02x\n",src[srcp-1]);
          return -1;
        }
    }
    if ((err<0)||(err>srcc-srcp)) return -1;
    srcp+=err;
  }
  return 0;
}

/* Decode, minsyn format.
 */
 
static int stdsyn_instrument_decode_minsyn(struct stdsyn_instrument *ins,const uint8_t *src,int srcc) {
  if (srcc<1) return -1;
  uint8_t features=src[0];
  if (features&0xc0) return -1;
  int srcp=1;
  
  ins->type=&stdsyn_node_type_ctlm;
  
  if (features&0x01) { // main harmonics
    if (srcp>srcc-1) return -1;
    uint8_t coefc=src[srcp++];
    if (srcp>srcc-coefc) return -1;
    if (!(ins->wave=stdsyn_wave_from_harmonics(src+srcp,coefc))) return -1;
    srcp+=coefc;
  } else {
    if (!(ins->wave=stdsyn_wave_from_harmonics("\xff",1))) return -1;
  }
  
  switch (features&0x06) { // env
    case 0: stdsyn_env_default(&ins->env); break;
    case 0x02: {
        if (srcp>srcc-5) return -1;
        stdsyn_env_decode_lo(&ins->env,src+srcp);
        stdsyn_env_decode_hi(&ins->env,0);
        srcp+=5;
      } break;
    case 0x04: {
        if (srcp>srcc-5) return -1;
        stdsyn_env_decode_hi(&ins->env,src+srcp);
        stdsyn_env_decode_lo(&ins->env,0);
        srcp+=5;
      } break;
    case 0x06: {
        if (srcp>srcc-10) return -1;
        stdsyn_env_decode_lo(&ins->env,src+srcp);
        srcp+=5;
        stdsyn_env_decode_hi(&ins->env,src+srcp);
        srcp+=5;
      } break;
  }

  if (features&0x08) { // mix harmonics
    if (srcp>srcc-1) return -1;
    uint8_t coefc=src[srcp++];
    if (srcp>srcc-coefc) return -1;
    if (!(ins->mixwave=stdsyn_wave_from_harmonics(src+srcp,coefc))) return -1;
    srcp+=coefc;
  } else {
    // null is ok, for no mixing
  }
  
  switch (features&0x30) { // mix env
    case 0: stdsyn_env_default(&ins->mixenv); break;
    case 0x10: {
        if (srcp>srcc-5) return -1;
        stdsyn_env_decode_lo(&ins->mixenv,src+srcp);
        stdsyn_env_decode_hi(&ins->mixenv,0);
        srcp+=5;
      } break;
    case 0x20: {
        if (srcp>srcc-5) return -1;
        stdsyn_env_decode_hi(&ins->mixenv,src+srcp);
        stdsyn_env_decode_lo(&ins->mixenv,0);
        srcp+=5;
      } break;
    case 0x30: {
        if (srcp>srcc-10) return -1;
        stdsyn_env_decode_lo(&ins->mixenv,src+srcp);
        srcp+=5;
        stdsyn_env_decode_hi(&ins->mixenv,src+srcp);
        srcp+=5;
      } break;
  }
  return 0;
}

/* Decode.
 */
 
struct stdsyn_instrument *stdsyn_instrument_decode(struct bigpc_synth_driver *driver,int id,const void *v,int c) {
  struct stdsyn_instrument *instrument=calloc(1,sizeof(struct stdsyn_instrument));
  if (!instrument) return 0;
  
  int err=-1;
  if ((c>=1)&&((*(unsigned char*)v)&0xc0)) err=stdsyn_instrument_decode_stdsyn(instrument,v,c);
  else err=stdsyn_instrument_decode_minsyn(instrument,v,c);
  if (err<0) {
    fprintf(stderr,"stdsyn: Decode instrument failed id=%d c=%d\n",id,c);
    stdsyn_instrument_del(instrument);
    return 0;
  }
  
  return instrument;
}

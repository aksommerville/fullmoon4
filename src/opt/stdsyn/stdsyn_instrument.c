#include "fmn_stdsyn_internal.h"
#include "bits/stdsyn_pipe.h"

/* Delete.
 */
 
void stdsyn_instrument_del(struct stdsyn_instrument *ins) {
  if (!ins) return;
  stdsyn_wave_del(ins->wave);
  stdsyn_wave_del(ins->mixwave);
  if (ins->prefix) { stdsyn_pipe_config_cleanup(ins->prefix); free(ins->prefix); }
  if (ins->voice) { stdsyn_pipe_config_cleanup(ins->voice); free(ins->voice); }
  if (ins->suffix) { stdsyn_pipe_config_cleanup(ins->suffix); free(ins->suffix); }
  free(ins);
}

/* Decode individual pipe commands.
 */
 
static int stdsyn_pipe_decode_SILENCE(struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  cmd->overwrite=1;
  cmd->type=&stdsyn_node_type_oscillator;
  if (!(cmd->argv=malloc(1))) return -1;
  ((uint8_t*)cmd->argv)[0]=0x00;
  cmd->argc=1;
  return 0;
}
 
static int stdsyn_pipe_decode_NOISE(struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  cmd->overwrite=1;
  cmd->type=&stdsyn_node_type_oscillator;
  if (!(cmd->argv=malloc(1))) return -1;
  ((uint8_t*)cmd->argv)[0]=0x01;
  cmd->argc=1;
  return 0;
}
 
static int stdsyn_pipe_decode_COPY(struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  if (srcc<1) return -1;
  cmd->overwrite=1;
  cmd->type=&stdsyn_node_type_mixbuf;
  if (!(cmd->argv=malloc(2))) return -1;
  cmd->argc=2;
  uint8_t *DST=cmd->argv;
  DST[0]=src[0]; // bufid
  DST[1]=0xff; // mix amt
  return 1;
}
 
static int stdsyn_pipe_decode_PENV(struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  cmd->overwrite=0;
  cmd->type=&stdsyn_node_type_env;
  int len=stdsyn_env_decode(0,src,srcc);
  if (len<0) return -1;
  if (!(cmd->argv=malloc(len))) return -1;
  memcpy(cmd->argv,src,len);
  cmd->argc=len;
  return len;
}
 
static int stdsyn_pipe_decode_OSC(int relative,struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  cmd->overwrite=1;
  cmd->type=&stdsyn_node_type_oscillator;
  if (srcc<3) return -1;
  int coefc=src[2];
  if (3+coefc>srcc) return -1;
  cmd->argc=1+3+coefc;
  if (!(cmd->argv=malloc(cmd->argc))) return -1;
  memcpy(cmd->argv,src-1,cmd->argc);
  ((uint8_t*)cmd->argv)[0]&=0x3f;
  return 3+coefc;
}
 
static int stdsyn_pipe_decode_PFM(int rel_car,int rel_mod,struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  if (srcc<6) return -1;
  int srcp=6;
  int envlen;
  if ((srcc>=srcp+1)&&((src[srcp]&0xf0)==0xf0)) envlen=1; // buffer source
  else envlen=stdsyn_env_decode(0,src+srcp,srcc-srcp);
  if (envlen<0) return -1;
  srcp+=envlen;
  
  cmd->overwrite=1;
  cmd->type=&stdsyn_node_type_fm;
  cmd->argc=1+srcp; // +1 to add the opcode from src[-1]
  if (!(cmd->argv=malloc(cmd->argc))) return -1;
  memcpy(cmd->argv,src-1,cmd->argc);
  (*(uint8_t*)cmd->argv)&=0x3f;
  
  return srcp;
}
 
static int stdsyn_pipe_decode_BANDPASS(int relative,struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  if (srcc<4) return -1;
  cmd->overwrite=0;
  cmd->type=&stdsyn_node_type_filter;
  
  uint8_t *argv=malloc(5);
  if (!argv) return -1;
  if (relative) {
    argv[0]=5;
  } else {
    argv[0]=3;
  }
  argv[1]=src[0];
  argv[2]=src[1];
  argv[3]=src[2]; // band width
  argv[4]=src[3];
  cmd->argv=argv;
  cmd->argc=5;
  
  return 4;
}
 
static int stdsyn_pipe_decode_LOPASS(struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  if (srcc<2) return -1;
  cmd->overwrite=0;
  cmd->type=&stdsyn_node_type_filter;
  
  uint8_t *argv=malloc(3);
  if (!argv) return -1;
  argv[0]=1;
  argv[1]=src[0];
  argv[2]=src[1];
  cmd->argv=argv;
  cmd->argc=3;

  return 2;
}
 
static int stdsyn_pipe_decode_HIPASS(struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  if (srcc<2) return -1;
  cmd->overwrite=0;
  cmd->type=&stdsyn_node_type_filter;
  
  uint8_t *argv=malloc(3);
  if (!argv) return -1;
  argv[0]=2;
  argv[1]=src[0];
  argv[2]=src[1];
  cmd->argv=argv;
  cmd->argc=3;

  return 2;
}
 
static int stdsyn_pipe_decode_GAIN(struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  if (srcc<4) return -1;
  cmd->overwrite=0;
  cmd->type=&stdsyn_node_type_gain;
  if (!(cmd->argv=malloc(4))) return -1;
  memcpy(cmd->argv,src,4);
  cmd->argc=4;
  return 4;
}
 
static int stdsyn_pipe_decode_DELAY(int relative,struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  if (srcc<6) return -1;
  cmd->overwrite=0;
  cmd->type=&stdsyn_node_type_delay;
  if (!(cmd->argv=malloc(7))) return -1;
  memcpy(cmd->argv,src-1,7);
  ((uint8_t*)cmd->argv)[0]&=0x3f;
  cmd->argc=7;
  return 6;
}
 
static int stdsyn_pipe_decode_ADD(struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  if (srcc<1) return -1;
  cmd->overwrite=0;
  cmd->type=&stdsyn_node_type_mixbuf;
  if (!(cmd->argv=malloc(1))) return -1;
  cmd->argc=1;
  uint8_t *DST=cmd->argv;
  DST[0]=src[0]; // bufid
  // Omit second byte (mix amt) for straight add.
  return 1;
}
 
static int stdsyn_pipe_decode_MLT(struct stdsyn_pipe_config_cmd *cmd,const uint8_t *src,int srcc) {
  if (srcc<2) return -1;
  cmd->overwrite=0;
  cmd->type=&stdsyn_node_type_mlt;
  if (!(cmd->argv=malloc(2))) return -1;
  memcpy(cmd->argv,src,2);
  cmd->argc=2;
  return 2;
}

/* Decode pipe config.
 */
 
static int stdsyn_pipe_config_decode(struct stdsyn_pipe_config *config,const uint8_t *src,int srcc) {
  int srcp=0;
  while (srcp<srcc) {
    uint8_t opcode=src[srcp++];
    uint8_t bufid=opcode>>6;
    opcode&=0x3f;
    
    struct stdsyn_pipe_config_cmd *cmd=stdsyn_pipe_config_add_command(config);
    if (!cmd) return -1;
    cmd->bufid=bufid;
    
    int err=-1;
    switch (opcode) {
      case 0x00: err=stdsyn_pipe_decode_SILENCE(cmd,src+srcp,srcc-srcp); break;
      case 0x01: err=stdsyn_pipe_decode_NOISE(cmd,src+srcp,srcc-srcp); break;
      case 0x02: err=stdsyn_pipe_decode_COPY(cmd,src+srcp,srcc-srcp); break;
      case 0x03: err=stdsyn_pipe_decode_PENV(cmd,src+srcp,srcc-srcp); break;
      case 0x04: err=stdsyn_pipe_decode_OSC(0,cmd,src+srcp,srcc-srcp); break;
      case 0x05: err=stdsyn_pipe_decode_OSC(1,cmd,src+srcp,srcc-srcp); break;
      case 0x06: err=stdsyn_pipe_decode_PFM(0,0,cmd,src+srcp,srcc-srcp); break;
      case 0x07: err=stdsyn_pipe_decode_PFM(1,0,cmd,src+srcp,srcc-srcp); break;
      case 0x08: err=stdsyn_pipe_decode_PFM(0,1,cmd,src+srcp,srcc-srcp); break;
      case 0x09: err=stdsyn_pipe_decode_PFM(1,1,cmd,src+srcp,srcc-srcp); break;
      case 0x0a: err=stdsyn_pipe_decode_BANDPASS(0,cmd,src+srcp,srcc-srcp); break;
      case 0x0b: err=stdsyn_pipe_decode_BANDPASS(1,cmd,src+srcp,srcc-srcp); break;
      case 0x0c: err=stdsyn_pipe_decode_LOPASS(cmd,src+srcp,srcc-srcp); break;
      case 0x0d: err=stdsyn_pipe_decode_HIPASS(cmd,src+srcp,srcc-srcp); break;
      case 0x0e: err=stdsyn_pipe_decode_GAIN(cmd,src+srcp,srcc-srcp); break;
      case 0x0f: err=stdsyn_pipe_decode_DELAY(0,cmd,src+srcp,srcc-srcp); break;
      case 0x10: err=stdsyn_pipe_decode_DELAY(1,cmd,src+srcp,srcc-srcp); break;
      case 0x11: err=stdsyn_pipe_decode_ADD(cmd,src+srcp,srcc-srcp); break;
      case 0x12: err=stdsyn_pipe_decode_MLT(cmd,src+srcp,srcc-srcp); break;
      case 0x13: err=stdsyn_pipe_decode_OSC(-1,cmd,src+srcp,srcc-srcp); break;
      default: {
          fprintf(stderr,"%s: opcode 0x%02x not implemented (%d/%d in encoded pipe)\n",__func__,opcode,srcp-1,srcc);
          return -1;
        }
    }
    
    if (!cmd->type) return -1;
    if (err<0) return -1;
    srcp+=err;
  }
  return 0;
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

static int stdsyn_instrument_decode_MASTER(struct stdsyn_instrument *ins,const uint8_t *src,int srcc) {
  if (srcc<2) return -1;
  uint16_t n=(src[0]<<8)|src[1];
  ins->master=n/4096.0f;
  return 2;
}

/* "prefix", "voice", "suffix"
 */
 
static int stdsyn_instrument_decode_pipe(
  struct stdsyn_pipe_config **dst,
  struct stdsyn_instrument *ins,
  const uint8_t *src,int srcc,
  const char *name
) {
  if (*dst) return -1;
  if (srcc<1) return -1;
  int len=src[0];
  if (1+len>srcc) return -1;
  if (!(*dst=calloc(1,sizeof(struct stdsyn_pipe_config)))) return -1;
  if (stdsyn_pipe_config_decode(*dst,src+1,len)<0) return -1;
  return 1+len;
}

/* Decode stdsyn format.
 */
 
static int stdsyn_instrument_decode_stdsyn(struct stdsyn_instrument *ins,const uint8_t *src,int srcc) {

  ins->type=&stdsyn_node_type_ctlv;
  
  int srcp=0;
  while (srcp<srcc) {
    int err=0;
    switch (src[srcp++]) {
      case 0x01: err=stdsyn_instrument_decode_FM_A_S(ins,src+srcp,srcc-srcp); break;
      case 0x02: err=stdsyn_instrument_decode_FM_R_S(ins,src+srcp,srcc-srcp); break;
      case 0x03: err=stdsyn_instrument_decode_FM_A_E(ins,src+srcp,srcc-srcp); break;
      case 0x04: err=stdsyn_instrument_decode_FM_R_E(ins,src+srcp,srcc-srcp); break;
      case 0x05: err=stdsyn_instrument_decode_STDENV(ins,src+srcp,srcc-srcp); break;
      case 0x06: err=stdsyn_instrument_decode_MASTER(ins,src+srcp,srcc-srcp); break;
      case 0x07: err=stdsyn_instrument_decode_pipe(&ins->prefix,ins,src+srcp,srcc-srcp,"prefix"); break;
      case 0x08: err=stdsyn_instrument_decode_pipe(&ins->voice,ins,src+srcp,srcc-srcp,"voice"); break;
      case 0x09: err=stdsyn_instrument_decode_pipe(&ins->suffix,ins,src+srcp,srcc-srcp,"suffix"); break;
      case 0xc0: break;
      default: {
          fprintf(stderr,"stdsyn: Unknown instrument opcode 0x%02x\n",src[srcp-1]);
          return -1;
        }
    }
    if ((err<0)||(err>srcc-srcp)) return -1;
    srcp+=err;
  }
  
  // Select a type based on features we found.
  if (ins->prefix||ins->voice||ins->suffix) {
    ins->type=&stdsyn_node_type_ctl3;
  } else if (ins->fmrate>0.0f) {
    ins->type=&stdsyn_node_type_ctlv;
  } else {
    // Let it default to ctlv; that can operate unconfigured.
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
  
  instrument->master=1.0f;
  
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

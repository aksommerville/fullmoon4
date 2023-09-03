#include "mkd_internal.h"
#include "mkd_instrument_format.h"

/* Instrument context.
 */
 
struct mkd_stdsyn_instrument {
  const char *path;
  int lineno;
  int id;
  
  /* All minsyn features, pre-encoded when we read them, in the order of their feature bit (little-endian):
   */
  uint8_t *wave; // 1..256
  uint8_t *loenv; // 5
  uint8_t *hienv; // 5
  uint8_t *mixwave; // 1..256
  uint8_t *lomixenv; // 5
  uint8_t *himixenv; // 5
  
  /* stdsyn features.
   */
  uint8_t fm[32]; int fmc; // opcode(1) + rate(2) + range(2..29) = 5..32. If present, preencoded with opcode and all.
  uint8_t stdenv[30]; int stdenvc; // opcode(1) + env(5..29) = 6..30.
  uint8_t master[3]; int masterc; // opcode(2) + u4.12 = 3.
  uint8_t prefix[258]; // opcode(1) + length(1) + pipe
  uint8_t voice[258];
  uint8_t suffix[258];
  int pipemode; // 0=none, 1=prefix, 2=voice, 3=suffix
};

#define INS ((struct mkd_stdsyn_instrument*)instrument)

static void _instrument_del(void *instrument) {
  if (INS->wave) free(INS->wave);
  if (INS->loenv) free(INS->loenv);
  if (INS->hienv) free(INS->hienv);
  if (INS->mixwave) free(INS->mixwave);
  if (INS->lomixenv) free(INS->lomixenv);
  if (INS->himixenv) free(INS->himixenv);
  free(instrument);
}

static void *_instrument_new(int id,const char *path,int lineno) {
  void *instrument=calloc(1,sizeof(struct mkd_stdsyn_instrument));
  if (!instrument) return 0;
  INS->path=path;
  INS->lineno=lineno;
  INS->id=id;
  return instrument;
}

/* Text helpers.
 */
 
static int stdsyn_is_float_token(const char *src,int srcc) {
  for (;srcc-->0;src++) if (*src=='.') return 1;
  return 0;
}

static int stdsyn_env_time_eval(const char *src,int srcc) {
  int ms;
  if (sr_int_eval(&ms,src,srcc)<2) return -1;
  if (ms<0) return -1;
  if (ms>0xffff) return -1;
  return ms;
}

/* "wave" and "mixwave".
 */
 
static int _instrument_wave(uint8_t **dst,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  if (*dst) {
    fprintf(stderr,"%s:%d: Duplicate wave\n",path,lineno);
    return -2;
  }
  uint8_t coefv[255];
  uint8_t coefc=0;
  int srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    int v;
    if ((sr_int_eval(&v,token,tokenc)<2)||(v<0)||(v>0xff)) {
      fprintf(stderr,"%s:%d: Expected integer in 0..255, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
    if (coefc>=255) {
      fprintf(stderr,"%s:%d: Too many wave coefficients, limit 255\n",path,lineno);
      return -2;
    }
    coefv[coefc++]=v;
  }
  if (!(*dst=malloc(1+coefc))) return -1;
  (*dst)[0]=coefc;
  memcpy((*dst)+1,coefv,coefc);
  return 0;
}

/* "env" and "mixenv".
 */
 
static int _instrument_env_1(uint8_t *dst,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  int srcp=0,dstc=0;
  for (;dstc<5;dstc++) {
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    if (srcp>=srcc) {
      fprintf(stderr,"%s:%d: Unexpected end of line reading envelope. Expected 5 integers.\n",path,lineno);
      return -2;
    }
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    int v;
    int hilimit=(dstc==4)?0x7f8:0xff;
    if ((sr_int_eval(&v,token,tokenc)<2)||(v<0)||(v>hilimit)) {
      fprintf(stderr,"%s:%d: Expected integer in 0..%d, found '%.*s'\n",path,lineno,hilimit,tokenc,token);
      return -2;
    }
    if (dstc==4) v>>=3;
    dst[dstc]=v;
  }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  return srcp;
}
 
static int _instrument_env(uint8_t **dstlo,uint8_t **dsthi,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  int err,srcp=0;
  if (*dstlo||*dsthi) {
    fprintf(stderr,"%s:%d: Duplicate envelope\n",path,lineno);
    return -2;
  }
  if (!(*dstlo=malloc(5))) return -1;
  if ((err=_instrument_env_1(*dstlo,instrument,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
  if (srcp>=srcc) return 0;
  if ((srcp>srcc-2)||(src[srcp]!='.')||(src[srcp+1]!='.')) {
    fprintf(stderr,"%s:%d: Expected '..' between low and high envelopes.\n",path,lineno);
    return -2;
  }
  srcp+=2;
  if (!(*dsthi=malloc(5))) return -1;
  if ((err=_instrument_env_1(*dsthi,instrument,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
  if (srcp<srcc) {
    fprintf(stderr,"%s:%d: Unexpected tokens after envelope.\n",path,lineno);
    return -2;
  }
  return 0;
}

/* Long-form stdsyn envelopes.
 * Input may have leading and trailing whitespace but otherwise must be the full parenthesized envelope and nothing more.
 * Output must have room for at least 29 bytes. Returns actual output length.
 */
 
static int stdsyn_env_compile(uint8_t *dst,const char *src,int srcc,const char *path,int lineno) {
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((srcp>=srcc)||(src[srcp++]!='(')) return -1;
  
  /* Pop tokens into a "low" and "high" list until we meet the closing paren.
   */
  struct token { const char *v; int c; };
  struct token lotokv[7],hitokv[7];
  int lotokc=0,hitokc=-1; // (hitokc>=0) after we consume the separator.
  while (1) {
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    if (srcp>=srcc) break;
    if (src[srcp]==')') break;
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    if ((tokenc==2)&&!memcmp(token,"..",2)) {
      if (hitokc>=0) {
        fprintf(stderr,"%s:%d: Extra '..' separator in envelope.\n",path,lineno);
        return -2;
      }
      hitokc=0;
      continue;
    }
    if (hitokc<0) {
      if (lotokc>=7) {
        fprintf(stderr,"%s:%d: Expected '..' or ')' before '%.*s'\n",path,lineno,tokenc,token);
        return -2;
      }
      lotokv[lotokc].v=token;
      lotokv[lotokc].c=tokenc;
      lotokc++;
    } else {
      if (hitokc>=7) {
        fprintf(stderr,"%s:%d: Expected ')' before '%.*s'\n",path,lineno,tokenc,token);
        return -2;
      }
      hitokv[hitokc].v=token;
      hitokv[hitokc].c=tokenc;
      hitokc++;
    }
  }
  if ((srcp>=srcc)||(src[srcp++]!=')')) {
    fprintf(stderr,"%s:%d: Unclosed envelope.\n",path,lineno);
    return -2;
  }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp<srcc) {
    fprintf(stderr,"%s:%d: Unexpected tokens after envelope: %.*s\n",path,lineno,srcc-srcp,src+srcp);
    return -2;
  }
  
  /* Validate token counts.
   */
  if ((lotokc<5)||((hitokc>=0)&&(hitokc!=lotokc))) {
    fprintf(stderr,"%s:%d: Invalid token counts in envelope. Must be 5..7 per edge, and edges must match. (%d,%d)\n",path,lineno,lotokc,hitokc);
    return -2;
  }

  /* Determine features by examining token lists.
   */
  int velocity=(hitokc>=0);
  int initial=stdsyn_is_float_token(lotokv[0].v,lotokv[0].c);
  int final=stdsyn_is_float_token(lotokv[lotokc-1].v,lotokv[lotokc-1].c);
  if (initial&&final) {
    if (lotokc!=7) { fprintf(stderr,"%s:%d: Envelope with both initial and final must have 7 tokens.\n",path,lineno); return -2; }
  } else if (initial||final) {
    if (lotokc!=6) { fprintf(stderr,"%s:%d: Envelope with one initial or final must have 6 tokens.\n",path,lineno); return -2; }
  } else {
    if (lotokc!=5) { fprintf(stderr,"%s:%d: Envelope without initial or final must have 5 tokens.\n",path,lineno); return -2; }
  }
  int sustainp=initial?4:3;
  int sustain=0;
  if (lotokv[sustainp].v[lotokv[sustainp].c-1]=='*') { sustain=1; lotokv[sustainp].c--; }
  if (velocity) {
    if (hitokv[sustainp].v[hitokv[sustainp].c-1]=='*') { sustain=1; hitokv[sustainp].c--; }
  }
  // hires_time, hires_level, signed_level will be determined during evaluation.
  
  /* Evaluate times and decide whether to go hi-res.
   * There are always 3 times per edge.
   */
  int hires_time=0;
  int timev[6];
  #define GETTIME(dstp,tokset,index) { \
    const struct token *tok=tokset+initial+(index<<1); \
    if ((timev[dstp]=stdsyn_env_time_eval(tok->v,tok->c))<0) { \
      fprintf(stderr,"%s:%d: Expected time in ms (0..65535), found '%.*s'\n",path,lineno,tok->c,tok->v); \
      return -2; \
    } \
    if (timev[dstp]>1020) hires_time=1; \
  }
  GETTIME(0,lotokv,0)
  GETTIME(1,lotokv,1)
  GETTIME(2,lotokv,2)
  if (velocity) {
    GETTIME(3,hitokv,0)
    GETTIME(4,hitokv,1)
    GETTIME(5,hitokv,2)
  }
  #undef GETTIME
  
  /* Evaluate levels and determine hi-res and signed.
   * We'll fill this list as if initial and final are present, to keep indices constant.
   */
  int hires_level=0,signed_level=0;
  float levelv[8];
  #define GETLEVEL(dstp,token) { \
    int frc=0,i=0,fr=0; \
    for (;i<token.c;i++) { \
      if (fr) frc++; \
      else if (token.v[i]=='.') fr=1; \
    } \
    if (frc>3) hires_level=1; \
    if (token.v[0]=='-') signed_level=1; \
    if ((sr_float_eval(levelv+dstp,token.v,token.c)<0)||(levelv[dstp]<-1.0f)||(levelv[dstp]>1.0f)) { \
      fprintf(stderr,"%s:%d: Expected floating-point level in -1..1, found '%.*s'\n",path,lineno,token.c,token.v); \
      return -2; \
    } \
  }
  if (initial) {
    GETLEVEL(0,lotokv[0])
    GETLEVEL(1,lotokv[2])
    GETLEVEL(2,lotokv[4])
    if (final) GETLEVEL(3,lotokv[6])
  } else {
    GETLEVEL(1,lotokv[1])
    GETLEVEL(2,lotokv[3])
    if (final) GETLEVEL(3,lotokv[5])
  }
  if (velocity) {
    if (initial) {
      GETLEVEL(4,hitokv[0])
      GETLEVEL(5,hitokv[2])
      GETLEVEL(6,hitokv[4])
      if (final) GETLEVEL(7,hitokv[6])
    } else {
      GETLEVEL(5,hitokv[1])
      GETLEVEL(6,hitokv[3])
      if (final) GETLEVEL(7,hitokv[5])
    }
  }
  #undef GETLEVEL

  /* Compose output.
   */
  int dstc=0;
  dst[dstc++]=
    (velocity?    0x01:0)|
    (sustain?     0x02:0)|
    (initial?     0x04:0)|
    (hires_time?  0x08:0)|
    (hires_level? 0x10:0)|
    (signed_level?0x20:0)|
    (final?       0x40:0)|
  0;
  #define APPEND_LEVEL(v) { \
    if (hires_level) { \
      int n=(v)*65535.0f; \
      dst[dstc++]=n>>8; \
      dst[dstc++]=n; \
    } else { \
      dst[dstc++]=(v)*255.0f; \
    } \
  }
  #define APPEND_TIME(v) { \
    if (hires_time) { \
      dst[dstc++]=(v)>>8; \
      dst[dstc++]=(v); \
    } else { \
      dst[dstc++]=(v)>>2; \
    } \
  }
  if (initial) APPEND_LEVEL(levelv[0])
  APPEND_TIME(timev[0])
  APPEND_LEVEL(levelv[1])
  APPEND_TIME(timev[1])
  APPEND_LEVEL(levelv[2])
  APPEND_TIME(timev[2])
  if (final) APPEND_LEVEL(levelv[3])
  if (velocity) {
    if (initial) APPEND_LEVEL(levelv[4])
    APPEND_TIME(timev[3])
    APPEND_LEVEL(levelv[5])
    APPEND_TIME(timev[4])
    APPEND_LEVEL(levelv[6])
    APPEND_TIME(timev[5])
    if (final) APPEND_LEVEL(levelv[7])
  }
  #undef APPEND_LEVEL
  #undef APPEND_TIME
  
  return dstc;
}

/* "fm"
 */
 
static int _instrument_fm(void *instrument,const char *src,int srcc,const char *path,int lineno) {
  if (INS->fmc) {
    fprintf(stderr,"%s:%d: Duplicate 'fm' command.\n",path,lineno);
    return -2;
  }
  int srcp=0,err;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  // rate
  if (srcp>=srcc) return -1;
  float rate_denominator;
  if (src[srcp]=='*') { // relative rate, the typical case. u8.8
    srcp++;
    INS->fm[INS->fmc++]=0x02; // FM_R_S (assuming "S" for now)...
    rate_denominator=256.0f;
  } else { // absolute rate. u12.4
    INS->fm[INS->fmc++]=0x01; // FM_A_S (assuming "S" for now)...
    rate_denominator=16.0f;
  }
  const char *token=src+srcp;
  int tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  float ratef;
  if (sr_float_eval(&ratef,token,tokenc)<0) {
    fprintf(stderr,"%s:%d: Failed to evaluate '%.*s' as float for fm rate.\n",path,lineno,tokenc,token);
    return -2;
  }
  uint16_t ratei=ratef*rate_denominator;
  INS->fm[INS->fmc++]=ratei>>8;
  INS->fm[INS->fmc++]=ratei;
  
  // Range or range limit.
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  double range;
  if (sr_double_eval(&range,token,tokenc)<0) {
    fprintf(stderr,"%s:%d: Expected scalar 'range', found '%.*s'\n",path,lineno,tokenc,token);
    return -2;
  }
  int rangen=range*256.0f;
  INS->fm[INS->fmc++]=rangen>>8;
  INS->fm[INS->fmc++]=rangen;
  
  // Optional range envelope.
  if ((srcp<srcc)&&(src[srcp]=='(')) {
    INS->fm[0]+=2; // "S"=>"E" for both Relative and Absolute.
    if ((err=stdsyn_env_compile(INS->fm+INS->fmc,src+srcp,srcc-srcp,path,lineno))<0) return err;
    INS->fmc+=err;
    srcp=srcc;
  }
  
  if (srcp<srcc) {
    fprintf(stderr,"%s:%d: Unexpected tokens after fm command: %.*s\n",path,lineno,srcc-srcp,src+srcp);
    return -2;
  }
  return 0;
}

/* "stdenv"
 */
 
static int _instrument_stdenv(void *instrument,const char *src,int srcc,const char *path,int lineno) {
  if (INS->stdenvc) {
    fprintf(stderr,"%s:%d: Duplicate 'stdenv' command\n",path,lineno);
    return -2;
  }
  INS->stdenv[INS->stdenvc++]=0x05;
  int err=stdsyn_env_compile(INS->stdenv+INS->stdenvc,src,srcc,path,lineno);
  if (err<0) return err;
  INS->stdenvc+=err;
  return 0;
}

/* "master"
 */
 
static int _instrument_master(void *instrument,const char *src,int srcc,const char *path,int lineno) {
  if (INS->masterc) {
    fprintf(stderr,"%s:%d: Duplicate 'master' command\n",path,lineno);
    return -2;
  }
  float v;
  if ((sr_float_eval(&v,src,srcc)<0)||(v<0.0f)||(v>16.0f)) {
    fprintf(stderr,"%s:%d: Expected float in 0..16, found '%.*s'\n",path,lineno,srcc,src);
    return -2;
  }
  int n=v*4096.0f;
  if (n>0xffff) n=0xffff;
  INS->master[0]=0x06;
  INS->master[1]=n>>8;
  INS->master[2]=n;
  INS->masterc=3;
  return 0;
}

/* "prefix", "voice", "suffix"
 */
 
static int _instrument_pipemode(void *instrument,const char *src,int srcc,const char *path,int lineno,int modeid,uint8_t *storage) {
  if (srcc) {
    fprintf(stderr,"%s:%d: Unexpected tokens after pipe mode command: '%.*s'\n",path,lineno,srcc,src);
    return -2;
  }
  if (storage[0]) {
    fprintf(stderr,"%s:%d: Duplicate pipe mode command.\n",path,lineno);
    return -2;
  }
  storage[0]=6+modeid; // our modeid line up with opcodes, kind of
  storage[1]=0;
  INS->pipemode=modeid;
  return 0;
}

/* Pipe: "copy"
 * | copy  | `DST SRC`
 * 0x02 COPY (u8 src)
 */
 
static int _instrument_pipecmd_COPY(uint8_t *dst,int dsta,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  int bufid;
  if ((sr_int_eval(&bufid,src,srcc)<2)||(bufid<0)||(bufid>3)) {
    fprintf(stderr,"%s:%d: Expected buffer id in 0..3, found '%.*s'\n",path,lineno,srcc,src);
    return -2;
  }
  if (dsta>=1) {
    dst[0]=bufid;
  }
  return 1;
}

/* Pipe: "penv"
 * | penv     | `BUFID ENV`
 * 0x03 PENV (...env)
 */
 
static int _instrument_pipecmd_PENV(uint8_t *dst,int dsta,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  if (dsta<29) return 29; // stdsyn_env_compile() doesn't take a buffer length, we must assume the max.
  return stdsyn_env_compile(dst,src,srcc,path,lineno);
}

/* Pipe: "osc"
 * | osc      | `BUFID RATE SHAPE`. RATE may be `*N` to multiply against the note's rate, in a voice.
 * 0x04 OSC_A (u16 rate,u8 coefc,...coefv)
 * 0x05 OSC_R (u8.8 rate,u8 coefc,...coefv)
 */
 
static int _instrument_pipecmd_OSC(uint8_t *dst,int dsta,void *instrument,const char *src,int srcc,const char *path,int lineno) {

  // RATE, and switch to the relative command as needed.
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *token=src+srcp;
  int tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  int absolute=1,rate;
  if (tokenc&&(token[0]=='*')) {
    dst[-1]=0x05; // OSC_R
    absolute=0;
    token++;
    tokenc--;
    double ratef;
    if ((sr_double_eval(&ratef,token,tokenc)<0)||(ratef<0.0f)||(ratef>256.0f)) {
      fprintf(stderr,"%s:%d: Expected floating-point relative rate in 0..256, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
    rate=ratef*256.0f;
    if (rate>0xffff) rate=0xffff;
  } else {
    if ((sr_int_eval(&rate,token,tokenc)<2)||(rate<0)||(rate>0xffff)) {
      fprintf(stderr,"%s:%d: Expected integer rate in 0..65535, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
  }
  int dstc=0;
  if (dstc<=dsta-2) {
    dst[dstc++]=rate>>8;
    dst[dstc++]=rate;
  } else dstc+=2;
  
  // Coefficients.
  int coefp=dstc++;
  int coefc=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    token=src+srcp;
    tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    int v;
    if ((sr_int_eval(&v,token,tokenc)<2)||(v<0)||(v>0xff)) {
      fprintf(stderr,"%s:%d: Expected coefficient in 0..255, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
    if (dstc<dsta) dst[dstc]=v;
    dstc++;
    coefc++;
  }
  if (coefc>0xff) {
    fprintf(stderr,"%s:%d: Too many coefficients (%d, limit 255)\n",path,lineno,coefc);
    return -2;
  }
  if (coefp<dsta) dst[coefp]=coefc;
  
  return dstc;
}

/* Pipe: "pfm"
 * | pfm      | `BUFID RATE MODRATE RANGE [ENV]`
 * 0x06 PFM_A_A (u16 rate,u12.4 modrate,u8.8 range,...env)
 * 0x07 PFM_R_A (u8.8 rate_mlt,u12.4 modrate,u8.8 range,...env)
 * 0x08 PFM_A_R (u16 rate,u8.8 modrate_mlt,u8.8 range,...env)
 * 0x09 PFM_R_R (u8.8 rate_mlt,u8.8 modrate_mlt,u8.8 range,...env)
 */
 
static int _instrument_pipecmd_PFM(uint8_t *dst,int dsta,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  int srcp=0,tokenc,err,dstc=0;
  const char *token;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  // rate
  int ratei;
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (tokenc&&(token[0]=='*')) {
    dst[-1]++; // PFM_A_A=>PFM_R_A
    token++;
    tokenc--;
    double ratef;
    if ((sr_double_eval(&ratef,token,tokenc)<0)||(ratef<0.0)||(ratef>256.0)) {
      fprintf(stderr,"%s:%d: Expected carrier rate multiplier in 0..256, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
    ratei=ratef*256.0f;
    if (ratei>0xffff) ratei=0xffff;
  } else {
    if ((sr_int_eval(&ratei,token,tokenc)<2)||(ratei<0)||(ratei>0xffff)) {
      fprintf(stderr,"%s:%d: Expected integer absolute rate in 0..65535, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
  }
  if (ratei>0xffff) ratei=0xffff;
  if (dstc<=dsta-2) {
    dst[dstc++]=ratei>>8;
    dst[dstc++]=ratei;
  } else dstc+=2;
  
  // modrate
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (tokenc&&(token[0]=='*')) {
    dst[-1]+=2; // PFM_A_A=>PFM_A_R or PFM_R_A=>PFM_R_R
    token++;
    tokenc--;
    double f;
    if ((sr_double_eval(&f,token,tokenc)<0)||(f<0.0)||(f>256.0)) {
      fprintf(stderr,"%s:%d: Expected float in 0..256 for modrate, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
    ratei=f*256.0f;
  } else {
    double f;
    if ((sr_double_eval(&f,token,tokenc)<0)||(f<0.0)||(f>4096.0)) {
      fprintf(stderr,"%s:%d: Expected float in 0..4096 for modrate, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
    ratei=f*16.0f;
  }
  if (ratei>0xffff) ratei=0xffff;
  if (dstc<=dsta-2) {
    dst[dstc++]=ratei>>8;
    dst[dstc++]=ratei;
  } else dstc+=2;
  
  // env. If absent in source, make up a constant one.
  if (dstc>dsta-29) err=29;
  else if (srcp<srcc) err=stdsyn_env_compile(dst+dstc,src+srcp,srcc-srcp,path,lineno);
  else err=stdsyn_env_compile(dst+dstc,"( 1.0 1 1.0 1 1.0 1 1.0 )",25,path,lineno);
  if (err<0) return err;
  dstc+=err;
  
  return dstc;
}

/* Pipe: "bandpass"
 * | bandpass | `BUFID RATE WIDTH`. RATE and WIDTH in Hz, or `*N` for RATE.
 * 0x0a BANDPASS_A (u16 rate,u8.8 width)
 * 0x0b BANDPASS_R (u8.8 rate,u8.8 width)
 */
 
static int _instrument_pipecmd_BANDPASS(uint8_t *dst,int dsta,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  if (dsta<4) return 4;
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *token=src+srcp;
  int tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  int ratei;
  if (tokenc&&(token[0]=='*')) {
    dst[-1]++; // BANDPASS_A=>BANDPASS_R
    token++;
    tokenc--;
    double f;
    if ((sr_double_eval(&f,token,tokenc)<0)||(f<0.0)||(f>256.0)) {
      fprintf(stderr,"%s:%d: Expected floating-point rate multiplier in 0..256, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
    ratei=f*256.0;
    if (ratei>0xffff) ratei=0xffff;
  } else {
    if ((sr_int_eval(&ratei,token,tokenc)<2)||(ratei<0)||(ratei>0xffff)) {
      fprintf(stderr,"%s:%d: Expected integer rate in 0..65535, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
  }
  dst[0]=ratei>>8;
  dst[1]=ratei;
  
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  double f;
  if ((sr_double_eval(&f,token,tokenc)<0)||(f<0.0f)||(f>256.0f)) {
    fprintf(stderr,"%s:%d: Expected floating-point bandwidth in 0..256 Hz, found '%.*s'\n",path,lineno,tokenc,token);
    return -2;
  }
  ratei=f*256.0f;
  if (ratei>0xffff) ratei=0xffff;
  dst[2]=ratei>>8;
  dst[3]=ratei;
  
  return 4;
}

/* Pipe: "lopass" or "hipass"
 * | lopass   | `BUFID RATE`. RATE in Hz only.
 * | hipass   | `BUFID RATE`. RATE in Hz only.
 * 0x0c LOPASS (u16 rate)
 * 0x0d HIPASS (u16 rate)
 */
 
static int _instrument_pipecmd_LOHIPASS(uint8_t *dst,int dsta,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  if (dsta<2) return 2;
  int v;
  if ((sr_int_eval(&v,src,srcc)<2)||(v<0)||(v>0xffff)) {
    fprintf(stderr,"%s:%d: Expected integer rate in 0..65535 Hz, found '%.*s'\n",path,lineno,srcc,src);
    return -2;
  }
  dst[0]=v>>8;
  dst[1]=v;
  return 2;
}

/* Pipe: "gain"
 * | gain     | `BUFID GAIN CLIP GATE`
 * 0x0e GAIN (u8.8 gain,u0.8 clip,u0.8 gate)
 */
 
static int _instrument_pipecmd_GAIN(uint8_t *dst,int dsta,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  if (dsta<4) return 4;
  int srcp=0,tokenc,vi;
  double vf;
  const char *token;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((sr_double_eval(&vf,token,tokenc)<1)||(vf<0.0)||(vf>256.0)) {
    fprintf(stderr,"%s:%d: Expected floating-point gain in 0..256, found '%.*s'\n",path,lineno,tokenc,token);
    return -2;
  }
  if ((vi=vf*256.0)>0xffff) vi=0xffff;
  dst[0]=vi>>8;
  dst[1]=vi;
  
  if (srcp>=srcc) {
    dst[2]=0xff; // clip defaults to 1
  } else {
    token=src+srcp;
    tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    if ((sr_double_eval(&vf,token,tokenc)<1)||(vf<0.0)||(vf>256.0)) {
      fprintf(stderr,"%s:%d: Expected floating-point clip in 0..1, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
    if ((vi=vf*256.0)>0xff) vi=0xff;
    dst[2]=vi;
  }
  
  if (srcp>=srcc) {
    dst[3]=0x00; // gate defaults to 0
  } else {
    token=src+srcp;
    tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    if ((sr_double_eval(&vf,token,tokenc)<1)||(vf<0.0)||(vf>256.0)) {
      fprintf(stderr,"%s:%d: Expected floating-point gate in 0..1, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
    if ((vi=vf*256.0)>0xff) vi=0xff;
    dst[3]=vi;
  }
  
  if (srcp<srcc) {
    fprintf(stderr,"%s:%d: Unexpected tokens after gain command: '%.*s'\n",path,lineno,srcc-srcp,src+srcp);
    return -2;
  }
  
  return 4;
}

/* Pipe: "delay"
 * | delay    | `BUFID INTERVAL DRY WET STORE FEEDBACK`. INTERVAL in ms or `*N` to multiply against one qnote's length.
 * 0x0f DELAY_A (u16 ms,u0.8 dry,u0.8 wet,u0.8 store,u0.8 feedback)
 * 0x10 DELAY_R (u8.8 qnotes,u0.8 dry,u0.8 web,u0.8 store,u0.8 feedback)
 */
 
static int _instrument_pipecmd_DELAY(uint8_t *dst,int dsta,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  if (dsta<6) return 6;
  int srcp=0,tokenc,vi;
  double vf;
  const char *token;
  
  // Interval, may change opcode.
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (tokenc&&(token[0]=='*')) {
    dst[-1]++; // DELAY_A=>DELAY_R
    token++;
    tokenc--;
    if ((sr_double_eval(&vf,token,tokenc)<0)||(vf<0.0)||(vf>256.0)) {
      fprintf(stderr,"%s:%d: Expected tempo multiplier in 0..256 (unit=qnote), found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
    if ((vi=vf*256.0)>0xffff) vi=0xffff;
  } else {
    if ((sr_int_eval(&vi,token,tokenc)<0)||(vi<0)||(vi>0xffff)) {
      fprintf(stderr,"%s:%d: Expected interval in 0..65535 ms, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
  }
  dst[0]=vi>>8;
  dst[1]=vi;
  
  // Followed by four identical u0.8s: dry, wet, store, feedback.
  const char *fieldnames[4]={"dry","wet","store","feedback"};
  int i=0; for (;i<4;i++) {
    token=src+srcp;
    tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    if ((sr_double_eval(&vf,token,tokenc)<0)||(vf<0.0)||(vf>1.0)) {
      fprintf(stderr,"%s:%d: Expected %s level in 0..1, found '%.*s'\n",path,lineno,fieldnames[i],tokenc,token);
      return -2;
    }
    if ((vi=vf*256.0)>0xff) vi=0xff;
    dst[2+i]=vi;
  }
  
  if (srcp<srcc) {
    fprintf(stderr,"%s:%d: Unexpected tokens after 'delay' command: '%.*s'\n",path,lineno,srcc-srcp,src+srcp);
    return -2;
  }
  
  return 6;
}

/* Pipe: "add"
 * | add  | `DST SRC`
 * 0x11 ADD (u8 src)
 */
 
static int _instrument_pipecmd_ADD(uint8_t *dst,int dsta,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  int bufid;
  if ((sr_int_eval(&bufid,src,srcc)<2)||(bufid<0)||(bufid>3)) {
    fprintf(stderr,"%s:%d: Expected buffer id in 0..3, found '%.*s'\n",path,lineno,srcc,src);
    return -2;
  }
  if (dsta>=1) {
    dst[0]=bufid;
  }
  return 1;
}

/* Pipe: "mlt"
 * | mlt  | `BUFID MULTIPLIER`
 * 0x12 MLT (u8.8 multiplier)
 */
 
static int _instrument_pipecmd_MLT(uint8_t *dst,int dsta,void *instrument,const char *src,int srcc,const char *path,int lineno) {
  double vf;
  if ((sr_double_eval(&vf,src,srcc)<0)||(vf<0.0)||(vf>256.0)) {
    fprintf(stderr,"%s:%d: Expected multiplier in 0..256, found '%.*s'\n",path,lineno,srcc,src);
    return -2;
  }
  if (dsta>=2) {
    int vi=vf*256.0;
    if (vi>0xffff) vi=0xffff;
    dst[0]=vi>>8;
    dst[1]=vi;
  }
  return 2;
}

/* Any command except pipe modes, with pipe mode set.
 */
 
// Opcodes must be in 0..63, so we use 255 for errors.
static uint8_t _instrument_pipe_opcode_eval(const char *src,int srcc) {
  if ((srcc==7)&&!memcmp(src,"silence",7)) return 0x00;
  if ((srcc==5)&&!memcmp(src,"noise",5)) return 0x01;
  if ((srcc==4)&&!memcmp(src,"copy",4)) return 0x02;
  if ((srcc==4)&&!memcmp(src,"penv",4)) return 0x03;
  if ((srcc==3)&&!memcmp(src,"osc",3)) return 0x04; // OSC_A; handler may change to 0x05 OSC_R
  if ((srcc==3)&&!memcmp(src,"pfm",3)) return 0x06; // PFM_A_A; handler may change to 0x07..0x09
  if ((srcc==8)&&!memcmp(src,"bandpass",8)) return 0x0a; // BANDPASS_A; handler may change to 0x0b BANDPASS_R
  if ((srcc==6)&&!memcmp(src,"lopass",6)) return 0x0c;
  if ((srcc==6)&&!memcmp(src,"hipass",6)) return 0x0d;
  if ((srcc==4)&&!memcmp(src,"gain",4)) return 0x0e;
  if ((srcc==5)&&!memcmp(src,"delay",5)) return 0x0f; // DELAY_A; handler may change to 0x10 DELAY_R
  if ((srcc==3)&&!memcmp(src,"add",3)) return 0x11;
  if ((srcc==3)&&!memcmp(src,"mlt",3)) return 0x12;
  return 0xff;
}
 
static int _instrument_pipecmd(void *instrument,const char *kw,int kwc,const char *src,int srcc,const char *path,int lineno) {

  // Parse opcode and bufid generically, they are formatted the same for all commands.
  uint8_t opcode=_instrument_pipe_opcode_eval(kw,kwc);
  if (opcode==0xff) {
    fprintf(stderr,"%s:%d: Expected pipe mode or command, found '%.*s'\n",path,lineno,kwc,kw);
    return -2;
  }
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *token=src+srcp;
  int tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  int bufid;
  if ((sr_int_eval(&bufid,token,tokenc)<2)||(bufid<0)||(bufid>3)) {
    fprintf(stderr,"%s:%d: Expected buffer ID 0..3, found '%.*s'\n",path,lineno,tokenc,token);
    return -2;
  }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  // Emit opcode and bufid generically. Validate output buffer, etc.
  uint8_t *dst;
  switch (INS->pipemode) {
    case 1: dst=INS->prefix; break;
    case 2: dst=INS->voice; break;
    case 3: dst=INS->suffix; break;
    default: return -1;
  }
  uint8_t *subdst=dst+2+dst[1];
  int subdsta=256-subdst[1];
  if (subdsta<1) {
    fprintf(stderr,"%s:%d: No more room in pipe.\n",path,lineno);
    return -2;
  }
  subdst[0]=(bufid<<6)|opcode;
  subdst++;
  subdsta--;
  dst[1]++;
  
  // And finally let command handlers process the args and produce payload.
  int err=0;
  switch (opcode) {
  
    case 0x00: break; // SILENCE, no payload
    case 0x01: break; // NOISE, no payload
    case 0x02: err=_instrument_pipecmd_COPY(subdst,subdsta,instrument,src+srcp,srcc-srcp,path,lineno); break;
    case 0x03: err=_instrument_pipecmd_PENV(subdst,subdsta,instrument,src+srcp,srcc-srcp,path,lineno); break;
    case 0x04: err=_instrument_pipecmd_OSC(subdst,subdsta,instrument,src+srcp,srcc-srcp,path,lineno); break;
    case 0x06: err=_instrument_pipecmd_PFM(subdst,subdsta,instrument,src+srcp,srcc-srcp,path,lineno); break;
    case 0x0a: err=_instrument_pipecmd_BANDPASS(subdst,subdsta,instrument,src+srcp,srcc-srcp,path,lineno); break;
    case 0x0c: err=_instrument_pipecmd_LOHIPASS(subdst,subdsta,instrument,src+srcp,srcc-srcp,path,lineno); break;
    case 0x0d: err=_instrument_pipecmd_LOHIPASS(subdst,subdsta,instrument,src+srcp,srcc-srcp,path,lineno); break;
    case 0x0e: err=_instrument_pipecmd_GAIN(subdst,subdsta,instrument,src+srcp,srcc-srcp,path,lineno); break;
    case 0x0f: err=_instrument_pipecmd_DELAY(subdst,subdsta,instrument,src+srcp,srcc-srcp,path,lineno); break;
    case 0x11: err=_instrument_pipecmd_ADD(subdst,subdsta,instrument,src+srcp,srcc-srcp,path,lineno); break;
    case 0x12: err=_instrument_pipecmd_MLT(subdst,subdsta,instrument,src+srcp,srcc-srcp,path,lineno); break;
  
    default: {
        fprintf(stderr,"%s:%d: Pipe opcode 0x%02x ('%.*s') known but not handled. Please implement at %s:%d.\n",path,lineno,opcode,kwc,kw,__FILE__,__LINE__);
        return -2;
      }
  }
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error processing pipe command '%.*s'\n",path,lineno,kwc,kw);
    return -2;
  }
  if (err>subdsta) {
    fprintf(stderr,"%s:%d: No more room in pipe.\n",path,lineno);
    return -2;
  }
  dst[1]+=err;
  
  return 0;
}

/* Instrument input.
 */

static int _instrument_line(void *instrument,const char *kw,int kwc,const char *src,int srcc,const char *path,int lineno) {

  /* Commands allowed in any context. */
    
  if ((kwc==6)&&!memcmp(kw,"prefix",6)) {
    return _instrument_pipemode(instrument,src,srcc,path,lineno,1,INS->prefix);
    
  } else if ((kwc==5)&&!memcmp(kw,"voice",5)) {
    return _instrument_pipemode(instrument,src,srcc,path,lineno,2,INS->voice);
    
  } else if ((kwc==6)&&!memcmp(kw,"suffix",6)) {
    return _instrument_pipemode(instrument,src,srcc,path,lineno,3,INS->suffix);
    
  } else if ((kwc==6)&&!memcmp(kw,"master",6)) {
    return _instrument_master(instrument,src,srcc,path,lineno);
    
  /* Branch off in pipe mode. */
    
  } else if (INS->pipemode) {
    return _instrument_pipecmd(instrument,kw,kwc,src,srcc,path,lineno);
    
  /* Non-pipe commands. */

  } else if ((kwc==4)&&!memcmp(kw,"wave",4)) {
    return _instrument_wave(&INS->wave,instrument,src,srcc,path,lineno);
    
  } else if ((kwc==3)&&!memcmp(kw,"env",3)) {
    return _instrument_env(&INS->loenv,&INS->hienv,instrument,src,srcc,path,lineno);
    
  } else if ((kwc==7)&&!memcmp(kw,"mixwave",7)) {
    return _instrument_wave(&INS->mixwave,instrument,src,srcc,path,lineno);
    
  } else if ((kwc==6)&&!memcmp(kw,"mixenv",6)) {
    return _instrument_env(&INS->lomixenv,&INS->himixenv,instrument,src,srcc,path,lineno);
    
  } else if ((kwc==2)&&!memcmp(kw,"fm",2)) {
    return _instrument_fm(instrument,src,srcc,path,lineno);
    
  } else if ((kwc==6)&&!memcmp(kw,"stdenv",6)) {
    return _instrument_stdenv(instrument,src,srcc,path,lineno);
    
  } else {
    fprintf(stderr,"%s:%d: Unexpected instrument command '%.*s'\n",path,lineno,kwc,kw);
    return -2;
  }
  return 0;
}

/* Test features after decode complete.
 */
 
static int stdsyn_has_stdsyn_features(const void *instrument) {
  if (INS->fmc) return 1;
  if (INS->stdenvc) return 1;
  //if (INS->masterc) return 1; // "master" is ok for minsyn instruments, tho the synth itself doesn't use it.
  if (INS->prefix[1]) return 1;
  if (INS->voice[1]) return 1;
  if (INS->suffix[1]) return 1;
  return 0;
}

static int stdsyn_has_minsyn_features(const void *instrument) {
  if (INS->wave) return 1;
  if (INS->loenv) return 1;
  if (INS->hienv) return 1;
  if (INS->mixwave) return 1;
  if (INS->lomixenv) return 1;
  if (INS->himixenv) return 1;
  return 0;
}

/* Instrument output.
 */

static int _instrument_encode(struct sr_encoder *dst,void *instrument) {

  if (stdsyn_has_stdsyn_features(instrument)) {
    if (stdsyn_has_minsyn_features(instrument)) {
      fprintf(stderr,"%s:%d: Instrument mixes minsyn and stdsyn features. Must use only one or the other set.\n",INS->path,INS->lineno);
      return -2;
    }
    if (sr_encode_raw(dst,"\xc0",1)<0) return -1; // HELLO. Required leading byte for stdsyn.
    if (INS->prefix[1]&&(sr_encode_raw(dst,INS->prefix,2+INS->prefix[1])<0)) return -1;
    if (INS->voice[1]&&(sr_encode_raw(dst,INS->voice,2+INS->voice[1])<0)) return -1;
    if (INS->suffix[1]&&(sr_encode_raw(dst,INS->suffix,2+INS->suffix[1])<0)) return -1;
    #define PREENCODED(tag) if (INS->tag##c&&(sr_encode_raw(dst,INS->tag,INS->tag##c)<0)) return -1;
    PREENCODED(fm)
    PREENCODED(stdenv)
    PREENCODED(master)
    #undef PREENCODED
    return 0;
  }
  // No need to check stdsyn_has_minsyn_features(). Empty is a valid minsyn instrument. ...i think?

  uint8_t flags=0;
  if (INS->wave) flags|=0x01;
  if (INS->loenv) flags|=0x02;
  if (INS->hienv) flags|=0x04;
  if (INS->mixwave) flags|=0x08;
  if (INS->lomixenv) flags|=0x10;
  if (INS->himixenv) flags|=0x20;
  if (sr_encode_u8(dst,flags)<0) return -1;
  
  if (INS->wave&&(sr_encode_raw(dst,INS->wave,1+INS->wave[0])<0)) return -1;
  if (INS->loenv&&(sr_encode_raw(dst,INS->loenv,5)<0)) return -1;
  if (INS->hienv&&(sr_encode_raw(dst,INS->hienv,5)<0)) return -1;
  if (INS->mixwave&&(sr_encode_raw(dst,INS->mixwave,1+INS->mixwave[0])<0)) return -1;
  if (INS->lomixenv&&(sr_encode_raw(dst,INS->lomixenv,5)<0)) return -1;
  if (INS->himixenv&&(sr_encode_raw(dst,INS->himixenv,5)<0)) return -1;
  
  return 0;
}

/* Sound context.
 */
 
struct mkd_stdsyn_sound {
  const char *path;
  int lineno;
  int id;
  int lenms;
  struct sr_encoder dst;
};

#define SND ((struct mkd_stdsyn_sound*)sound)

static void _sound_del(void *sound) {
  sr_encoder_cleanup(&SND->dst);
  free(sound);
}

static void *_sound_new(int id,const char *path,int lineno) {
  void *sound=calloc(1,sizeof(struct mkd_stdsyn_sound));
  if (!sound) return 0;
  SND->path=path;
  SND->lineno=lineno;
  SND->id=id;
  return sound;
}

/* Sound bits.
 */
 
static int _sound_env(void *sound,const char *src,int srcc,const char *path,int lineno) {
  // ENV are integers: `LEVEL [MS LEVEL...]`, one time may be replaced by `*` to fill in the remainder.
  // env: u16 level,u8 count,count*(u16 ms,u16 level)
  int srcp=0,ptc=0,ptcp=-1,starp=-1,totalms=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((srcp>=srcc)||(src[srcp++]!='(')) {
    fprintf(stderr,"%s:%d: Expected parenthesized envelope.\n",path,lineno);
    return -2;
  }
  while (1) {
    if (srcp>=srcc) {
      fprintf(stderr,"%s:%d: Unexpected end of line reading envelope.\n",path,lineno);
      return -2;
    }
    if (src[srcp]==')') {
      srcp++;
      break;
    }
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    
    if (ptcp>=0) { // After the initial LEVEL, we expect TIME then LEVEL.
      const char *token=src+srcp;
      int tokenc=0;
      while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
      if ((tokenc==1)&&(token[0]=='*')) {
        if (starp>=0) {
          fprintf(stderr,"%s:%d: Multiple wildcard times in envelope.\n",path,lineno);
          return -2;
        }
        starp=SND->dst.c;
        if (sr_encode_raw(&SND->dst,"\0\0",2)<0) return -1;
      } else {
        int ms;
        if ((sr_int_eval(&ms,token,tokenc)<2)||(ms<0)||(ms>0xffff)) {
          fprintf(stderr,"%s:%d: Expected time (ms) in 0..65535, found '%.*s'\n",path,lineno,tokenc,token);
          return -2;
        }
        if (sr_encode_intbe(&SND->dst,ms,2)<0) return -1;
        totalms+=ms;
      }
      while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
      if (srcp>=srcc) {
        fprintf(stderr,"%s:%d: Unexpected end of line reading envelope.\n",path,lineno);
        return -2;
      }
      ptc++;
    }
    
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    int v;
    if ((sr_int_eval(&v,token,tokenc)<2)||(v<0)||(v>0xffff)) {
      fprintf(stderr,"%s:%d: Expected level in 0..65535, found '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
    if (sr_encode_intbe(&SND->dst,v,2)<0) return -1;
    
    if (ptcp<0) { // Insert dummy count after first level.
      ptcp=SND->dst.c;
      if (sr_encode_u8(&SND->dst,0)<0) return -1;
    }
  }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (ptc>0xff) {
    fprintf(stderr,"%s:%d: Too many envelope points (%d, limit 255)\n",path,lineno,ptc);
    return -2;
  }
  if (ptcp<0) {
    fprintf(stderr,"%s:%d: Envelope must contain an initial level.\n",path,lineno);
    return -2;
  }
  SND->dst.v[ptcp]=ptc;
  if (totalms>SND->lenms) {
    fprintf(stderr,"%s:%d: Envelope length %d ms exceeds sound length %d ms.\n",path,lineno,totalms,SND->lenms);
    return -2;
  }
  if (starp>=0) {
    int starlen=SND->lenms-totalms;
    SND->dst.v[starp]=starlen>>8;
    SND->dst.v[starp+1]=starlen;
  }
  return srcp;
}

static int _sound_shape(void *sound,const char *src,int srcc,const char *path,int lineno) {
  // SHAPE is one of ("sine", "square", "sawtooth", "triangle"), or a list of integer in 0..255.
  // shape: u8 name: (200,201,202,203,204)=(sine,square,sawtooth,triangle,noise)
  //   OR: u8 coefc(<200),u8*coefc coefv
  // Shape can only be at the end of line, we're consuming all of (src).
  
  if ((srcc==4)&&!memcmp(src,"sine",4)) {
    if (sr_encode_u8(&SND->dst,200)<0) return -1;
  
  } else if ((srcc==6)&&!memcmp(src,"square",6)) {
    if (sr_encode_u8(&SND->dst,201)<0) return -1;
  
  } else if ((srcc==8)&&!memcmp(src,"sawtooth",8)) {
    if (sr_encode_u8(&SND->dst,202)<0) return -1;
  
  } else if ((srcc==8)&&!memcmp(src,"triangle",8)) {
    if (sr_encode_u8(&SND->dst,203)<0) return -1;
  
  } else if ((srcc==5)&&!memcmp(src,"noise",5)) {
    if (sr_encode_u8(&SND->dst,204)<0) return -1;
    
  } else { // harmonics
    int srcp=0,coefcp=SND->dst.c,coefc=0;
    if (sr_encode_u8(&SND->dst,0)<0) return -1;
    while (srcp<srcc) {
      if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
      const char *token=src+srcp;
      int tokenc=0,v;
      while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
      if ((sr_int_eval(&v,token,tokenc)<2)||(v<0)||(v>0xff)) {
        fprintf(stderr,"%s:%d: Expected integer in 0..255, found '%.*s'\n",path,lineno,tokenc,token);
        if (!coefc) fprintf(stderr,"%s:%d: A wave name would also be ok: sine square sawtooth triangle noise\n",path,lineno);
        return -2;
      }
      if (sr_encode_u8(&SND->dst,v)<0) return -1;
      coefc++;
    }
    if (coefc>=200) {
      fprintf(stderr,"%s:%d: Too many wave coefficients (%d, limit 199)\n",path,lineno,coefc);
      return -2;
    }
    SND->dst.v[coefcp]=coefc;
  }
  
  return srcc;
}

static int _sound_scalar(void *sound,int wbitc,int fbitc,const char *src,int srcc,const char *path,int lineno) {
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *token=src+srcp;
  int tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  double vf;
  if (sr_double_eval(&vf,token,tokenc)<0) {
    fprintf(stderr,"%s:%d: Expected float, found '%.*s'\n",path,lineno,tokenc,token);
    return -2;
  }
  int vi=(vf*(1<<fbitc));
  int limit=(1<<(wbitc+fbitc))-1;
  if (vi<0) vi=0; else if (vi>limit) vi=limit;
  if (sr_encode_intbe(&SND->dst,vi,(wbitc+fbitc)>>3)<0) return -1;
  return srcp;
}

/* Sound input.
 */

static int _sound_line(void *sound,const char *kw,int kwc,const char *src,int srcc,const char *path,int lineno) {
  if (SND->dst.c) {
    // Encode regular commands as we read them.
    int srcp=0,err;

    if ((kwc==2)&&!memcmp(kw,"fm",2)) {
      if (sr_encode_u8(&SND->dst,0x01)<0) return -1;
      if ((err=_sound_env(sound,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,4,4,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      if ((err=_sound_env(sound,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      if ((err=_sound_shape(sound,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      
    } else if ((kwc==4)&&!memcmp(kw,"wave",4)) {
      if (sr_encode_u8(&SND->dst,0x02)<0) return -1;
      if ((err=_sound_env(sound,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      if ((err=_sound_shape(sound,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      
    } else if ((kwc==5)&&!memcmp(kw,"noise",5)) {
      if (sr_encode_u8(&SND->dst,0x03)<0) return -1;
      
    } else if ((kwc==3)&&!memcmp(kw,"env",3)) {
      if (sr_encode_u8(&SND->dst,0x04)<0) return -1;
      if ((err=_sound_env(sound,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      
    } else if ((kwc==3)&&!memcmp(kw,"mlt",3)) {
      if (sr_encode_u8(&SND->dst,0x05)<0) return -1;
      if ((err=_sound_scalar(sound,8,8,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      
    } else if ((kwc==5)&&!memcmp(kw,"delay",5)) {
      if (sr_encode_u8(&SND->dst,0x06)<0) return -1;
      if ((err=_sound_scalar(sound,0,8,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,0,8,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,0,8,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,0,8,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,0,8,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      
    } else if ((kwc==8)&&!memcmp(kw,"bandpass",8)) {
      if (sr_encode_u8(&SND->dst,0x07)<0) return -1;
      if ((err=_sound_scalar(sound,16,0,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,16,0,src+srcp,srcc-srcp,path,lineno))<0) return err; srcp+=err;
      
    } else if ((kwc==11)&&!memcmp(kw,"new_channel",11)) {
      if (sr_encode_u8(&SND->dst,0x08)<0) return -1;
      
    } else {
      fprintf(stderr,"%s:%d: Unknown stdsyn sound command '%.*s'\n",path,lineno,kwc,kw);
      return -2;
    }
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    if (srcp<srcc) {
      fprintf(stderr,"%s:%d: Extra tokens after '%.*s' command: '%.*s'\n",path,lineno,kwc,kw,srcc-srcp,src+srcp);
      return -2;
    }
    
  } else {
    // First command must be "len S"
    double s;
    if (sr_double_eval(&s,src,srcc)<0) {
      fprintf(stderr,"%s:%d: Expected length in seconds, found '%.*s'\n",path,lineno,srcc,src);
      return -2;
    }
    int ms=(int)(s*1000.0);
    int u17=(int)(s*128.0);
    if (ms<1) ms=1;
    if (u17<0) u17=0; else if (u17>0xff) u17=0xff;
    if (sr_encode_u8(&SND->dst,u17)<0) return -1;
    SND->lenms=ms;
  }
  return 0;
}

/* Sound output.
 */

static int _sound_encode(struct sr_encoder *dst,void *sound) {
  if (SND->dst.c<1) return -1;
  if (sr_encode_raw(dst,SND->dst.v,SND->dst.c)<0) return -1;
  return 0;
}

/* Type definition.
 */
 
const struct mkd_instrument_format mkd_instrument_format_stdsyn={
  .name="stdsyn",
  .instrument_del=_instrument_del,
  .instrument_new=_instrument_new,
  .instrument_line=_instrument_line,
  .instrument_encode=_instrument_encode,
  .sound_del=_sound_del,
  .sound_new=_sound_new,
  .sound_line=_sound_line,
  .sound_encode=_sound_encode,
};


#include "mkd_internal.h"
#include "mkd_instrument_format.h"

static int evalhex(int *dst,const char *src,int srcc) {
  *dst=0;
  for (;srcc-->0;src++) {
    int digit=sr_digit_eval(*src);
    if ((digit<0)||(digit>0xf)) return -1;
    if ((*dst)&0xf0000000) return -1;
    (*dst)<<=4;
    (*dst)|=digit;
  }
  return 0;
}

/* Instrument context.
 */
 
struct mkd_WebAudio_instrument {
  const char *path;
  int lineno;
  int id;
  struct sr_encoder encoder;
};

#define INS ((struct mkd_WebAudio_instrument*)instrument)

static void _instrument_del(void *instrument) {
  sr_encoder_cleanup(&INS->encoder);
  free(instrument);
}

static void *_instrument_new(int id,const char *path,int lineno) {
  void *instrument=calloc(1,sizeof(struct mkd_WebAudio_instrument));
  if (!instrument) return 0;
  INS->path=path;
  INS->lineno=lineno;
  INS->id=id;
  return instrument;
}

/* Evaluate and encode a scalar command.
 */
 
static int _instrument_cmd_scalar(void *instrument,uint8_t opcode,int wbitc,int fbitc,const char *src,int srcc) {
  double v;
  if (sr_double_eval(&v,src,srcc)<0) return -1;
  int i=(int)(v*(1<<fbitc));
  if (sr_encode_u8(&INS->encoder,opcode)<0) return -1;
  if (sr_encode_intbe(&INS->encoder,i,(wbitc+fbitc)>>3)<0) return -1;
  return 0;
}

/* Evaluate and encode a command with one wave argument.
 */
 
static int _instrument_cmd_wave(void *instrument,uint8_t opcode,const char *src,int srcc) {
  if (sr_encode_u8(&INS->encoder,opcode)<0) return -1;
  
  if ((srcc==4)&&!memcmp(src,"sine",4)) return sr_encode_u8(&INS->encoder,0x80);
  if ((srcc==6)&&!memcmp(src,"square",6)) return sr_encode_u8(&INS->encoder,0x81);
  if ((srcc==8)&&!memcmp(src,"sawtooth",8)) return sr_encode_u8(&INS->encoder,0x82);
  if ((srcc==8)&&!memcmp(src,"triangle",8)) return sr_encode_u8(&INS->encoder,0x83);
  
  uint16_t coefv[128];
  uint8_t coefc=0;
  int srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (coefc>=127) return -1;
    int i=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) {
      int digit=sr_digit_eval(src[srcp++]);
      if ((digit<0)||(digit>0xf)) return -1;
      i<<=4;
      i|=digit;
      if (i&~0xffff) return -1;
    }
    coefv[coefc++]=i;
  }
  
  if (sr_encode_u8(&INS->encoder,coefc)<0) return -1;
  const uint16_t *v=coefv;
  int i=coefc;
  for (;i-->0;v++) {
    if (sr_encode_intbe(&INS->encoder,*v,2)<0) return -1;
  }
  
  return 0;
}

/* Evaluate and encode a command with one envelope argument.
 */
 
static int _instrument_cmd_env(void *instrument,uint8_t opcode,const char *src,int srcc) {
  
  /* Longest legal value is 15 tokens, and only 4 legal shapes:
   *  T V T V T
   *  T V T V T .. T V T V T
   *  V T V T V T V
   *  V T V T V T V .. V T V T V T V
   */
  struct token { const char *v; int c; } tokenv[15];
  int tokenc=0,srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (tokenc>=15) return -1;
    struct token *token=tokenv+tokenc++;
    token->v=src+srcp;
    token->c=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) token->c++;
  }
  
  /* Record what we need from the token counts, then expand to the full 15-token form.
   */
  uint8_t flags=0;
  if (tokenc>7) flags|=0x80; // Velocity
  if ((tokenc==7)||(tokenc==15)) flags|=0x10; // EdgeLevels
  switch (tokenc) {
    case 5: {
        memmove(tokenv+1,tokenv,sizeof(struct token)*5);
        tokenv[0].v=tokenv[6].v="0";
        tokenv[0].c=tokenv[6].c=1;
        tokenv[7].v="..";
        tokenv[7].c=2;
        memcpy(tokenv+8,tokenv,sizeof(struct token)*7);
      } break;
    case 7: {
        tokenv[7].v="..";
        tokenv[7].c=2;
        memcpy(tokenv+8,tokenv,sizeof(struct token)*7);
      } break;
    case 11: {
        memmove(tokenv+9,tokenv+6,sizeof(struct token)*5);
        memcpy(tokenv+7,tokenv+5,sizeof(struct token));
        memmove(tokenv+1,tokenv,sizeof(struct token)*5);
        tokenv[0].v=tokenv[6].v=tokenv[8].v=tokenv[14].v="0";
        tokenv[0].c=tokenv[6].c=tokenv[8].c=tokenv[14].c=1;
      }
    case 15: break;
    default: return -1;
  }
  
  /* [7] must now be the separator.
   * [2] is a value from the original input -- its size tells us the value resolution.
   */
  if ((tokenv[7].c!=2)||memcmp(tokenv[7].v,"..",2)) return -1;
  if (tokenv[2].c==4) flags|=0x20; // HighResLevels
  
  /* Evaluate the 14 integers. Mind that V are hexadecimal with no prefix.
   */
  int vv[15];
  if (
    (evalhex(    vv+ 0,tokenv[ 0].v,tokenv[ 0].c)<0)||
    (sr_int_eval(vv+ 1,tokenv[ 1].v,tokenv[ 1].c)<2)||
    (evalhex(    vv+ 2,tokenv[ 2].v,tokenv[ 2].c)<0)||
    (sr_int_eval(vv+ 3,tokenv[ 3].v,tokenv[ 3].c)<2)||
    (evalhex(    vv+ 4,tokenv[ 4].v,tokenv[ 4].c)<0)||
    (sr_int_eval(vv+ 5,tokenv[ 5].v,tokenv[ 5].c)<2)||
    (evalhex(    vv+ 6,tokenv[ 6].v,tokenv[ 6].c)<0)||
    (evalhex(    vv+ 8,tokenv[ 8].v,tokenv[ 8].c)<0)||
    (sr_int_eval(vv+ 9,tokenv[ 9].v,tokenv[ 9].c)<2)||
    (evalhex(    vv+10,tokenv[10].v,tokenv[10].c)<0)||
    (sr_int_eval(vv+11,tokenv[11].v,tokenv[11].c)<2)||
    (evalhex(    vv+12,tokenv[12].v,tokenv[12].c)<0)||
    (sr_int_eval(vv+13,tokenv[13].v,tokenv[13].c)<2)||
    (evalhex(    vv+14,tokenv[14].v,tokenv[14].c)<0)
  ) return -1;
  
  /* If any time is above 255, set HighResTimes.
   */
  if (
    (vv[1]>0xff)||(vv[3]>0xff)||(vv[5]>0xff)||(vv[9]>0xff)||(vv[11]>0xff)||(vv[13]>0xff)
  ) flags|=0x40;
  
  int vsize=(flags&0x20)?2:1;
  int tsize=(flags&0x40)?2:1;
  
  if (sr_encode_u8(&INS->encoder,opcode)<0) return -1;
  if (sr_encode_u8(&INS->encoder,flags)<0) return -1;
  if ((flags&0x10)&&(sr_encode_intbe(&INS->encoder,vv[0],vsize)<0)) return -1;
  if (sr_encode_intbe(&INS->encoder,vv[1],tsize)<0) return -1;
  if (sr_encode_intbe(&INS->encoder,vv[2],vsize)<0) return -1;
  if (sr_encode_intbe(&INS->encoder,vv[3],tsize)<0) return -1;
  if (sr_encode_intbe(&INS->encoder,vv[4],vsize)<0) return -1;
  if (sr_encode_intbe(&INS->encoder,vv[5],tsize)<0) return -1;
  if ((flags&0x10)&&(sr_encode_intbe(&INS->encoder,vv[6],vsize)<0)) return -1;
  if (flags&0x80) {
    if ((flags&0x10)&&(sr_encode_intbe(&INS->encoder,vv[8],vsize)<0)) return -1;
    if (sr_encode_intbe(&INS->encoder,vv[9],tsize)<0) return -1;
    if (sr_encode_intbe(&INS->encoder,vv[10],vsize)<0) return -1;
    if (sr_encode_intbe(&INS->encoder,vv[11],tsize)<0) return -1;
    if (sr_encode_intbe(&INS->encoder,vv[12],vsize)<0) return -1;
    if (sr_encode_intbe(&INS->encoder,vv[13],tsize)<0) return -1;
    if ((flags&0x10)&&(sr_encode_intbe(&INS->encoder,vv[14],vsize)<0)) return -1;
  }
  
  return 0;
}

/* Instrument input.
 */

static int _instrument_line(void *instrument,const char *kw,int kwc,const char *src,int srcc,const char *path,int lineno) {

  if ((kwc==4)&&!memcmp(kw,"wave",4)) return _instrument_cmd_wave(instrument,0x01,src,srcc);
  if ((kwc==3)&&!memcmp(kw,"env",3)) return _instrument_cmd_env(instrument,0x02,src,srcc);
  if ((kwc==15)&&!memcmp(kw,"modAbsoluteRate",15)) return _instrument_cmd_scalar(instrument,0x03,16,8,src,srcc);
  if ((kwc==7)&&!memcmp(kw,"modRate",7)) return _instrument_cmd_scalar(instrument,0x04,8,8,src,srcc);
  if ((kwc==8)&&!memcmp(kw,"modRange",8)) return _instrument_cmd_scalar(instrument,0x05,8,8,src,srcc);
  if ((kwc==6)&&!memcmp(kw,"modEnv",6)) return _instrument_cmd_env(instrument,0x06,src,srcc);
  if ((kwc==15)&&!memcmp(kw,"modRangeLfoRate",15)) return _instrument_cmd_scalar(instrument,0x07,16,8,src,srcc);
  if ((kwc==10)&&!memcmp(kw,"wheelRange",10)) return _instrument_cmd_scalar(instrument,0x08,16,0,src,srcc);
  if ((kwc==3)&&!memcmp(kw,"bpq",3)) return _instrument_cmd_scalar(instrument,0x09,8,8,src,srcc);
  if ((kwc==4)&&!memcmp(kw,"bpq2",4)) return _instrument_cmd_scalar(instrument,0x0a,8,8,src,srcc);
  if ((kwc==7)&&!memcmp(kw,"bpBoost",7)) return _instrument_cmd_scalar(instrument,0x0b,16,0,src,srcc);
  
  fprintf(stderr,"%s:%d: Unknown command '%.*s' for WebAudio instrument\n",path,lineno,kwc,kw);
  return 0;
}

/* Instrument output.
 */

static int _instrument_encode(struct sr_encoder *dst,void *instrument) {
  return sr_encode_raw(dst,INS->encoder.v,INS->encoder.c);
}

/* Sound context.
 */
 
struct mkd_WebAudio_sound {
  const char *path;
  int lineno;
  int id;
  struct sr_encoder encoder;
  uint8_t maxbufid;
  uint16_t lenms;
};

#define SND ((struct mkd_WebAudio_sound*)sound)

static void _sound_del(void *sound) {
  sr_encoder_cleanup(&SND->encoder);
  free(sound);
}

static void *_sound_new(int id,const char *path,int lineno) {
  void *sound=calloc(1,sizeof(struct mkd_WebAudio_sound));
  if (!sound) return 0;
  SND->path=path;
  SND->lineno=lineno;
  SND->id=id;
  return sound;
}

/* Sound bits.
 * All consume leading and trailing space.
 */
 
static int _sound_bufid(void *sound,const char *src,int srcc) {
  int srcp=0,tokenc=0,v;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *token=src+srcp;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (sr_int_eval(&v,token,tokenc)<2) return -1;
  if ((v<0)||(v>254)) return -1;
  if (sr_encode_u8(&SND->encoder,v)<0) return -1;
  if (v>SND->maxbufid) SND->maxbufid=v;
  return srcp;
}

// input in seconds, output in milliseconds
static int _sound_ms_u16(void *sound,const char *src,int srcc) {
  int srcp=0,tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *token=src+srcp;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  double s;
  if (sr_double_eval(&s,token,tokenc)<0) return -1;
  int ms=(int)(s*1000.0);
  if (ms>0xffff) ms=0xffff;
  else if (ms<0) ms=0;
  if (sr_encode_intbe(&SND->encoder,ms,2)<0) return -1;
  return srcp;
}

static int _sound_scalar(void *sound,int wbitc,int fbitc,const char *src,int srcc) {
  int srcp=0,tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *token=src+srcp;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  double vf;
  if (sr_double_eval(&vf,token,tokenc)<0) return -1;
  int vi=(int)(vf*(1<<fbitc));
  int limit=(1<<(wbitc+fbitc));
  if (vi>=limit) vi=limit-1;
  else if (vi<0) vi=0;
  if (sr_encode_intbe(&SND->encoder,vi,(wbitc+fbitc)>>3)<0) return -1;
  return srcp;
}

static int _sound_opt_scalar(void *sound,int wbitc,int fbitc,const char *src,int srcc,const char *subst) {
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp>=srcc) {
    int substc=0; while (subst[substc]) substc++;
    if (_sound_scalar(sound,wbitc,fbitc,subst,substc)<0) return -1;
    return srcp;
  }
  return _sound_scalar(sound,wbitc,fbitc,src,srcc);
}

static int _sound_shape(void *sound,const char *src,int srcc) {
  int srcp=0,tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *token=src+srcp;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  uint8_t v;
  if ((tokenc==4)&&!memcmp(token,"sine",4)) v=0x00;
  else if ((tokenc==6)&&!memcmp(token,"square",6)) v=0x01;
  else if ((tokenc==8)&&!memcmp(token,"sawtooth",8)) v=0x02;
  else if ((tokenc==8)&&!memcmp(token,"triangle",8)) v=0x03;
  else return -1;
  if (sr_encode_u8(&SND->encoder,v)<0) return -1;
  return srcp;
}

static int _sound_harmonics(void *sound,const char *src,int srcc) {
  int coefcp=SND->encoder.c,srcp=0,coefc=0;
  if (sr_encode_u8(&SND->encoder,0)<0) return -1;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    int v;
    if (evalhex(&v,token,tokenc)<0) return -1;
    if (v>0xffff) v=0xffff; else if (v<0) v=0;
    if (sr_encode_intbe(&SND->encoder,v,2)<0) return -1;
    coefc++;
  }
  if (coefc>0xff) return -1;
  SND->encoder.v[coefcp]=coefc;
  return srcp;
}

static int _sound_env(void *sound,const char *src,int srcc) {
// ENV is "( RATE TIME RATE TIME ... RATE )", RATE in Hz, TIME in ms. One TIME may be "*" to fill in.
// ENV: u16 level, u8 count, then count * (u16 timems,u16 level)
  int srcp=0,tokenc=0,v;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((srcp>=srcc)||(src[srcp++]!='(')) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *token=src+srcp;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  if ((sr_int_eval(&v,token,tokenc)<2)||(v<0)||(v>0xffff)) return -1;
  if (sr_encode_intbe(&SND->encoder,v,2)<0) return -1; // initial level
  int ptcp=SND->encoder.c,ptc=0;
  if (sr_encode_u8(&SND->encoder,0)<0) return -1; // count placeholder
  uint16_t totaltime=0;
  int starp=-1;
  while (1) {
    if (srcp>=srcc) return -1;
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (src[srcp]==')') { srcp++; break; }
    token=src+srcp;
    tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    if ((tokenc==1)&&(token[0]=='*')) {
      if (starp>=0) {
        fprintf(stderr,"Envelope contains multiple stars, limit to just one please.\n");
        return -1;
      }
      starp=SND->encoder.c;
      if (sr_encode_raw(&SND->encoder,"\0\0",2)<0) return -1;
    } else {
      if ((sr_int_eval(&v,token,tokenc)<0)||(v<0)||(v>0xffff)) return -1;
      if (sr_encode_intbe(&SND->encoder,v,2)<0) return -1; // time
      totaltime+=v;
    }
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    token=src+srcp;
    tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    if ((sr_int_eval(&v,token,tokenc)<0)||(v<0)||(v>0xffff)) return -1;
    if (sr_encode_intbe(&SND->encoder,v,2)<0) return -1; // level
    ptc++;
  }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (ptc>0xff) return -1;
  if (totaltime>SND->lenms) {
    fprintf(stderr,"Envelope length %d in sound length %d.\n",totaltime,SND->lenms);
    return -1;
  }
  SND->encoder.v[ptcp]=ptc;
  if (starp>=0) {
    uint16_t starlen=SND->lenms-totaltime;
    SND->encoder.v[starp]=starlen>>8;
    SND->encoder.v[starp+1]=starlen;
  }
  return srcp;
}

/* Sound input.
 */

static int _sound_line(void *sound,const char *kw,int kwc,const char *src,int srcc,const char *path,int lineno) {

  if (SND->encoder.c) {
    // Have length, so we're reading general commands.
    int srcp=0,err;
    
    if ((kwc==5)&&!memcmp(kw,"noise",5)) {
      // `noise BUFFER`
      // 0x01 NOISE (u8 buf)
      if (sr_encode_u8(&SND->encoder,0x01)<0) return -1;
      if ((err=_sound_bufid(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      
    } else if ((kwc==4)&&!memcmp(kw,"wave",4)) {
      // `wave BUFFER RATE SHAPE`
      // `wave BUFFER ( ENV ) SHAPE`
      // 0x02 WAVE_FIXED_NAME (u8 buf,u16 rate,u8 name)
      // 0x03 WAVE_FIXED_HARM (u8 buf,u16 rate,u8 coefc,u16... coefv)
      // 0x04 WAVE_ENV_NAME (u8 buf,ENV,u8 name)
      // 0x05 WAVE_ENV_HARM (u8 buf,ENV,u8 coefc,u16... coefv)
      int opcodep=SND->encoder.c,env=0,harm=0;
      if (sr_encode_u8(&SND->encoder,0)<0) return -1;
      if ((err=_sound_bufid(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((srcp<srcc)&&(src[srcp]=='(')) { // rate envelope
        if ((err=_sound_env(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
        env=1;
      } else {
        if ((err=_sound_scalar(sound,16,0,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      }
      if ((srcp<srcc)&&(src[srcp]>='g')&&(src[srcp]<='z')) { // named shape (all start with 's' or 't')
        if ((err=_sound_shape(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      } else {
        if ((err=_sound_harmonics(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
        harm=1;
      }
      if (env) {
        if (harm) SND->encoder.v[opcodep]=0x05;
        else SND->encoder.v[opcodep]=0x04;
      } else {
        if (harm) SND->encoder.v[opcodep]=0x03;
        else SND->encoder.v[opcodep]=0x02;
      }
      
    } else if ((kwc==2)&&!memcmp(kw,"fm",2)) {
      // `fm BUFFER RATE MODRATE ( RANGEENV ) SHAPE`
      // `fm BUFFER ( ENV ) MODRATE ( RANGEENV ) SHAPE`
      // 0x06 FM_FIXED_NAME (u8 buf,u16 rate,u8.8 modrate,ENV range,u8 name)
      // 0x07 FM_FIXED_HARM (u8 buf,u16 rate,u8.8 modrate,ENV range,u8 coefc,u16... coefv)
      // 0x08 FM_ENV_NAME (u8 buf,ENV rate,u8.8 modrate,ENV range,u8 name)
      // 0x09 FM_ENV_HARM (u8 buf,ENV rate,u8.8 modrate,ENV range,u8 coefc,u16... coefv)
      int opcodep=SND->encoder.c,env=0,harm=0;
      if (sr_encode_u8(&SND->encoder,0)<0) return -1;
      if ((err=_sound_bufid(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((srcp<srcc)&&(src[srcp]=='(')) { // rate envelope
        if ((err=_sound_env(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
        env=1;
      } else {
        if ((err=_sound_scalar(sound,16,0,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      }
      if ((err=_sound_scalar(sound,8,8,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_env(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((srcp<srcc)&&(src[srcp]>='g')&&(src[srcp]<='z')) { // named shape
        if ((err=_sound_shape(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      } else {
        if ((err=_sound_harmonics(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
        harm=1;
      }
      if (env) {
        if (harm) SND->encoder.v[opcodep]=0x09;
        else SND->encoder.v[opcodep]=0x08;
      } else {
        if (harm) SND->encoder.v[opcodep]=0x07;
        else SND->encoder.v[opcodep]=0x06;
      }
      
    } else if ((kwc==4)&&!memcmp(kw,"gain",4)) {
      // `gain BUFFER MLT [CLIP [GATE]]`
      // 0x0a GAIN (u8 buf,u8.8 mlt,u0.8 clip, u0.8 gate)
      if (sr_encode_u8(&SND->encoder,0x0a)<0) return -1;
      if ((err=_sound_bufid(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,8,8,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_opt_scalar(sound,0,8,src+srcp,srcc-srcp,"1.0"))<0) return err; srcp+=err;
      if ((err=_sound_opt_scalar(sound,0,8,src+srcp,srcc-srcp,"0.0"))<0) return err; srcp+=err;
      
    } else if ((kwc==3)&&!memcmp(kw,"env",3)) {
      // `env BUFFER ( LEVEL TIME LEVEL TIME ... LEVEL )`
      // 0x0b ENV (u8 buf,ENV)
      if (sr_encode_u8(&SND->encoder,0x0b)<0) return -1;
      if ((err=_sound_bufid(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_env(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      
    } else if ((kwc==3)&&!memcmp(kw,"mix",3)) {
      // `mix DSTBUFFER SRCBUFFER`
      // 0x0c MIX (u8 dst,u8 src)
      if (sr_encode_u8(&SND->encoder,0x0c)<0) return -1;
      if ((err=_sound_bufid(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_bufid(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      
    } else if ((kwc==4)&&!memcmp(kw,"norm",4)) {
      // `norm BUFFER [PEAK]`
      // 0x0d norm (u8 buf,u0.8 peak)
      if (sr_encode_u8(&SND->encoder,0x0d)<0) return -1;
      if ((err=_sound_bufid(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_opt_scalar(sound,0,8,src+srcp,srcc-srcp,"1.0"))<0) return err; srcp+=err;
      
    } else if ((kwc==5)&&!memcmp(kw,"delay",5)) {
      // `delay BUFFER DURATION DRY WET STORE FEEDBACK`
      // 0x0e delay (u8 buf,u16 durationMs,u0.8 dry,u0.8 wet,u0.8 store,u0.8 feedback)
      if (sr_encode_u8(&SND->encoder,0x0e)<0) return -1;
      if ((err=_sound_bufid(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_ms_u16(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,0,8,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,0,8,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,0,8,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,0,8,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      
    } else if ((kwc==8)&&!memcmp(kw,"bandpass",8)) {
      // `bandpass BUFFER MIDFREQ RANGE`
      // 0x0f bandpass (u8 buf,u16 midfreq,u16 range)
      if (sr_encode_u8(&SND->encoder,0x0f)<0) return -1;
      if ((err=_sound_bufid(sound,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,16,0,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      if ((err=_sound_scalar(sound,16,0,src+srcp,srcc-srcp))<0) return err; srcp+=err;
      
    } else {
      fprintf(stderr,"%s:%d: Unexpected sound command '%.*s'\n",path,lineno,kwc,kw);
      return -2;
    }
    if (srcp<srcc) {
      fprintf(stderr,"%s:%d: Unexpected extra tokens '%.*s'\n",path,lineno,srcc-srcp,src+srcp);
      return -2;
    }
    
  } else {
    // First command must be "len".
    if ((kwc!=3)||memcmp(kw,"len",3)) {
      fprintf(stderr,"%s:%d: Sound must begin with 'len S'\n",path,lineno);
      return -2;
    }
    double s;
    if (sr_double_eval(&s,src,srcc)<0) return -1;
    SND->lenms=(s*1000.0);
    s*=64.0;
    int si=(int)s;
    if (si>0xff) si=0xff;
    else if (si<0) si=0;
    if (sr_encode_u8(&SND->encoder,si)<0) return -1;
    if (sr_encode_u8(&SND->encoder,0)<0) return -1; // buffer count; will fill in at the end.
  }

  return 0;
}

/* Sound output.
 */

static int _sound_encode(struct sr_encoder *dst,void *sound) {
  if (SND->encoder.c<2) {
    fprintf(stderr,"%s:%d: Expected 'len S'\n",SND->path,SND->lineno);
    return -2;
  }
  SND->encoder.v[1]=SND->maxbufid+1;
  if (sr_encode_raw(dst,SND->encoder.v,SND->encoder.c)<0) return -1;
  return 0;
}

/* Type definition.
 */
 
const struct mkd_instrument_format mkd_instrument_format_WebAudio={
  .name="WebAudio",
  .instrument_del=_instrument_del,
  .instrument_new=_instrument_new,
  .instrument_line=_instrument_line,
  .instrument_encode=_instrument_encode,
  .sound_del=_sound_del,
  .sound_new=_sound_new,
  .sound_line=_sound_line,
  .sound_encode=_sound_encode,
};


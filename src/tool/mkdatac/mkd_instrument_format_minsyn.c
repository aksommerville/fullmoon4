#include "mkd_internal.h"
#include "mkd_instrument_format.h"

/* Instrument context.
 */
 
struct mkd_minsyn_instrument {
  const char *path;
  int lineno;
  int id;
  // All features, pre-encoded when we read them, in the order of their feature bit (little-endian):
  uint8_t *wave; // 1..256
  uint8_t *loenv; // 5
  uint8_t *hienv; // 5
  uint8_t *mixwave; // 1..256
  uint8_t *lomixenv; // 5
  uint8_t *himixenv; // 5
};

#define INS ((struct mkd_minsyn_instrument*)instrument)

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
  void *instrument=calloc(1,sizeof(struct mkd_minsyn_instrument));
  if (!instrument) return 0;
  INS->path=path;
  INS->lineno=lineno;
  INS->id=id;
  return instrument;
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

/* Instrument input.
 */

static int _instrument_line(void *instrument,const char *kw,int kwc,const char *src,int srcc,const char *path,int lineno) {

  if ((kwc==4)&&!memcmp(kw,"wave",4)) {
    return _instrument_wave(&INS->wave,instrument,src,srcc,path,lineno);
    
  } else if ((kwc==3)&&!memcmp(kw,"env",3)) {
    return _instrument_env(&INS->loenv,&INS->hienv,instrument,src,srcc,path,lineno);
    
  } else if ((kwc==7)&&!memcmp(kw,"mixwave",7)) {
    return _instrument_wave(&INS->mixwave,instrument,src,srcc,path,lineno);
    
  } else if ((kwc==6)&&!memcmp(kw,"mixenv",6)) {
    return _instrument_env(&INS->lomixenv,&INS->himixenv,instrument,src,srcc,path,lineno);
    
  } else {
    fprintf(stderr,"%s:%d: Unexpected instrument command '%.*s'\n",path,lineno,kwc,kw);
    return -2;
  }
  return 0;
}

/* Instrument output.
 */

static int _instrument_encode(struct sr_encoder *dst,void *instrument) {

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
 
struct mkd_minsyn_sound {
  const char *path;
  int lineno;
  int id;
  int lenms;
  struct sr_encoder dst;
};

#define SND ((struct mkd_minsyn_sound*)sound)

static void _sound_del(void *sound) {
  sr_encoder_cleanup(&SND->dst);
  free(sound);
}

static void *_sound_new(int id,const char *path,int lineno) {
  void *sound=calloc(1,sizeof(struct mkd_minsyn_sound));
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
      fprintf(stderr,"%s:%d: Unknown minsyn sound command '%.*s'\n",path,lineno,kwc,kw);
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
 
const struct mkd_instrument_format mkd_instrument_format_minsyn={
  .name="minsyn",
  .instrument_del=_instrument_del,
  .instrument_new=_instrument_new,
  .instrument_line=_instrument_line,
  .instrument_encode=_instrument_encode,
  .sound_del=_sound_del,
  .sound_new=_sound_new,
  .sound_line=_sound_line,
  .sound_encode=_sound_encode,
};


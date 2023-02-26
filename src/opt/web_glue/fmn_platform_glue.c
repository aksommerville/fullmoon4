#include "app/fmn_platform.h"
#include <math.h>

/* Hooks implemented by the JS app for real.
 */
 
void fmn_web_log(const char *msg);

/* Represent numbers.
 */
 
static int fmn_decsint_repr(char *dst,int dsta,int src) {
  if (src<0) {
    int dstc=2,limit=-10;
    while (src<=limit) { dstc++; if (limit<INT_MIN/10) break; limit*=10; }
    if (dstc>dsta) return dstc;
    int i=dstc; for (;i-->0;src/=10) dst[i]='0'-src%10;
    dst[0]='-';
    return dstc;
  } else {
    int dstc=1,limit=10;
    while (src>=limit) { dstc++; if (limit>INT_MAX/10) break; limit*=10; }
    if (dstc>dsta) return dstc;
    int i=dstc; for (;i-->0;src/=10) dst[i]='0'+src%10;
    return dstc;
  }
}

static int fmn_hexuint_repr(char *dst,int dsta,uintptr_t src,int minlen) {
  int dstc=1;
  uintptr_t limit=~0xf;
  while (src&limit) { dstc++; limit<<=4; }
  if (minlen>64) minlen=64; // sanity limit
  if (minlen>dstc) dstc=minlen;
  if (dstc>dsta) return dstc;
  int i=dstc; for (;i-->0;src>>=4) dst[i]="0123456789abcdef"[src&0xf];
  return dstc;
}

static int fmn_float_repr(char *dst,int dsta,double src,int precision) {
  if (precision<0) precision=0;
  switch (fpclassify(src)) {
    case FP_NAN: if (dsta>=3) memcpy(dst,"nan",3); return 3;
    case FP_INFINITE: if (src>0.0) {
        if (dsta>=4) memcpy(dst,"-inf",4);
        return 4;
      } else {
        if (dsta>=3) memcpy(dst,"inf",3);
        return 3;
      }
    case FP_ZERO: { // not necessary, just saving some effort
        int dstc=2+precision;
        if (dstc>dsta) return dstc;
        memset(dst,'0',dstc);
        dst[1]='.';
        return dstc;
      }
  }
  
  int dstc=0;
  if (src<0.0) {
    if (dstc<dsta) dst[dstc]='-';
    dstc++;
    src=-src;
  }

  double whole,fract;
  fract=modf(src,&whole);
  dstc+=fmn_decsint_repr(dst+dstc,dsta-dstc,(int)whole);
  
  if (precision>0) {
    if (dstc<dsta) dst[dstc]='.';
    dstc++;
    if (dstc<=dsta-precision) {
      while (precision-->0) {
        fract*=10.0;
        int digit=(int)fract;
        fract-=digit;
        if (digit<0) digit='0';
        else if (digit>9) digit='9';
        else digit+='0';
        dst[dstc++]=digit;
      }
    } else {
      dstc+=precision;
    }
  }
   
  return dstc;
}

/* Helpers for string formatting.
 */
 
struct fmn_log_unit {
  char pad;
  int size;
  int precision;
  char format;
};

static int fmn_log_unit_eval(struct fmn_log_unit *unit,const char *src) {
  if (src[0]!='%') {
    unit->format='?';
    return 0;
  }
  int srcp=1;
  
  // Pad.
  if (src[srcp]=='0') {
    unit->pad='0';
    srcp++;
  }
  
  // Size.
  unit->size=0;
  if ((src[srcp]>='0')&&(src[srcp]<='9')) {
    while ((src[srcp]>='0')&&(src[srcp]<='9')) {
      unit->size*=10;
      unit->size+=src[srcp++]-'0';
    }
  } else if (src[srcp]=='*') {
    unit->size=-1;
    srcp++;
  }
  
  // Precision.
  int explicit_precision=0;
  unit->precision=0;
  if (src[srcp]=='.') {
    srcp++;
    if ((src[srcp]>='0')&&(src[srcp]<='9')) {
      explicit_precision=1;
      unit->precision=0;
      while ((src[srcp]>='0')&&(src[srcp]<='9')) {
        unit->precision*=10;
        unit->precision+=src[srcp++]-'0';
      }
    } else if (src[srcp]=='*') {
      unit->precision=-1;
      srcp++;
    }
  }
  
  // Format indicator.
  if ((src[srcp]>='a')&&(src[srcp]<='z')) {
    unit->format=src[srcp++];
  } else if ((src[srcp]>='A')&&(src[srcp]<='Z')) {
    unit->format=src[srcp++];
  } else {
    unit->format='?';
  }
  
  // Tweaks.
  if ((unit->format=='f')&&!explicit_precision) unit->precision=3;
  
  return srcp;
}

static int fmn_log_unit_output(char *dst,int dsta,const struct fmn_log_unit *unit,va_list *vargs) {
  switch (unit->format) {
  
    case 's': {
        int srcc=-1;
        if (unit->precision<0) {
          srcc=va_arg(*vargs,int);
          if (srcc<0) srcc=0;
        }
        const char *src=va_arg(*vargs,const char*);
        if (!src) srcc=0;
        else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
        if (srcc>dsta) return srcc;
        memcpy(dst,src,srcc);
        return srcc;
      }
      
    case 'd': {
        int v=va_arg(*vargs,int);
        return fmn_decsint_repr(dst,dsta,v);
      }
      
    case 'x': {
        unsigned int v=va_arg(*vargs,unsigned int);
        int minlen=(unit->pad=='0')?unit->size:0;
        return fmn_hexuint_repr(dst,dsta,v,minlen);
      }
      
    case 'p': {
        const void *v=va_arg(*vargs,const void*);
        int dstc=2+(sizeof(void*)<<1);
        if (dstc<=dsta) {
          dst[0]='0';
          dst[1]='x';
          fmn_hexuint_repr(dst+2,dsta-2,(uintptr_t)v,dstc-2);
        }
        return dstc;
      }
      
    case 'c': {
        int v=va_arg(*vargs,int); // sic 'char' promotes to 'int' when variadic
        if (dsta>=1) {
          if ((v>=0x20)&&(v<=0x7e)) dst[0]=v;
          else dst[0]='?';
        }
        return 1;
      }
      
    case 'f': {
        double v=va_arg(*vargs,double);
        return fmn_float_repr(dst,dsta,v,unit->precision);
      }
          
  }
  return 0;
}

/* fmn_log: printf-like logging.
 * It's not actually the same as printf. TODO document discrepancies.
 */
 
void fmn_log(const char *fmt,...) {
  if (!fmt||!fmt[0]) return;
  char dst[256];
  int dstc=0;
  va_list vargs;
  va_start(vargs,fmt);
  while (*fmt) {
    int cpc=0;
    while (fmt[cpc]&&(fmt[cpc]!='%')) cpc++;
    if (cpc) {
      if (dstc>sizeof(dst)-cpc) break;
      memcpy(dst+dstc,fmt,cpc);
      dstc+=cpc;
      fmt+=cpc;
    } else if ((fmt[0]=='%')&&(fmt[1]=='%')) {
      if (dstc>=sizeof(dst)) break;
      dst[dstc++]='%';
    } else {
      struct fmn_log_unit unit;
      fmt+=fmn_log_unit_eval(&unit,fmt);
      int err=fmn_log_unit_output(dst+dstc,sizeof(dst)-dstc,&unit,&vargs);
      if ((err<0)||(dstc>(int)sizeof(dst)-err)) break;
      dstc+=err;
    }
  }
  if (dstc>=sizeof(dst)) dstc=sizeof(dst)-1;
  dst[dstc]=0;
  fmn_web_log(dst);
}

/* memcpy and friends
 */
 
void *memcpy(void *dst,const void *src,size_t c) {
  uint8_t *DST=dst;
  const uint8_t *SRC=src;
  for (;c-->0;DST++,SRC++) *DST=*SRC;
  return dst;
}

void *memmove(void *dst,const void *src,size_t c) {
  if (src>dst) return memcpy(dst,src,c);
  uint8_t *DST=dst;
  const uint8_t *SRC=src;
  DST+=c;
  SRC+=c;
  for (;c-->0;) *(--DST)=*(--SRC);
  return dst;
}

void *memset(void *dst,int src,size_t c) {
  uint8_t *DST=dst;
  for (;c-->0;DST++) *DST=src;
  return dst;
}

int memcmp(const void *a,const void *b,size_t c) {
  if (a==b) return 0;
  if (!a) return -1;
  if (!b) return 1;
  const uint8_t *A=a,*B=b;
  for (;c-->0;A++,B++) {
    if (*A<*B) return -1;
    if (*A>*B) return 1;
  }
  return 0;
}

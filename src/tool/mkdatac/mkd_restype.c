#include "mkd_internal.h"

/* Friendly resource id representation.
 */
 
int mkd_resid_repr(char *dst,int dsta,int type,int qualifier,int id) {
  int dstc=0;
  
  /* Type name, lowercase.
   * Unsigned decimal integer if we don't know its name.
   */
  const char *gtype=mkd_restype_repr(type);
  if (!gtype||!gtype[0]||(gtype[0]=='?')) {
    dstc+=sr_decuint_repr(dst+dstc,dsta-dstc,type,0);
  } else {
    for (;*gtype;gtype++) {
      if (dstc<dsta) {
        if ((*gtype>='A')&&(*gtype<='Z')) dst[dstc]=(*gtype)+0x20;
        else dst[dstc]=*gtype;
      }
      dstc++;
    }
  }
  
  /* ".QUALIFIER".
   * Use friendly labels per type if we know it.
   * Or an unsigned decimal integer if unknown.
   * Skip altogether if zero.
   */
  if (qualifier) {
    int qok=0;
    switch (type) {
      case FMN_RESTYPE_IMAGE: break;//TODO image qualifier
      case FMN_RESTYPE_SONG: break; // Shouldn't have a qualifier
      case FMN_RESTYPE_MAP: switch (qualifier) {
          case 1: if (dstc<=dsta-5) memcpy(dst+dstc,".full",5); dstc+=5; qok=1; break;
          case 2: if (dstc<=dsta-5) memcpy(dst+dstc,".demo",5); dstc+=5; qok=1; break;
        } break;
      case FMN_RESTYPE_TILEPROPS: break; // Shouldn't have a qualifier
      case FMN_RESTYPE_SPRITE: break; // Shouldn't have a qualifier
      case FMN_RESTYPE_STRING: { // ISO 631 language codes. Confirm it's two ASCII lowercase letters.
          uint8_t a=qualifier>>8;
          uint8_t b=qualifier;
          if ((a>'a')&&(a<='z')&&(b>'a')&&(b<'z')) {
            if (dstc<dsta) dst[dstc]='.'; dstc++;
            if (dstc<=dsta-2) {
              dst[dstc++]=a;
              dst[dstc++]=b;
            } else dstc+=2;
            qok=1;
          }
        } break;
      case FMN_RESTYPE_INSTRUMENT: // same as sound...
      case FMN_RESTYPE_SOUND: switch (qualifier) {
          case 1: if (dstc<=dsta-9) memcpy(dst+dstc,".WebAudio",9); dstc+=9; qok=1; break;
          case 2: if (dstc<=dsta-7) memcpy(dst+dstc,".minsyn",7); dstc+=7; qok=1; break;
          case 3: if (dstc<=dsta-7) memcpy(dst+dstc,".stdsyn",7); dstc+=7; qok=1; break;
        } break;
    }
    if (!qok) {
      if (dstc<dsta) dst[dstc]='.'; dstc++;
      dstc+=sr_decuint_repr(dst+dstc,dsta-dstc,qualifier,0);
    }
  }
  
  /* ":ID"
   */
  if (dstc<dsta) dst[dstc]=':'; dstc++;
  dstc+=sr_decuint_repr(dst+dstc,dsta-dstc,id,0);
  
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* Resource ID from basename.
 */
 
static int mkd_resid_from_base(const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int id=0;
  for (;srcc-->0;src++) {
    if (*src=='-') return id;
    if (*src=='.') return id;
    if ((*src>='0')&&(*src<='9')) {
      int digit=(*src)-'0';
      if (id>INT_MAX/10) return 0;
      id*=10;
      if (id>INT_MAX-digit) return 0;
      id+=digit;
    } else {
      return 0;
    }
  }
  return id;
}

/* Evaluate resource type.
 */
 
int mkd_restype_eval(const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char norm[16];
  if (srcc>sizeof(norm)) return 0;
  int i=srcc; while (i-->0) {
    if ((src[i]>='a')&&(src[i]<='z')) norm[i]=src[i]-0x20;
    else norm[i]=src[i];
  }
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(norm,#tag,srcc)) return FMN_RESTYPE_##tag;
  _(IMAGE)
  _(SONG)
  _(MAP)
  _(TILEPROPS)
  _(SPRITE)
  _(STRING)
  _(INSTRUMENT)
  _(SOUND)
  #undef _
  if ((srcc==8)&&!memcmp(norm,"MAP-DEMO",8)) return FMN_RESTYPE_MAP;
  if ((srcc==8)&&!memcmp(norm,"MAP-FULL",8)) return FMN_RESTYPE_MAP;
  return 0;
}

const char *mkd_restype_repr(int restype) {
  switch (restype) {
    #define _(tag) case FMN_RESTYPE_##tag: return #tag;
    _(IMAGE)
    _(SONG)
    _(MAP)
    _(TILEPROPS)
    _(SPRITE)
    _(STRING)
    _(INSTRUMENT)
    _(SOUND)
    #undef _
  }
  return "?";
}

/* Check for known non-resource files.
 * If it matches, we set restype==FMN_RESTYPE_KNOWN_UNKNOWN and return nonzero.
 */
 
static int mkd_respath_check_known_unknown(struct mkd_respath *respath) {
  if (respath->restype) return 0;
  if ((respath->basec==5)&&!memcmp(respath->base,"gsbit",5)) { respath->restype=FMN_RESTYPE_KNOWN_UNKNOWN; return 1; }
  return 0;
}

/* Determine qualifier for string set.
 */
 
static int mkd_respath_eval_string(struct mkd_respath *respath) {
  respath->qname=respath->base;
  respath->qnamec=respath->basec;
  if (
    (respath->qnamec==2)&&
    (respath->qname[0]>='a')&&(respath->qname[0]<='z')&&
    (respath->qname[1]>='a')&&(respath->qname[1]<='z')
  ) {
    respath->resq=(respath->qname[0]<<8)|respath->qname[1];
  }
  return 0;
}

/* Determine qualifier for instrument+sound set.
 */
 
static int mkd_respath_eval_instrument(struct mkd_respath *respath) {
  respath->qname=respath->base;
  respath->qnamec=respath->basec;
  if ((respath->qnamec==8)&&!memcmp(respath->qname,"WebAudio",8)) { respath->resq=1; return 0; }
  if ((respath->qnamec==6)&&!memcmp(respath->qname,"minsyn",6)) { respath->resq=2; return 0; }
  if ((respath->qnamec==6)&&!memcmp(respath->qname,"stdsyn",6)) { respath->resq=3; return 0; }
  return 0;
}

/* Determine qualifier for map.
 * For maps, qualifier is in the type name.
 * "map-full/N" or "map-demo/N". Anything else is qualifier zero.
 */
 
static int mkd_respath_eval_map(struct mkd_respath *respath) {
  if (!respath->qnamec) {
    if ((respath->tnamec<4)||memcmp(respath->tname,"map-",4)) return 0;
    respath->qname=respath->tname+4;
    respath->qnamec=respath->tnamec-4;
  }
       if ((respath->qnamec==4)&&!memcmp(respath->qname,"full",4)) respath->resq=1;
  else if ((respath->qnamec==4)&&!memcmp(respath->qname,"demo",4)) respath->resq=2;
  return 0;
}

/* General eval of qualifier.
 */
 
int mkd_qualifier_eval(int type,const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (!srcc) return 0;
  struct mkd_respath respath={
    .restype=type,
    .base=src,.basec=srcc,
    .qname=src,.qnamec=srcc,
    .resq=-1,
  };
  if (sr_int_eval(&respath.resq,src,srcc)>=2) return respath.resq;
  switch (type) {
    case FMN_RESTYPE_IMAGE: break; // qualifier TODO (format and resolution)
    case FMN_RESTYPE_MAP: if (mkd_respath_eval_map(&respath)<0) return -1; break;
    case FMN_RESTYPE_STRING: if (mkd_respath_eval_string(&respath)<0) return -1; break;
    case FMN_RESTYPE_SOUND: // SOUND live in the same files as INSTRUMENT, we shouldn't see them here at all.
    case FMN_RESTYPE_INSTRUMENT: if (mkd_respath_eval_instrument(&respath)<0) return -1; break;
  }
  return respath.resq;
}

/* Split and evaluate a path for one input resource.
 */
 
int mkd_respath_eval(struct mkd_respath *respath,const char *src) {

  // First collect the path, basename, and last directory name.
  if (!src) src="";
  respath->path=src;
  respath->pathc=0;
  respath->tname=0;
  respath->tnamec=0;
  respath->base=src;
  respath->basec=0;
  while (respath->path[respath->pathc]) {
    if (respath->path[respath->pathc]=='/') {
      respath->tname=respath->base;
      respath->tnamec=respath->basec;
      respath->base=respath->path+respath->pathc+1;
      respath->basec=0;
    } else { 
      respath->basec++;
    }
    respath->pathc++;
  }
  
  // (resid) comes straight off (base).
  respath->resid=mkd_resid_from_base(respath->base,respath->basec);
  
  // (restype) comes straight off (tname).
  respath->restype=mkd_restype_eval(respath->tname,respath->tnamec);
  
  // Qualifier is more subtle.
  respath->qname=0;
  respath->qnamec=0;
  respath->resq=0;
  
  // Check for "known unknown" files, flag them to ignore.
  if (mkd_respath_check_known_unknown(respath)) return 0;
  
  switch (respath->restype) {
    case FMN_RESTYPE_SONG: break; // no qualifier
    case FMN_RESTYPE_TILEPROPS: break; // no qualifier
    case FMN_RESTYPE_SPRITE: break; // no qualifier
    
    case FMN_RESTYPE_IMAGE: break; // qualifier TODO (format and resolution)
    
    case FMN_RESTYPE_MAP: return mkd_respath_eval_map(respath);
    
    case FMN_RESTYPE_STRING: { // qualifier=language. Basename should be a 2-letter ISO 631 code.
        return mkd_respath_eval_string(respath);
      }
      
    case FMN_RESTYPE_SOUND: // SOUND live in the same files as INSTRUMENT, we shouldn't see them here at all.
    case FMN_RESTYPE_INSTRUMENT: { // From basename.
        return mkd_respath_eval_instrument(respath);
      }
      
  }
  return 0;
}

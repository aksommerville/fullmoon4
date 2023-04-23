#include "mkd_internal.h"

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
 
static int mkd_restype_eval(const char *src,int srcc) {
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
  
  switch (respath->restype) {
    case FMN_RESTYPE_SONG: break; // no qualifier
    case FMN_RESTYPE_TILEPROPS: break; // no qualifier
    case FMN_RESTYPE_SPRITE: break; // no qualifier
    
    case FMN_RESTYPE_IMAGE: break; // qualifier TODO (format and resolution)
    
    case FMN_RESTYPE_MAP: break; // qualifier TODO (demo vs full)
    
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

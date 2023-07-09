#include "datan_internal.h"

/* Resource type names.
 */
 
const char *fmn_restype_repr(uint16_t type) {
  switch (type) {
    #define _(tag) case FMN_RESTYPE_##tag: return #tag;
    FMN_FOR_EACH_RESTYPE
    #undef _
  }
  return "???";
}

uint16_t fmn_restype_eval(const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char norm[32];
  if (srcc>sizeof(norm)) return 0;
  int i=srcc; while (i-->0) {
    if ((src[i]>='a')&&(src[i]<='z')) norm[i]=src[i]-0x20;
    else norm[i]=src[i];
  }
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(norm,#tag,srcc)) return FMN_RESTYPE_##tag;
  FMN_FOR_EACH_RESTYPE
  #undef _
  return 0;
}

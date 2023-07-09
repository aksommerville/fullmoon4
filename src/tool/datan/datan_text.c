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

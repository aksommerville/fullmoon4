#include "mkd_internal.h"
#include "opt/png/png.h"

/* Compile image.
 */
 
int mkd_compile_image(struct mkd_respath *respath) {
  //TODO Opportunity to optimize PNG, or reformat per platform, etc.
  if (sr_encode_raw(&mkd.dst,mkd.src,mkd.srcc)<0) return -1;
  return 0;
}

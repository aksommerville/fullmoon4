#include "datan_internal.h"

/* Validate tileprops.
 */
 
int datan_tileprops_validate_serial(uint16_t qualifier,uint32_t id,const void *v,int c) {
  if (c!=256) {
    fprintf(stderr,"%s:tileprops:%d(%d): Expected length 256, found %d.\n",datan.arpath,id,qualifier,c);
    return -2;
  }
  const uint8_t *cellphysics=v;
  if (cellphysics[0x0f]!=FMN_CELLPHYSICS_UNSHOVELLABLE) {
    fprintf(stderr,"%s:tileprops:%d(%d): Tile 0x0f must be UNSHOVELLABLE (%d), found %d.\n",datan.arpath,id,qualifier,FMN_CELLPHYSICS_UNSHOVELLABLE,cellphysics[0x0f]);
    return -2;
  }
  return 0;
}

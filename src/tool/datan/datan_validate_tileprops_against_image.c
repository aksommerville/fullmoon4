#include "datan_internal.h"

/* Every tileprops resource, there must be an image of the same ID.
 * Could possibly make some assertions on the dimensions of that image?
 */
 
static int datan_validate_tileprops_1(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  const void *img=0;
  int imgc=fmn_datafile_get_any(&img,datan.datafile,FMN_RESTYPE_IMAGE,id);
  if (imgc<=0) {
    fprintf(stderr,"%s:tileprops:%d(%d): image:%d not found. What is this tileprops for?\n",datan.arpath,id,qualifier,id);
    return -2;
  }
  return 0;
}
 
int datan_validate_tileprops_against_image() {
  return fmn_datafile_for_each_of_type(datan.datafile,FMN_RESTYPE_TILEPROPS,datan_validate_tileprops_1,0);
}

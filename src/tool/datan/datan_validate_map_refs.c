#include "datan_internal.h"

/* Any map with a nonzero REF command, its value must be unique across the archive.
 */
 
int datan_validate_map_refs() {
  uint16_t mapid_by_ref[256]={0};
  struct datan_res *res=datan.resv;
  int i=datan.resc;
  int result=0;
  for (;i-->0;res++) {
    if (res->type<FMN_RESTYPE_MAP) continue;
    if (res->type>FMN_RESTYPE_MAP) break;
    struct datan_map *map=res->obj;
    if (!map->ref) continue; // zero is normal and needn't be unique
    if (mapid_by_ref[map->ref]) {
      fprintf(stderr,
        "%s:map:%d(%d): REF %d conflict against map:%d\n",
        datan.arpath,res->id,res->qualifier,map->ref,mapid_by_ref[map->ref]
      );
      result=-1;
    } else {
      mapid_by_ref[map->ref]=res->id;
    }
  }
  return result;
}

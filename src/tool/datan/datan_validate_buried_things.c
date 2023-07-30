#include "datan_internal.h"

/* Cellphysics.
 */
 
static int ok_cellphysics_for_burying(uint8_t v) {
  switch (v) {
    case FMN_CELLPHYSICS_VACANT:
      return 1;
  }
  return 0;
}

/* Single map.
 */
 
static int datan_validate_buried_things_cmd(uint8_t opcode,const uint8_t *v,int c,void *userdata) {
  struct datan_map *map=userdata;
  switch (opcode) {
    case 0x62: { // (u8 cellp,u16 gsbit,u8 itemid) BURIED_TREASURE
        uint8_t x=v[0]%FMN_COLC;
        uint8_t y=v[0]/FMN_COLC;
        if (y>=FMN_ROWC) {
          fprintf(stderr,"%s:map:%d(%d): Invalid position %d,%d for BURIED_TREASURE\n",datan.arpath,map->id,map->qualifier,x,y);
          return -2;
        }
        uint8_t tileid=map->v[y*FMN_COLC+x];
        uint8_t cellphysics=map->cellphysics[tileid];
        if (!ok_cellphysics_for_burying(cellphysics)) {
          fprintf(stderr,
            "%s:map:%d(%d): BURIED_TREASURE at (%d,%d), tile 0x%02x, cellphysics %d. Not shovellable.\n",
            datan.arpath,map->id,map->qualifier,x,y,tileid,cellphysics
          );
          return -2;
        }
      } break;
    case 0x81: { // (u8 cellp,u16 gsbit,u16 mapid,u8 dstp) BURIED_DOOR
        uint8_t x=v[0]%FMN_COLC;
        uint8_t y=v[0]/FMN_COLC;
        if (y>=FMN_ROWC) {
          fprintf(stderr,"%s:map:%d(%d): Invalid position %d,%d for BURIED_DOOR\n",datan.arpath,map->id,map->qualifier,x,y);
          return -2;
        }
        uint8_t tileid=map->v[y*FMN_COLC+x];
        uint8_t cellphysics=map->cellphysics[tileid];
        if (!ok_cellphysics_for_burying(cellphysics)) {
          fprintf(stderr,
            "%s:map:%d(%d): BURIED_DOOR at (%d,%d), tile 0x%02x, cellphysics %d. Not shovellable.\n",
            datan.arpath,map->id,map->qualifier,x,y,tileid,cellphysics
          );
          return -2;
        }
      } break;
  }
  return 0;
}
 
static int datan_validate_buried_things_1(struct datan_map *map) {
  if (!map->cellphysics) return 0;
  return datan_map_for_each_command(map,datan_validate_buried_things_cmd,map);
}

/* Assert that every BURIED_DOOR and BURIED_TREASURE command is located on a SHOVELLABLE tile.
 */
 
int datan_validate_buried_things() {
  struct datan_res *res=datan.resv;
  int i=datan.resc;
  for (;i-->0;res++) {
    if (res->type<FMN_RESTYPE_MAP) continue;
    if (res->type>FMN_RESTYPE_MAP) break;
    int err=datan_validate_buried_things_1(res->obj);
    if (err<0) return err;
  }
  return 0;
}

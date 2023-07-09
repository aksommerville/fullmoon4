#include "datan_internal.h"

/* Delete.
 */
 
void datan_map_del(struct datan_map *map) {
  if (!map) return;
  if (map->spawnv) free(map->spawnv);
  free(map);
}

/* New.
 */
 
struct datan_map *datan_map_new(uint16_t qualifier,uint32_t id,const void *src,int srcc) {
  if (srcc<FMN_COLC*FMN_ROWC) return 0;
  struct datan_map *map=calloc(1,sizeof(struct datan_map));
  if (!map) return 0;
  map->qualifier=qualifier;
  map->id=id;
  map->v=src;
  map->addl=src+FMN_COLC*FMN_ROWC;
  map->addlc=srcc-FMN_COLC*FMN_ROWC;
  return map;
}

/* Add spawn point.
 */
 
static int datan_map_add_spawn(struct datan_map *map,uint8_t x,uint8_t y,uint16_t spriteid,uint8_t arg0,uint8_t arg1,uint8_t arg2) {
  if (map->spawnc>=map->spawna) {
    int na=map->spawna+4;
    if (na>INT_MAX/sizeof(struct datan_map_spawn)) return -1;
    void *nv=realloc(map->spawnv,sizeof(struct datan_map_spawn)*na);
    if (!nv) return -1;
    map->spawnv=nv;
    map->spawna=na;
  }
  struct datan_map_spawn *spawn=map->spawnv+map->spawnc++;
  spawn->x=x;
  spawn->y=y;
  spawn->spriteid=spriteid;
  spawn->arg0=arg0;
  spawn->arg1=arg1;
  spawn->arg2=arg2;
  spawn->sprite=0;
  return 0;
}

/* Validate.
 */
 
static int datan_map_validate_1(uint8_t opcode,const uint8_t *v,int c,void *userdata) {
  struct datan_map *map=userdata;
  switch (opcode) {
   
    #define ONCE8(fldname) { \
      if (map->fldname) { \
        fprintf(stderr,"%s:map:%d(%d): Duplicate values for map field '%s' (%d,%d)\n",datan.arpath,map->id,map->qualifier,#fldname,map->fldname,v[0]); \
        return -2; \
      } \
      map->fldname=v[0]; \
    }
    case 0x20: ONCE8(songid) break;
    case 0x21: ONCE8(imageid) break;
    case 0x22: ONCE8(saveto) break;
    case 0x23: ONCE8(winddir) break;
    case 0x24: ONCE8(flags) break;
    #undef ONCE8
   
    #define ONCE16(fldname) { \
      if (map->fldname) { \
        fprintf(stderr,"%s:map:%d(%d): Duplicate values for map field '%s' (%d,%d)\n",datan.arpath,map->id,map->qualifier,#fldname,map->fldname,(v[0]<<8)|v[1]); \
        return -2; \
      } \
      map->fldname=(v[0]<<8)|v[1]; \
    }
    case 0x40: ONCE16(neighborw) break;
    case 0x41: ONCE16(neighbore) break;
    case 0x42: ONCE16(neighborn) break;
    case 0x43: ONCE16(neighbors) break;
    #undef ONCE16
    
    
    case 0x45: { // HERO
        map->herox=v[0]%FMN_COLC;
        map->heroy=v[0]/FMN_COLC;
        if (map->heroy>=FMN_ROWC) {
          fprintf(stderr,"%s:map:%d(%d): Invalid hero position %d,%d\n",datan.arpath,map->id,map->qualifier,map->herox,map->heroy);
          return -2;
        }
        map->spellid=v[1];
      } break;
      
    case 0x80: { // (u8 cellp,u16 spriteid,u8 arg0,u8 arg1,u8 arg2) SPRITE
        uint8_t x=v[0]%FMN_COLC;
        uint8_t y=v[0]/FMN_COLC;
        uint16_t spriteid=(v[1]<<8)|v[2];
        if (y>=FMN_ROWC) {
          fprintf(stderr,"%s:map:%d(%d): Invalid spawn point %d,%d\n",datan.arpath,map->id,map->qualifier,x,y);
          return -2;
        }
        if (datan_map_add_spawn(map,x,y,spriteid,v[3],v[4],v[5])<0) return -1;
      } break;

    // Not extracting these automatically. Preserve for documentation, and so there's no "unknown command" warning.
    case 0x44: break; // (u8 cellp,u8 0x80:to 0x40:from 0x3f:state) TRANSMOGRIFY
    case 0x60: break; // (u8 cellp,u16 mapid,u8 dstcellp) DOOR
    case 0x61: break; // (u8 cellp,u24 bits) SKETCH
    case 0x62: break; // (u8 cellp,u16 gsbit,u8 itemid) BURIED_TREASURE
    case 0x63: break; // (u8 evid,u16 cbid,u8 param) CALLBACK
    case 0x64: break; // (u8 cellp,u16 eventid,u8 unused) EVENT_TRIGGER
    case 0x65: break; // (u16 gsbit_horz,u16 gsbit_vert) FACEDIR
    case 0x81: break; // (u8 cellp,u16 gsbit,u16 mapid,u8 dstp) BURIED_DOOR

    default: {
        fprintf(stderr,"%s:map:%d(%d):WARNING: Unknown command 0x%02x.\n",datan.arpath,map->id,map->qualifier,opcode);
      } break;
  }
  return 0;
}
 
int datan_map_validate(struct datan_map *map) {
  int err=datan_map_for_each_command(map,datan_map_validate_1,map);
  if (err<0) return err;
  return 0;
}

/* Iterate commands.
 */
 
int datan_map_for_each_command(
  struct datan_map *map,
  int (*cb)(uint8_t opcode,const uint8_t *v,int c,void *userdata),
  void *userdata
) {
  int addlp=0,err;
  while (addlp<map->addlc) {
    uint8_t opcode=map->addl[addlp++];
    if (!opcode) break;
    int paylen;
         if (opcode<0x20) paylen=0;
    else if (opcode<0x40) paylen=1;
    else if (opcode<0x60) paylen=2;
    else if (opcode<0x80) paylen=4;
    else if (opcode<0xa0) paylen=6;
    else if (opcode<0xc0) paylen=8;
    else if (opcode<0xe0) {
      if (addlp>=map->addlc) {
        fprintf(stderr,"%s:map:%d(%d): Expected length for opcode 0x%02x, found EOF.\n",datan.arpath,map->id,map->qualifier,opcode);
        return -2;
      }
      paylen=map->addl[addlp++];
    } else {
      fprintf(stderr,"%s:map:%d(%d): Unknown opcode 0x%02x at %d/%d\n",datan.arpath,map->id,map->qualifier,opcode,addlp-1,map->addlc);
      return -2;
    }
    if (addlp>map->addlc-paylen) {
      fprintf(stderr,"%s:map:%d(%d): Command 0x%02x at %d/%d overruns EOF.\n",datan.arpath,map->id,map->qualifier,opcode,addlp-1,map->addlc);
      return -2;
    }
    if (err=cb(opcode,map->addl+addlp,paylen,userdata)) return err;
    addlp+=paylen;
  }
  return 0;
}

/* Examine cells.
 */

int datan_map_cell_is_solid(const struct datan_map *map,int x,int y) {
  switch (datan_map_get_cellphysics(map,x,y)) {
    case FMN_CELLPHYSICS_SOLID:
    case FMN_CELLPHYSICS_UNCHALKABLE:
    case FMN_CELLPHYSICS_SAP:
    case FMN_CELLPHYSICS_SAP_NOCHALK:
    case FMN_CELLPHYSICS_REVELABLE:
      return 1;
  }
  return 0;
}

uint8_t datan_map_get_cellphysics(const struct datan_map *map,int x,int y) {
  if ((x<0)||(y<0)||(x>=FMN_COLC)||(y>=FMN_ROWC)) return 0;
  if (!map->cellphysics) return 0;
  return map->cellphysics[map->v[y*FMN_COLC+x]];
}

#include "datan_internal.h"

/* firewall
 */
 
static int datan_validate_spawn_firewall(struct datan_map *map,struct datan_map_spawn *spawn,struct datan_sprite *sprite) {
  
  if (
    ((spawn->x==0)||(spawn->x==FMN_COLC-1))&&
    ((spawn->y==0)||(spawn->y==FMN_ROWC-1))
  ) {
    fprintf(stderr,"%s:map:%d(%d): Firewall must not spawn in corner.\n",datan.arpath,map->id,map->qualifier);
    return -2;
  }
  
  uint8_t dx=0,dy=0;
  if ((spawn->x==0)||(spawn->x==FMN_COLC-1)) dy=1;
  else if ((spawn->y==0)||(spawn->y==FMN_ROWC-1)) dx=1;
  else {
    fprintf(stderr,"%s:map:%d(%d): Firewall must spawn on edge. Found at %d,%d.\n",datan.arpath,map->id,map->qualifier,spawn->x,spawn->y);
    return -2;
  }
  
  int xlo=spawn->x,xhi=spawn->x,ylo=spawn->y,yhi=spawn->y;
  int look=0,hiok=0;
  while (1) {
    if ((xlo<0)||(xhi>=FMN_COLC)||(ylo<0)||(yhi>=FMN_ROWC)) {
      fprintf(stderr,"%s:map:%d(%d): Firewall must be bounded by solid tiles.\n",datan.arpath,map->id,map->qualifier);
      return -2;
    }
    if (!look) {
      if (datan_map_cell_is_solid(map,xlo,ylo)) look=1;
      else { xlo-=dx; ylo-=dy; }
    }
    if (!hiok) {
      if (datan_map_cell_is_solid(map,xhi,yhi)) hiok=1;
      else { xhi+=dx; yhi+=dy; }
    }
    if (look&&hiok) break;
  }
  int openc=(xhi-xlo)+(yhi-ylo)-1;
  if (openc<2) {
    fprintf(stderr,"%s:map:%d(%d): Firewall at %d,%d requires at least 2 vacant cells to expand into.\n",datan.arpath,map->id,map->qualifier,spawn->x,spawn->y);
    return -2;
  }
  
  return 0;
}

/* Single map.
 */
 
static int datan_validate_spawn_points_1(struct datan_map *map) {

  // We're also using this hook for general linkage.
  // Load cellphysics:
  int cellphysicsc=fmn_datafile_get_qualified(&map->cellphysics,datan.datafile,FMN_RESTYPE_TILEPROPS,0,map->imageid);
  if (cellphysicsc<=0) {
    if (map->imageid) {
      fprintf(stderr,"%s:map:%d(%d): Image ID unset\n",datan.arpath,map->id,map->qualifier);
    } else {
      fprintf(stderr,"%s:map:%d(%d): Image ID unset\n",datan.arpath,map->id,map->qualifier);
    }
    return -2;
  }
  if (cellphysicsc<256) {
    fprintf(stderr,"%s:tileprops:%d(0): Expected 256 bytes, found %d.\n",datan.arpath,map->imageid,cellphysicsc);
    return -2;
  }

  // And finally, as promised, the spawn points:
  struct datan_map_spawn *spawn=map->spawnv;
  int i=map->spawnc,err;
  for (;i-->0;spawn++) {
    if (!(spawn->sprite=datan_res_get(FMN_RESTYPE_SPRITE,0,spawn->spriteid))) {
      fprintf(stderr,"%s:map:%d(%d): Spawn point for sprite %d at (%d,%d), sprite not found.\n",datan.arpath,map->id,map->qualifier,spawn->spriteid,spawn->x,spawn->y);
      return -2;
    }
    switch (spawn->sprite->controller) {
    
      case FMN_SPRCTL_firewall: if ((err=datan_validate_spawn_firewall(map,spawn,spawn->sprite))<0) return err; break;
      
    }
  }
  return 0;
}

/* Validate sprite spawn points in each map.
 * Populate (spawnv->sprite) in each, and fail if sprite not found.
 * Can add specific controller knowledge here.
 * For now I'm only looking at firewall.
 */
 
int datan_validate_spawn_points() {
  struct datan_res *res=datan.resv;
  int i=datan.resc;
  while (i&&(res->type<FMN_RESTYPE_MAP)) { res++; i--; }
  while (i&&(res->type==FMN_RESTYPE_MAP)) {
    int err=datan_validate_spawn_points_1(res->obj);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s:map:%d(%d):%s: Unspecified error.\n",datan.arpath,res->id,res->qualifier,__func__);
      return -2;
    }
    res++;
    i--;
  }
  return 0;
}

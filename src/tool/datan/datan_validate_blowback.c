#include "datan_internal.h"

/* Validate one map.
 */
 
struct neighbors {
  int w,e,n,s;
};

static int datan_validate_blowback_cmd(uint8_t opcode,const uint8_t *v,int c,void *userdata) {
  struct neighbors *neighbors=userdata;
  switch (opcode) {
    case 0x80: {
        uint8_t x=v[0]%FMN_COLC;
        uint8_t y=v[0]/FMN_ROWC;
        int *neighbor=0;
        if (!x) neighbor=&neighbors->w;
        else if (!y) neighbor=&neighbors->n;
        else if (x==FMN_COLC-1) neighbor=&neighbors->e;
        else if (y==FMN_ROWC-1) neighbor=&neighbors->s;
        else return 0;
        if (!*neighbor) return 0; // already not checking that neighbor; carry on
        struct datan_sprite *sprite=datan_res_get(FMN_RESTYPE_SPRITE,0,(v[1]<<8)|v[2]);
        if (!sprite) return 0;
        if (sprite->controller!=FMN_SPRCTL_firewall) return 0;
        // ok! the edge is firewalled. don't check it.
        *neighbor=0;
      } break;
  }
  return 0;
}
 
static int datan_validate_blowback_1(struct datan_map *map) {

  // Blowback enabled? Great, nothing to check.
  if (map->flags&0x04) return 0;
  
  // Decide which edges matter.
  // Important edges have no cardinal neighbor, and no firewall resting on them.
  // If a map has a firewall, but a separate open portion not covered by the firewall, we do NOT check for that open portion.
  // (that's a deficiency in this validation, but it's complicated, so just let's worry about that at map editing time, manually).
  struct neighbors neighbors={
    .w=!map->neighborw,
    .e=!map->neighbore,
    .n=!map->neighborn,
    .s=!map->neighbors,
  };
  datan_map_for_each_command(map,datan_validate_blowback_cmd,&neighbors);
  
  if (
    (neighbors.w&&!datan_map_rect_entirely_solid(map,0,         0,         1,       FMN_ROWC))||
    (neighbors.e&&!datan_map_rect_entirely_solid(map,FMN_COLC-1,0,         1,       FMN_ROWC))||
    (neighbors.n&&!datan_map_rect_entirely_solid(map,0,         0,         FMN_COLC,1       ))||
    (neighbors.s&&!datan_map_rect_entirely_solid(map,0,         FMN_ROWC-1,FMN_COLC,1       ))||
  0) {
    fprintf(stderr,
      "%s:map:%d(%d): Map has an unpopulated cardinal edge. Must block with solids or enable blowback.\n",
      datan.arpath,map->id,map->qualifier
    );
    return -2;
  }
  
  return 0;
}

/* Maps with an open edge and no neighbor or firewall, must have blowback enabled.
 */
 
int datan_validate_blowback() {
  const struct datan_res *res=datan.resv;
  int i=datan.resc;
  for (;i-->0;res++) {
    if (res->type<FMN_RESTYPE_MAP) continue;
    if (res->type>FMN_RESTYPE_MAP) break;
    int err=datan_validate_blowback_1(res->obj);
    if (err<0) return err;
  }
  return 0;
}

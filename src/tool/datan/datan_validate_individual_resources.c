#include "datan_internal.h"

/* Live resource registry.
 */
 
void datan_res_clear() {
  struct datan_res *res=datan.resv;
  int i=datan.resc;
  for (;i-->0;res++) {
    if (!res->obj) continue;
    res->del(res->obj);
  }
  datan.resc=0;
}

int datan_res_search(uint16_t type,uint16_t qualifier,uint32_t id) {
  int lo=0,hi=datan.resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct datan_res *res=datan.resv+ck;
         if (type<res->type) hi=ck;
    else if (type>res->type) lo=ck+1;
    else if (qualifier<res->qualifier) hi=ck;
    else if (qualifier>res->qualifier) lo=ck+1;
    else if (id<res->id) hi=ck;
    else if (id>res->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

struct datan_res *datan_res_insert(int p,uint16_t type,uint16_t qualifier,uint32_t id) {
  if ((p<0)||(p>datan.resc)) return 0;
  // don't validate order, there's too much of it.
  if (datan.resc>=datan.resa) {
    int na=datan.resa+256;
    if (na>INT_MAX/sizeof(struct datan_res)) return 0;
    void *nv=realloc(datan.resv,sizeof(struct datan_res)*na);
    if (!nv) return 0;
    datan.resv=nv;
    datan.resa=na;
  }
  struct datan_res *res=datan.resv+p;
  memmove(res+1,res,sizeof(struct datan_res)*(datan.resc-p));
  memset(res,0,sizeof(struct datan_res));
  datan.resc++;
  res->type=type;
  res->qualifier=qualifier;
  res->id=id;
  return res;
}

int datan_res_add(uint16_t type,uint16_t qualifier,uint32_t id,void *obj,void *del) {
  int p=datan_res_search(type,qualifier,id);
  if (p>=0) return -1;
  struct datan_res *res=datan_res_insert(-p-1,type,qualifier,id);
  if (!res) return -1;
  res->obj=obj;
  res->del=del;
  return 0;
}

void *datan_res_get(uint16_t type,uint16_t qualifier,uint32_t id) {
  int p=datan_res_search(type,qualifier,id);
  if (p<0) return 0;
  return datan.resv[p].obj;
}

/* Dispatch per type.
 * If there's an associated live resource, decode and store it at this time.
 * If there's validation to run against the encoded resource, do that too.
 * Business logic does not belong here; just call out to whoever does it.
 */
 
static int validate_image(uint16_t qualifier,uint32_t id,const void *v,int c) {
  //TODO image validation. Surely plenty we can check.
  return 0;
}
 
static int validate_song(uint16_t qualifier,uint32_t id,const void *v,int c) {
  int err=datan_song_validate_serial(qualifier,id,v,c);
  if (err<0)  return err;
  return 0;
}
 
static int validate_map(uint16_t qualifier,uint32_t id,const void *v,int c) {
  struct datan_map *map=datan_map_new(qualifier,id,v,c);
  if (!map) return -1;
  int err=datan_map_validate(map);
  if (err<0) {
    datan_map_del(map);
    return err;
  }
  if (datan_res_add(FMN_RESTYPE_MAP,qualifier,id,map,datan_map_del)<0) {
    datan_map_del(map);
    return -1;
  }
  return 0;
}
 
static int validate_tileprops(uint16_t qualifier,uint32_t id,const void *v,int c) {
  int err=datan_tileprops_validate_serial(qualifier,id,v,c);
  if (err<0) return err;
  return 0;
}
 
static int validate_sprite(uint16_t qualifier,uint32_t id,const void *v,int c) {
  struct datan_sprite *sprite=datan_sprite_new(qualifier,id,v,c);
  if (!sprite) return -1;
  int err=datan_sprite_validate(sprite);
  if (err<0) {
    datan_sprite_del(sprite);
    return err;
  }
  if (datan_res_add(FMN_RESTYPE_SPRITE,qualifier,id,sprite,datan_sprite_del)<0) {
    datan_sprite_del(sprite);
    return -1;
  }
  return 0;
}
 
static int validate_string(uint16_t qualifier,uint32_t id,const void *v,int c) {
  int err=datan_string_validate_serial(qualifier,id,v,c);
  if (err<0) return err;
  return 0;
}
 
static int validate_instrument(uint16_t qualifier,uint32_t id,const void *v,int c) {
  //TODO instrument validation. Will be different for each qualifier.
  return 0;
}
 
static int validate_sound(uint16_t qualifier,uint32_t id,const void *v,int c) {
  //TODO sound validation. Will be different for each qualifier.
  return 0;
}

/* Check each resource in the archive.
 * Decode to a more digestible format, and make whatever assertions we can without examining other resources.
 */
 
static int validate_1(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  int err=-1;
  switch (type) {
    case FMN_RESTYPE_IMAGE: err=validate_image(qualifier,id,v,c); break;
    case FMN_RESTYPE_SONG: err=validate_song(qualifier,id,v,c); break;
    case FMN_RESTYPE_MAP: err=validate_map(qualifier,id,v,c); break;
    case FMN_RESTYPE_TILEPROPS: err=validate_tileprops(qualifier,id,v,c); break;
    case FMN_RESTYPE_SPRITE: err=validate_sprite(qualifier,id,v,c); break;
    case FMN_RESTYPE_STRING: err=validate_string(qualifier,id,v,c); break;
    case FMN_RESTYPE_INSTRUMENT: err=validate_instrument(qualifier,id,v,c); break;
    case FMN_RESTYPE_SOUND: err=validate_sound(qualifier,id,v,c); break;
    default: {
        fprintf(stderr,"%s: Unknown resource type %d (qualifier=%d id=%d c=%d)\n",datan.arpath,type,qualifier,id,c);
        return -2;
      }
  }
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s:%s:%d(%d): Unspecified validation error, %d bytes serial.\n",datan.arpath,fmn_restype_repr(type),id,qualifier,c);
    return -2;
  }
  return 0;
}
 
int datan_validate_individual_resources() {
  datan_res_clear();
  int err=fmn_datafile_for_each(datan.datafile,validate_1,0);
  if (err<0) return err;
  if ((err=datan_sprites_acquire_argtype())<0) return err;
  return 0;
}

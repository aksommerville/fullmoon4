#include "fmstore_internal.h"

/* Plant list primitives.
 */

int fmstore_plant_search(const struct fmstore *fmstore,uint16_t mapid,uint8_t x,uint8_t y) {
  int lo=0,hi=fmstore->plantc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct fmstore_plant *q=fmstore->plantv+ck;
         if (mapid<q->mapid) hi=ck;
    else if (mapid>q->mapid) lo=ck+1;
    else if (y<q->plant.y) hi=ck;
    else if (y>q->plant.y) lo=ck+1;
    else if (x<q->plant.x) hi=ck;
    else if (x>q->plant.x) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int fmstore_plants_for_map(struct fmstore_plant **v,const struct fmstore *fmstore,uint16_t mapid) {
  if (mapid==0xffff) { *v=fmstore->plantv+fmstore->plantc; return 0; }
  int pa=fmstore_plant_search(fmstore,mapid,0,0);
  if (pa<0) pa=-pa-1;
  int pz=fmstore_plant_search(fmstore,mapid+1,0,0);
  if (pz<0) pz=-pz-1;
  if (pz<pa) pz=pa;
  *v=fmstore->plantv+pa;
  return pz-pa;
}

struct fmstore_plant *fmstore_plant_insert(struct fmstore *fmstore,int p,uint16_t mapid,uint8_t x,uint8_t y) {
  if (mapid==0xffff) return 0;
  if ((p<0)||(p>fmstore->plantc)) return 0;
  if (fmstore->plantc>=fmstore->planta) {
    int na=fmstore->planta+32;
    if (na>INT_MAX/sizeof(struct fmstore_plant)) return 0;
    void *nv=realloc(fmstore->plantv,sizeof(struct fmstore_plant)*na);
    if (!nv) return 0;
    fmstore->plantv=nv;
    fmstore->planta=na;
  }
  struct fmstore_plant *plant=fmstore->plantv+p;
  memmove(plant+1,plant,sizeof(struct fmstore_plant)*(fmstore->plantc-p));
  fmstore->plantc++;
  memset(plant,0,sizeof(struct fmstore_plant));
  plant->mapid=mapid;
  plant->plant.x=x;
  plant->plant.y=y;
  return plant;
}

struct fmstore_plant *fmstore_plant_upsert(struct fmstore *fmstore,uint16_t mapid,uint8_t x,uint8_t y) {
  int p=fmstore_plant_search(fmstore,mapid,x,y);
  if (p<0) return fmstore_plant_insert(fmstore,-p-1,mapid,x,y);
  return fmstore->plantv+p;
}

/* Sketch list primitives.
 */

int fmstore_sketch_search(const struct fmstore *fmstore,uint16_t mapid,uint8_t x,uint8_t y) {
  int lo=0,hi=fmstore->sketchc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct fmstore_sketch *sketch=fmstore->sketchv+ck;
         if (mapid<sketch->mapid) hi=ck;
    else if (mapid>sketch->mapid) lo=ck+1;
    else if (y<sketch->sketch.y) hi=ck;
    else if (y>sketch->sketch.y) lo=ck+1;
    else if (x<sketch->sketch.x) hi=ck;
    else if (x>sketch->sketch.x) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int fmstore_sketches_for_map(struct fmstore_sketch **v,const struct fmstore *fmstore,uint16_t mapid) {
  if (mapid==0xffff) { *v=fmstore->sketchv+fmstore->sketchc; return 0; }
  int pa=fmstore_sketch_search(fmstore,mapid,0,0);
  if (pa<0) pa=-pa-1;
  int pz=fmstore_sketch_search(fmstore,mapid+1,0,0);
  if (pz<0) pz=-pz-1;
  if (pz<pa) pz=pa;
  *v=fmstore->sketchv+pa;
  return pz-pa;
}

struct fmstore_sketch *fmstore_sketch_insert(struct fmstore *fmstore,int p,uint16_t mapid,uint8_t x,uint8_t y) {
  if (mapid==0xffff) return 0;
  if ((p<0)||(p>fmstore->sketchc)) return 0;
  if (fmstore->sketchc>=fmstore->sketcha) {
    int na=fmstore->sketcha+32;
    if (na>INT_MAX/sizeof(struct fmstore_sketch)) return 0;
    void *nv=realloc(fmstore->sketchv,sizeof(struct fmstore_sketch)*na);
    if (!nv) return 0;
    fmstore->sketchv=nv;
    fmstore->sketcha=na;
  }
  struct fmstore_sketch *sketch=fmstore->sketchv+p;
  memmove(sketch+1,sketch,sizeof(struct fmstore_sketch)*(fmstore->sketchc-p));
  fmstore->sketchc++;
  memset(sketch,0,sizeof(struct fmstore_sketch));
  sketch->mapid=mapid;
  sketch->sketch.x=x;
  sketch->sketch.y=y;
  return sketch;
}

struct fmstore_sketch *fmstore_sketch_upsert(struct fmstore *fmstore,uint16_t mapid,uint8_t x,uint8_t y) {
  int p=fmstore_sketch_search(fmstore,mapid,x,y);
  if (p<0) return fmstore_sketch_insert(fmstore,-p-1,mapid,x,y);
  return fmstore->sketchv+p;
}

/* Drop entries for the given map with (visited) unset.
 */
 
static void fmstore_drop_unvisited_plants(struct fmstore *fmstore,uint16_t mapid) {
  int p=fmstore_plant_search(fmstore,mapid+1,0,0);
  if (p<0) p=-p-1;
  struct fmstore_plant *v=fmstore->plantv+p;
  while (p>0) {
    p--;
    v--;
    if (v->mapid!=mapid) return;
    int rmc=0;
    while ((p>=0)&&(v->mapid==mapid)&&!v->visited) { p--; v--; rmc++; }
    if (rmc) {
      p++; v++; // if we decided to remove something, we'll have stepped one too far.
      fmstore->plantc-=rmc;
      memmove(v,v+rmc,sizeof(struct fmstore_plant)*(fmstore->plantc-p));
    }
  }
}

static void fmstore_drop_unvisited_sketches(struct fmstore *fmstore,uint16_t mapid) {
  int p=fmstore_sketch_search(fmstore,mapid+1,0,0);
  if (p<0) p=-p-1;
  struct fmstore_sketch *v=fmstore->sketchv+p;
  while (p>0) {
    p--;
    v--;
    if (v->mapid!=mapid) return;
    int rmc=0;
    while ((p>=0)&&(v->mapid==mapid)&&!v->visited) { p--; v--; rmc++; }
    if (rmc) {
      p++; v++; // if we decided to remove something, we'll have stepped one too far.
      fmstore->sketchc-=rmc;
      memmove(v,v+rmc,sizeof(struct fmstore_sketch)*(fmstore->sketchc-p));
    }
  }
}

/* Find plants for this map and write to globals.
 */
 
void fmstore_write_plants_to_globals(struct fmstore *fmstore,uint16_t mapid) {
  struct fmstore_plant *srcv=0;
  int srcc=fmstore_plants_for_map(&srcv,fmstore,mapid);
  for (;srcc-->0;srcv++) {
    int overwrote=0;
    struct fmn_plant *dstv=fmn_global.plantv;
    int dsti=fmn_global.plantc;
    for (;dsti-->0;dstv++) {
      if ((dstv->x==srcv->plant.x)&&(dstv->y==srcv->plant.y)) {
        *dstv=srcv->plant;
        overwrote=1;
        break;
      }
    }
    if (overwrote) continue;
    if (fmn_global.plantc>=FMN_PLANT_LIMIT) continue;
    dstv=fmn_global.plantv+fmn_global.plantc++;
    *dstv=srcv->plant;
  }
}

/* Find sketches for this map and write to globals.
 */
 
void fmstore_write_sketches_to_globals(struct fmstore *fmstore,uint16_t mapid) {
  struct fmstore_sketch *srcv=0;
  int srcc=fmstore_sketches_for_map(&srcv,fmstore,mapid);
  for (;srcc-->0;srcv++) {
    int overwrote=0;
    struct fmn_sketch *dstv=fmn_global.sketchv;
    int dsti=fmn_global.sketchc;
    for (;dsti-->0;dstv++) {
      if ((dstv->x==srcv->sketch.x)&&(dstv->y==srcv->sketch.y)) {
        *dstv=srcv->sketch;
        overwrote=1;
        break;
      }
    }
    if (overwrote) continue;
    if (fmn_global.sketchc>=FMN_SKETCH_LIMIT) continue;
    dstv=fmn_global.sketchv+fmn_global.sketchc++;
    *dstv=srcv->sketch;
  }
}

/* Examine plants in globals and store as needed.
 */

void fmstore_read_plants_from_globals(struct fmstore *fmstore,uint16_t mapid) {
  struct fmstore_plant *v=0;
  int c=fmstore_plants_for_map(&v,fmstore,mapid);
  for (;c-->0;v++) v->visited=0;
  const struct fmn_plant *src=fmn_global.plantv;
  int srci=fmn_global.plantc;
  for (;srci-->0;src++) {
    // Plants in NONE or DEAD state will be dropped, by skipping here.
    if ((src->state==FMN_PLANT_STATE_NONE)||(src->state==FMN_PLANT_STATE_DEAD)) continue;
    if (!(v=fmstore_plant_upsert(fmstore,mapid,src->x,src->y))) return; // panic!
    v->visited=1; // keep it
    v->plant=*src;
  }
  fmstore_drop_unvisited_plants(fmstore,mapid);
}

/* Examine sketches in globals and store as needed.
 */
 
void fmstore_read_sketches_from_globals(struct fmstore *fmstore,uint16_t mapid) {
  struct fmstore_sketch *v=0;
  int c=fmstore_sketches_for_map(&v,fmstore,mapid);
  for (;c-->0;v++) v->visited=0;
  const struct fmn_sketch *src=fmn_global.sketchv;
  int srci=fmn_global.sketchc;
  for (;srci-->0;src++) {
    // Drop any sketch with zero bits.
    if (!src->bits) continue;
    if (!(v=fmstore_sketch_upsert(fmstore,mapid,src->x,src->y))) return;
    v->visited=1;
    v->sketch=*src;
  }
  fmstore_drop_unvisited_sketches(fmstore,mapid);
}

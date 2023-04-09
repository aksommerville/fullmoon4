#ifndef FMSTORE_INTERNAL_H
#define FMSTORE_INTERNAL_H

#include "app/fmn_platform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct fmstore {
  struct fmstore_plant {
    uint16_t mapid;
    uint16_t visited; // used transiently during sync
    struct fmn_plant plant;
  } *plantv;
  int plantc,planta;
  struct fmstore_sketch {
    uint16_t mapid;
    uint16_t visited; // used transiently during sync
    struct fmn_sketch sketch;
  } *sketchv;
  int sketchc,sketcha;
};

int fmstore_plant_search(const struct fmstore *fmstore,uint16_t mapid,uint8_t x,uint8_t y);
int fmstore_plants_for_map(struct fmstore_plant **v,const struct fmstore *fmstore,uint16_t mapid);
struct fmstore_plant *fmstore_plant_insert(struct fmstore *fmstore,int p,uint16_t mapid,uint8_t x,uint8_t y);
struct fmstore_plant *fmstore_plant_upsert(struct fmstore *fmstore,uint16_t mapid,uint8_t x,uint8_t y);

int fmstore_sketch_search(const struct fmstore *fmstore,uint16_t mapid,uint8_t x,uint8_t y);
int fmstore_sketches_for_map(struct fmstore_sketch **v,const struct fmstore *fmstore,uint16_t mapid);
struct fmstore_sketch *fmstore_sketch_insert(struct fmstore *fmstore,int p,uint16_t mapid,uint8_t x,uint8_t y);
struct fmstore_sketch *fmstore_sketch_upsert(struct fmstore *fmstore,uint16_t mapid,uint8_t x,uint8_t y);

#endif

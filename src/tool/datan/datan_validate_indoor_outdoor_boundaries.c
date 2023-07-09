#include "datan_internal.h"

/* Check from one map.
 */
 
struct doorage_context {
  uint8_t home_indoors;
  struct datan_map *home;
};

static int datan_validate_doorage_cb(uint8_t opcode,const uint8_t *v,int c,void *userdata) {
  struct doorage_context *ctx=userdata;
  switch (opcode) {
    // Not checking solid edges and firewall.
    // My feeling is that even when neighbors are practically unreachable, if they are technically neighbors they should share indoorness.
    #define CARDINAL(opcode,dirname) \
      case opcode: { \
          struct datan_map *neighbor=datan_res_get(FMN_RESTYPE_MAP,ctx->home->qualifier,(v[0]<<8)|v[1]); \
          if (!neighbor) return -1; \
          uint8_t neighbor_indoors=neighbor->flags&0x02; \
          if (neighbor_indoors!=ctx->home_indoors) { \
            fprintf(stderr, \
              "%s:map:%d(%d): %s neighbor (%d) is %s but I am %s -- cardinal edges shouldn't change indoorness.\n", \
              datan.arpath,ctx->home->id,ctx->home->qualifier,dirname,(v[0]<<8)|v[1], \
              neighbor_indoors?"indoors":"outdoors", \
              ctx->home_indoors?"indoors":"outdoors" \
            ); \
            return -2; \
          } \
        } break;
    CARDINAL(0x40,"west")
    CARDINAL(0x41,"east")
    CARDINAL(0x42,"north")
    CARDINAL(0x43,"south")
    #undef CARDINAL
    // Door commands don't matter; they are allowed to change indoorness.
  }
  return 0;
}
 
static int datan_validate_doorage_1(struct datan_map *map) {
  struct doorage_context ctx={
    .home_indoors=map->flags&0x02,
    .home=map,
  };
  return datan_map_for_each_command(map,datan_validate_doorage_cb,&ctx);
}

/* A change to the "indoors" flag can only happen via doors.
 * Fail if we find a case of cardinal neighbors with mismatched indoorness.
 */
 
int datan_validate_indoor_outdoor_boundaries() {
  const struct datan_res *res=datan.resv;
  int i=datan.resc;
  for (;i-->0;res++) {
    if (res->type<FMN_RESTYPE_MAP) continue;
    if (res->type>FMN_RESTYPE_MAP) break;
    int err=datan_validate_doorage_1(res->obj);
    if (err<0) return err;
  }
  return 0;
}

#include "datan_internal.h"

/* Context.
 */
 
struct datan_save_context {
  struct datan_save_entry {
    struct datan_map *map; // WEAK, optional (null means this id is unused)
    int effective_spellid; // If (map->spellid,map->saveto) unset. Zero if unvisited.
  } *v; // indexed by id
  int c,a;
  uint8_t TEMP_spellid; // quick handoff, prevents need for an extra callback context
};

static void datan_save_context_cleanup(struct datan_save_context *ctx) {
  if (ctx->v) free(ctx->v);
}

/* Add map.
 */
 
static int datan_save_add_map(struct datan_save_context *ctx,uint32_t id,struct datan_map *map) {
  if (!map) return 0;
  if (id>0x0000ffff) {
    fprintf(stderr,"%s:map:%d(0): Map ID must be less than 65536\n",datan.arpath,id);
    return -2;
  }
  if (id>=ctx->c) {
    if (id>=ctx->a) {
      int na=id+1;
      if (na<INT_MAX-128) na=(na+128)&~127;
      if (na>INT_MAX/sizeof(struct datan_save_entry)) return -1;
      void *nv=realloc(ctx->v,sizeof(struct datan_save_entry)*na);
      if (!nv) return -1;
      ctx->v=nv;
      ctx->a=na;
    }
    memset(ctx->v+ctx->c,0,sizeof(struct datan_save_entry)*(id+1-ctx->c));
    ctx->c=id+1;
  }
  struct datan_save_entry *entry=ctx->v+id;
  entry->map=map;
  return 0;
}

/* Build up initial context with all maps.
 */
 
static int datan_save_context_init(struct datan_save_context *ctx) {
  const struct datan_res *res=datan.resv;
  int i=datan.resc,err;
  for (;i-->0;res++) {
    if (res->type!=FMN_RESTYPE_MAP) continue;
    if ((err=datan_save_add_map(ctx,res->id,res->obj))<0) return err;
  }
  return 0;
}

/* Walk from one map.
 * (spellid==0) for the start of the walk. This means (entry) must have an explicit spell.
 */
 
static int datan_save_walk(struct datan_save_context *ctx,struct datan_save_entry *entry,uint8_t spellid);
 
static int datan_save_walk_neighbor_cb(struct datan_map *from_map,uint16_t mapid,void *userdata) {
  struct datan_save_context *ctx=userdata;
  uint8_t spellid=ctx->TEMP_spellid;
  if (!mapid) return 0;
  if (mapid>=ctx->c) return -1;
  int err=datan_save_walk(ctx,ctx->v+mapid,spellid);
  return err;
}
 
static int datan_save_walk(struct datan_save_context *ctx,struct datan_save_entry *entry,uint8_t spellid) {
  if (!entry->map) return 0;
  
  if (!spellid) { // First step, must have explicit spell.
    if (entry->map->spellid) spellid=entry->map->spellid;
    else if (entry->map->saveto) spellid=entry->map->saveto;
    else return -1;
  
  } else { // Further steps. Terminate if we see an explicit spell.
    if (entry->map->spellid) return 0;
    if (entry->map->saveto) return 0;
    if (entry->effective_spellid==spellid) return 0;
    if (entry->effective_spellid) {
      // This is the error we've been looking for.
      fprintf(stderr,"%s:map:%d(0): Reachable from save points %d and %d.\n",datan.arpath,entry->map->id,entry->effective_spellid,spellid);
      return -2;
    }
    entry->effective_spellid=spellid;
  }
  
  // Recur into neighbors, but skip cardinals with an impassable border.
  ctx->TEMP_spellid=spellid;
  int err=datan_map_for_each_neighbor(entry->map,1,datan_save_walk_neighbor_cb,ctx);
  if (err<0) return err;
  
  return 0;
}

/* From each map with (saveto) or (spellid), walk to all reachable maps that don't have an explicit spell.
 * Everything we can reach this way must all end up with the same effective_spellid.
 * If not, it means there is a path to some map from two different save-point-setting maps. They can't both be right.
 * During this walk, we will use cardinal neighbors only if the edge is actually passable.
 * This is population and also validation. No need for further validation, unless you want to identify unreachable maps.
 */
 
static int datan_save_walk_all(struct datan_save_context *ctx) {
  struct datan_save_entry *entry=ctx->v;
  int i=ctx->c;
  for (;i-->0;entry++) {
    if (!entry->map) continue;
    if (entry->effective_spellid) continue;
    if (!entry->map->saveto&&!entry->map->spellid) continue;
    int err=datan_save_walk(ctx,entry,0);
    if (err<0) return err;
  }
  return 0;
}

/* Confirm that every map is associated with exactly one save point.
 *
 * TODO An exception might be made for Full 65, where the church's basement connects to the castle.
 * If we make such an exception, I guess use a new map command or flag, to tell us it's a known overlap case.
 */
 
int datan_validate_save_points() {
  struct datan_save_context ctx={0};
  int err;
  if ((err=datan_save_context_init(&ctx))<0) {
    datan_save_context_cleanup(&ctx);
    return err;
  }
  if ((err=datan_save_walk_all(&ctx))<0) {
    datan_save_context_cleanup(&ctx);
    return err;
  }
  datan_save_context_cleanup(&ctx);
  return 0;
}

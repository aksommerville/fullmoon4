#include "datan_internal.h"

/* Context.
 */
 
struct datan_song_context {
  struct datan_song_entry {
    struct datan_map *map; // WEAK, optional (null means this id is unused)
    int effective_songid; // If (map->songid) unset. Zero if unvisited.
  } *v; // indexed by id
  int c,a;
  uint8_t TEMP_songid; // quick handoff, prevents need for an extra callback context
};

static void datan_song_context_cleanup(struct datan_song_context *ctx) {
  if (ctx->v) free(ctx->v);
}

/* Add map.
 */
 
static int datan_song_add_map(struct datan_song_context *ctx,uint32_t id,struct datan_map *map) {
  if (!map) return 0;
  if (id>0x0000ffff) {
    fprintf(stderr,"%s:map:%d(0): Map ID must be less than 65536\n",datan.arpath,id);
    return -2;
  }
  if (id>=ctx->c) {
    if (id>=ctx->a) {
      int na=id+1;
      if (na<INT_MAX-128) na=(na+128)&~127;
      if (na>INT_MAX/sizeof(struct datan_song_entry)) return -1;
      void *nv=realloc(ctx->v,sizeof(struct datan_song_entry)*na);
      if (!nv) return -1;
      ctx->v=nv;
      ctx->a=na;
    }
    memset(ctx->v+ctx->c,0,sizeof(struct datan_song_entry)*(id+1-ctx->c));
    ctx->c=id+1;
  }
  struct datan_song_entry *entry=ctx->v+id;
  entry->map=map;
  return 0;
}

/* Build up initial context with all maps.
 */
 
static int datan_song_context_init(struct datan_song_context *ctx) {
  const struct datan_res *res=datan.resv;
  int i=datan.resc,err;
  for (;i-->0;res++) {
    if (res->type!=FMN_RESTYPE_MAP) continue;
    if ((err=datan_song_add_map(ctx,res->id,res->obj))<0) return err;
  }
  return 0;
}

/* Walk from one map.
 * (songid==0) for the start of the walk. This means (entry) must have an explicit song.
 */
 
static int datan_song_walk(struct datan_song_context *ctx,struct datan_song_entry *entry,uint8_t songid);
 
static int datan_song_walk_neighbor_cb(struct datan_map *from_map,uint16_t mapid,void *userdata) {
  struct datan_song_context *ctx=userdata;
  uint8_t songid=ctx->TEMP_songid;
  if (!mapid) return 0;
  if (mapid>=ctx->c) return -1;
  int err=datan_song_walk(ctx,ctx->v+mapid,songid);
  return err;
}
 
static int datan_song_walk(struct datan_song_context *ctx,struct datan_song_entry *entry,uint8_t songid) {
  if (!entry->map) return 0;
  
  if (!songid) { // First step, must have explicit song.
    if (entry->map->songid) songid=entry->map->songid;
    else return -1;
  
  } else { // Further steps. Terminate if we see an explicit spell.
    if (entry->map->songid) return 0;
    if (entry->effective_songid==songid) return 0;
    if (entry->effective_songid) {
      // This is the error we've been looking for.
      fprintf(stderr,"%s:map:%d(0): Reachable with songs %d and %d.\n",datan.arpath,entry->map->id,entry->effective_songid,songid);
      return -2;
    }
    entry->effective_songid=songid;
  }
  
  // Recur into neighbors, but skip cardinals with an impassable border.
  ctx->TEMP_songid=songid;
  int err=datan_map_for_each_neighbor(entry->map,1,datan_song_walk_neighbor_cb,ctx);
  if (err<0) return err;
  
  return 0;
}

/* From each map with (songid), walk to all reachable maps that don't have an explicit song.
 * Everything we can reach this way must all end up with the same effective_songid.
 * If not, it means there is a path to some map from two different song-setting maps. They can't both be right.
 * During this walk, we will use cardinal neighbors only if the edge is actually passable.
 * This is population and also validation. No need for further validation, unless you want to identify unreachable maps.
 */
 
static int datan_song_walk_all(struct datan_song_context *ctx) {
  struct datan_song_entry *entry=ctx->v;
  int i=ctx->c;
  for (;i-->0;entry++) {
    if (!entry->map) continue;
    
    // One extra assertion: If the map is a teleport target, it *must* set its song explicitly.
    if (entry->map->spellid) {
      if (!entry->map->songid) {
        fprintf(stderr,"%s:map:%d(0): Teleport target must set song explicitly.\n",datan.arpath,entry->map->id);
        return -2;
      }
    }
    
    if (entry->effective_songid) continue;
    if (!entry->map->songid) continue;
    int err=datan_song_walk(ctx,entry,0);
    if (err<0) return err;
  }
  return 0;
}

/* Confirm that every map plays one song.
 * If it doesn't declare a song explicitly, any map you can reach from there must have the same song.
 * Very similar idea to datan_validate_song_points.
 */
 
int datan_validate_song_choice() {
  struct datan_song_context ctx={0};
  int err;
  if ((err=datan_song_context_init(&ctx))<0) {
    datan_song_context_cleanup(&ctx);
    return err;
  }
  if ((err=datan_song_walk_all(&ctx))<0) {
    datan_song_context_cleanup(&ctx);
    return err;
  }
  datan_song_context_cleanup(&ctx);
  return 0;
}

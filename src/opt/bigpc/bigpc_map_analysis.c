#include "bigpc_internal.h"

/* Fetch a map and search for commands, first match wins.
 */
 
struct fmn_find_map_command_context {
  // Input:
  uint8_t mask;
  const uint8_t *qv;
  int qc;
  // Output:
  int16_t *xy;
};

static int fmn_find_map_command_cb(uint8_t opcode,const uint8_t *argv,int argc,void *userdata) {
  struct fmn_find_map_command_context *ctx=userdata;
  
  /* The request (ctx->qv,ctx->qc) matches the original command, ie [opcode,...argv].
   * Since the first byte has been separated for our convenience, this is a little inconvenient.
   */
  if (ctx->qc>1+argc) return 0;
  if ((ctx->mask&0x01)&&(opcode!=ctx->qv[0])) return 0;
  int qp=1,argp=0,bit=0x02;
  for (;qp<ctx->qc;qp++,argp++,bit<<=1) {
    if (!(bit&ctx->mask)) continue;
    if (ctx->qv[qp]!=argv[argp]) return 0;
  }
  
  fmn_map_location_for_command(ctx->xy,opcode,argv,argc);
  
  return 1;
}
 
uint8_t fmn_find_map_command_1(int16_t *xy,uint16_t mapid,uint8_t mask,const uint8_t *v,int vc,int16_t dx,int16_t dy) {
  const void *serial=0;
  int serialc=fmn_datafile_get_any(&serial,bigpc.datafile,FMN_RESTYPE_MAP,mapid);
  struct fmn_find_map_command_context ctx={
    .mask=mask,
    .qv=v,
    .qc=vc,
    .xy=xy,
  };
  if (!fmn_map_for_each_command(serial,serialc,fmn_find_map_command_cb,&ctx)) return 0;
  xy[0]+=dx*FMN_COLC;
  xy[1]+=dy*FMN_ROWC;
  return 1;
}

/* Find a command in nearby maps, for the compass.
 * Try to operate the same way Web does:
 *  - Look in current map first.
 *  - Then look in cardinal neighbors: N,S,W,E
 *  - Then look in door neighbors.
 *  - No further than one hop.
 *  - Zero mask returns nothing, despite it logically meaning "anything".
 */
 
uint8_t fmn_find_map_command(int16_t *xy,uint8_t mask,const uint8_t *v) {
  xy[0]=xy[1]=0;
  
  int vc;
       if (mask&0x80) vc=8;
  else if (mask&0x40) vc=7;
  else if (mask&0x20) vc=6;
  else if (mask&0x10) vc=5;
  else if (mask&0x08) vc=4;
  else if (mask&0x04) vc=3;
  else if (mask&0x02) vc=2;
  else if (mask&0x01) vc=1;
  else return 0;
  
  if (bigpc.mapid&&fmn_find_map_command_1(xy,bigpc.mapid,mask,v,vc,0,0)) return 1;
  if (fmn_global.neighborn&&fmn_find_map_command_1(xy,fmn_global.neighborn,mask,v,vc,0,-1)) return 1;
  if (fmn_global.neighbors&&fmn_find_map_command_1(xy,fmn_global.neighbors,mask,v,vc,0,1)) return 1;
  if (fmn_global.neighborw&&fmn_find_map_command_1(xy,fmn_global.neighborw,mask,v,vc,-1,0)) return 1;
  if (fmn_global.neighbore&&fmn_find_map_command_1(xy,fmn_global.neighbore,mask,v,vc,1,0)) return 1;
  
  const struct fmn_door *door=fmn_global.doorv;
  int i=fmn_global.doorc;
  for (;i-->0;door++) {
    if (!door->mapid) continue;
    if (fmn_find_map_command_1(xy,door->mapid,mask,v,vc,0,0)) {
      // Point to the door's location in this map; location in the remote map is not important.
      xy[0]=door->x;
      xy[1]=door->y;
      return 1;
    }
  }
  
  return 0;
}

/* Teleport target.
 */

static int fmn_find_teleport_target_cmd_cb(uint8_t opcode,const uint8_t *argv,int argc,void *userdata) {
  if (opcode==0x45) {
    if (argv[1]==*(uint8_t*)userdata) return 1;
  }
  return 0;
}

static int fmn_find_teleport_target_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  if (fmn_map_for_each_command(v,c,fmn_find_teleport_target_cmd_cb,userdata)==1) return id;
  return 0;
}
 
uint16_t fmn_find_teleport_target(uint8_t spellid) {
  // TODO We examine potentially every command in every map, every time you teleport.
  // Consider searching once at load, or at the first request, and caching by spellid.
  // If we do that, use it at bigpc_savedgame.c:bigpc_savedgame_mapid_from_spellid too.
  return fmn_datafile_for_each_of_type(bigpc.datafile,FMN_RESTYPE_MAP,fmn_find_teleport_target_cb,&spellid);
}

/* Directions to item or map, for the crow.
 */

static int fmn_any_mapid_with_item_command_cb(uint8_t opcode,const uint8_t *argv,int argc,void *userdata) {
  uint8_t itemid=*(uint8_t*)userdata;
  switch (opcode) {
    case 0x04: return -1; // ANCILLARY. Hopefully this command comes before any BURIED_TREASURE or SPRITE...
    case 0x62: if (argv[3]==itemid) return 1; // BURIED_TREASURE
    case 0x80: { // SPRITE
        uint16_t spriteid=(argv[1]<<8)|argv[2];
        if (spriteid==3) { // treasure (TODO can we not hard-code this?)
          if (argv[3]==itemid) return 1; // sprite->argv[0]
        }
      } break;
  }
  return 0;
}

static int fmn_any_mapid_with_item_cb(uint16_t type,uint16_t qualifier,uint32_t mapid,const void *v,int c,void *userdata) {
  return (fmn_map_for_each_command(v,c,fmn_any_mapid_with_item_command_cb,userdata)>0)?mapid:0;
}

static uint16_t fmn_any_mapid_with_item(uint8_t itemid) {
  return fmn_datafile_for_each_of_type(bigpc.datafile,FMN_RESTYPE_MAP,fmn_any_mapid_with_item_cb,&itemid);
}
 
uint8_t fmn_find_direction_to_item(uint8_t itemid) {
  
  // If the itemid is invalid, or we already have it, the answer is No.
  if (itemid>=FMN_ITEM_COUNT) return 0;
  if (fmn_global.itemv[itemid]) return 0;
  
  // Any map with this item in a treasure chest or buried, go with it.
  // I don't expect a given item to be available in more than one place, not items the app would ask for anyway.
  return fmn_find_direction_to_map(fmn_any_mapid_with_item(itemid));
}

/* Travel plan object, for determining next step to a given map.
 */
 
struct fmn_travel_plan {
// Caller should initialize:
  uint16_t frommapid;
  uint16_t tomapid;
  uint8_t use_holes; // ok to recommend neighbors separated by water (ie has the broom)
  uint8_t use_doors; // ok to travel through doors at all, should always be 1
  uint8_t use_buried_doors; // ok to travel through buried doors (ie has the shovel)
// Answer gets filled in here. Initialize to zero. OK to read after cleanup.
  uint8_t result;
// Private state, initialize to zero:
  uint16_t *visitedv;
  int visitedc,visiteda;
  struct fmn_travel_plan_edge {
    uint8_t dir; // initial direction
    uint16_t mapid;
  } *edgev;
  int edgec,edgea,edgeswapa;
  struct fmn_travel_plan_edge *edgeswapv;
  uint16_t cellphysics_imageid; // neighbors are likely to use the same image, so cache it
  const uint8_t *cellphysics;
};

static void fmn_travel_plan_cleanup(struct fmn_travel_plan *plan) {
  if (plan->visitedv) free(plan->visitedv);
  if (plan->edgev) free(plan->edgev);
  if (plan->edgeswapv) free(plan->edgeswapv);
}

/* Visited-maps list.
 */
 
static int fmn_travel_plan_visitedv_search(const struct fmn_travel_plan *plan,uint16_t mapid) {
  int lo=0,hi=plan->visitedc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (mapid<plan->visitedv[ck]) hi=ck;
    else if (mapid>plan->visitedv[ck]) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int fmn_travel_plan_add_visited(struct fmn_travel_plan *plan,uint16_t mapid) {
  int p=fmn_travel_plan_visitedv_search(plan,mapid);
  if (p>=0) return 0;
  p=-p-1;
  if (plan->visitedc>=plan->visiteda) {
    int na=plan->visiteda+32;
    if (na>INT_MAX/sizeof(uint16_t)) return -1;
    void *nv=realloc(plan->visitedv,sizeof(uint16_t)*na);
    if (!nv) return -1;
    plan->visitedv=nv;
    plan->visiteda=na;
  }
  memmove(plan->visitedv+p+1,plan->visitedv+p,sizeof(uint16_t)*(plan->visitedc-p));
  plan->visitedc++;
  plan->visitedv[p]=mapid;
  return 0;
}

/* Add an edge.
 * We check for redundancy against (visitedv) and (edgev).
 * So, it's safe to call for every reachable neighbor.
 * (dir) should be nonzero for all but the single initial edge.
 */
 
static int fmn_travel_plan_add_edge(struct fmn_travel_plan *plan,uint16_t mapid,uint8_t dir) {

  // Already visited? Fine, no worries, don't list it.
  if (fmn_travel_plan_visitedv_search(plan,mapid)>=0) return 0;

  // If we already have it listed as an edge, quietly do nothing.
  // This is a perfectly likely case; there can be diagonal neighbors in one edge set that both report their mutual neighbor.
  // They might have provided different directions, and I think that's fine too. Either one would be a correct answer.
  const struct fmn_travel_plan_edge *q=plan->edgev;
  int i=plan->edgec;
  for (;i-->0;q++) if (q->mapid==mapid) return 0;

  // Add it for real.
  if (plan->edgec>=plan->edgea) {
    int na=plan->edgea+16;
    if (na>INT_MAX/sizeof(struct fmn_travel_plan_edge)) return -1;
    void *nv=realloc(plan->edgev,sizeof(struct fmn_travel_plan_edge)*na);
    if (!nv) return -1;
    plan->edgev=nv;
    plan->edgea=na;
  }
  struct fmn_travel_plan_edge *edge=plan->edgev+plan->edgec++;
  edge->dir=dir;
  edge->mapid=mapid;
  return 0;
}

/* Nonzero if a given edge of a given map is passable per context.
 * There can be neighbor relationships where a SOLID wall is between them, that's normal, and it's not passable.
 * We also have some context-sensitive checks on borders covered by HOLE tiles.
 */
 
static int fmn_travel_plan_edge_passable(const struct fmn_travel_plan *plan,const uint8_t *cellv,uint8_t dir) {
  
  // If we don't have cellphysics, assume everything is passable.
  // Note: We're depending on TILESHEET command before any NEIGHBOR commands.
  if (!plan->cellphysics) return 1;
  
  // Which cells are in play?
  int x,y,w,h;
  switch (dir) {
    case FMN_DIR_W: x=0; y=0; w=1; h=FMN_ROWC; break;
    case FMN_DIR_E: x=FMN_COLC-1; y=0; w=1; h=FMN_ROWC; break;
    case FMN_DIR_N: x=0; y=0; w=FMN_COLC; h=1; break;
    case FMN_DIR_S: x=0; y=FMN_ROWC-1; w=FMN_COLC; h=1; break;
    default: return 0;
  }
  
  cellv+=y*FMN_COLC+x;
  for (;h-->0;cellv+=FMN_COLC) {
    const uint8_t *cellp=cellv;
    int xi=w;
    for (;xi-->0;cellp++) {
      uint8_t physics=plan->cellphysics[*cellp];
      switch (physics) {
        // Universally passable:
        case FMN_CELLPHYSICS_VACANT:
        case FMN_CELLPHYSICS_UNSHOVELLABLE:
          return 1;
        // Universally impassable:
        case FMN_CELLPHYSICS_SOLID:
        case FMN_CELLPHYSICS_UNCHALKABLE:
        case FMN_CELLPHYSICS_SAP:
        case FMN_CELLPHYSICS_SAP_NOCHALK:
        case FMN_CELLPHYSICS_REVELABLE:
          break;
        // Conditional:
        case FMN_CELLPHYSICS_HOLE:
        case FMN_CELLPHYSICS_WATER:
          if (plan->use_holes) return 1;
          break;
      }
    }
  }
  
  return 0;
}

/* Expand travel plan from one map.
 * This is the interesting part.
 * The named map should already be in (visitedv).
 * We will produce new edges if possible.
 */
 
struct fmn_travel_plan_expand_from_map_context {
  struct fmn_travel_plan *plan;
  uint8_t dir;
  int edgec0;
  const uint8_t *cellv;
};

static int fmn_travel_plan_expand_from_map_cb(uint8_t opcode,const uint8_t *argv,int argc,void *userdata) {
  struct fmn_travel_plan_expand_from_map_context *ctx=userdata;
  switch (opcode) {
  
    /* ANCILLARY
     * This map should not participate in path detection.
     * Drop any edges we produced already, and stop iteration.
     * It's safe to back edgec up without cleaning up entries.
     * The removed entries should *not* be marked visited; there can be non-ancillary paths thru them too.
     */
    case 0x04: {
        ctx->plan->edgec=ctx->edgec0;
      } return 1;
      
    /* TILESHEET
     * Load the new cellphysics.
     */
    case 0x21: {
        uint8_t imageid=argv[0];
        if (imageid!=ctx->plan->cellphysics_imageid) {
          const void *v=0;
          if (fmn_datafile_get_any(&v,bigpc.datafile,FMN_RESTYPE_TILEPROPS,imageid)>=256) {
            ctx->plan->cellphysics=v;
            ctx->plan->cellphysics_imageid=imageid;
          }
        }
      } break;
      
    /* NEIGHBOR*
     */
    #define NEIGHBOR(opcode,tag) \
      case opcode: if (fmn_travel_plan_edge_passable(ctx->plan,ctx->cellv,FMN_DIR_##tag)) { \
          uint16_t mapid=(argv[0]<<8)|argv[1]; \
          fmn_travel_plan_add_edge(ctx->plan,mapid,ctx->dir?ctx->dir:FMN_DIR_##tag); \
        } break;
    NEIGHBOR(0x40,W)
    NEIGHBOR(0x41,E)
    NEIGHBOR(0x42,N)
    NEIGHBOR(0x43,S)
    #undef NEIGHBOR
    
    /* DOOR.
     */
    case 0x60: if (ctx->plan->use_doors) {
        uint16_t mapid=(argv[1]<<8)|argv[2];
        fmn_travel_plan_add_edge(ctx->plan,mapid,ctx->dir?ctx->dir:0xff);
      } break;
      
    /* BURIED_DOOR.
     * We propose this as an option whether it's exposed or not.
     * (in fact, that "not" case is an important one: "What are you trying to tell me, bird? What? Should I dig here?")
     */
    case 0x81: if (ctx->plan->use_doors&&ctx->plan->use_buried_doors) {
        uint16_t mapid=(argv[3]<<8)|argv[4];
        fmn_travel_plan_add_edge(ctx->plan,mapid,ctx->dir?ctx->dir:0xff);
      } break;

  }
  return 0;
}
 
static int fmn_travel_plan_expand_from_map(struct fmn_travel_plan *plan,uint16_t mapid,uint8_t dir) {
  const void *serial=0;
  int serialc=fmn_datafile_get_any(&serial,bigpc.datafile,FMN_RESTYPE_MAP,mapid);
  if (serialc<1) return 0;
  struct fmn_travel_plan_expand_from_map_context ctx={
    .plan=plan,
    .dir=dir,
    .edgec0=plan->edgec,
    .cellv=serial,
  };
  fmn_map_for_each_command(serial,serialc,fmn_travel_plan_expand_from_map_cb,&ctx);
  return 0;
}

/* Check edges, and if we're not there yet, expand the map.
 * Returns zero if we didn't find it but can keep trying.
 * >0 if we found the target, <0 for unreachable or real errors, either way please stop calling.
 */
 
static int fmn_travel_plan_expand(struct fmn_travel_plan *plan) {

  // First: Does any edge match the destination? If so we're done.
  const struct fmn_travel_plan_edge *edge=plan->edgev;
  int i=plan->edgec;
  for (;i-->0;edge++) if (edge->mapid==plan->tomapid) {
    plan->result=edge->dir;
    return 1;
  }

  // Mark each edge visited.
  for (edge=plan->edgev,i=plan->edgec;i-->0;edge++) {
    if (fmn_travel_plan_add_visited(plan,edge->mapid)<0) return -1;
  }
  
  // Swap the edge lists. (edgeswapv) will be the previous state, and (edgev) the initially empty new one.
  void *tmpv=plan->edgev;
  plan->edgev=plan->edgeswapv;
  plan->edgeswapv=tmpv;
  int tmpi=plan->edgea;
  plan->edgea=plan->edgeswapa;
  plan->edgeswapa=tmpi;
  int edgeswapc=plan->edgec;
  plan->edgec=0;
  
  // Expand one step from each of the previous edge maps.
  for (edge=plan->edgeswapv,i=edgeswapc;i-->0;edge++) {
    if (fmn_travel_plan_expand_from_map(plan,edge->mapid,edge->dir)<0) return -1;
  }
  
  // If we didn't produce any new edges, the destination is unreachable.
  return plan->edgec?0:-1;
}

/* Start the travel plan. Caller must initialize per struct declaration above.
 * Fails if the answer is immediately knowable.
 */
 
static int fmn_travel_plan_begin(struct fmn_travel_plan *plan) {
  if (plan->frommapid==plan->tomapid) { plan->result=0xff; return -1; }
  if (!plan->frommapid||!plan->tomapid) return -1;
  
  if (fmn_travel_plan_add_edge(plan,plan->frommapid,0)<0) return -1;
  
  return 0;
}

/* Next direction you should travel to get from one map to another.
 * 0xff: None, they are the same map.
 * 0: Can't determine.
 * Otherwise one of FMN_DIR_(W,E,N,S).
 */
 
static uint8_t fmn_direction_map_to_map(uint16_t frommapid,uint16_t tomapid) {
  
  /* Start at (frommapid) and expand into all possible directions from there.
   * Keep doing this until we've visited every map, or found (tomapid).
   * ie a breadth-first search.
   */
  struct fmn_travel_plan plan={
    .frommapid=frommapid,
    .tomapid=tomapid,
    .use_holes=fmn_global.itemv[FMN_ITEM_BROOM],
    .use_doors=1,
    .use_buried_doors=fmn_global.itemv[FMN_ITEM_SHOVEL],
  };
  if (fmn_travel_plan_begin(&plan)<0) goto _done_;
  
  while (!fmn_travel_plan_expand(&plan)) ;
  
 _done_:;
  fmn_travel_plan_cleanup(&plan);
  return plan.result;
}

/* Direction from here to a given mapid, public entry point.
 */

uint8_t fmn_find_direction_to_map(uint16_t mapid) {
  return fmn_direction_map_to_map(bigpc.mapid,mapid);
}

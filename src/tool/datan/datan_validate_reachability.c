#include "datan_internal.h"
#include "opt/midi/midi.h"

/* Reachability context.
 */
 
struct datan_reachability_context {
  struct datan_reachability_res {
    // We keep only one qualifier, and don't record which.
    // It is assumed that references out of qualified peers will all be the same.
    // (and that's safe to assume because the resources that can refer to others, don't use qualifiers).
    uint16_t type;
    uint32_t id;
    void *obj; // WEAK, optional. only present if datan.resv had it
    int reachable;
  } *resv;
  int resc,resa;
};

static int datan_reachable(struct datan_reachability_context *ctx,uint16_t type,uint32_t id);

static void datan_reachability_context_cleanup(struct datan_reachability_context *ctx) {
  if (ctx->resv) free(ctx->resv);
}

/* Private resource list.
 */
 
static int datan_reachability_search(const struct datan_reachability_context *ctx,uint16_t type,uint32_t id) {
  int lo=0,hi=ctx->resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct datan_reachability_res *res=ctx->resv+ck;
         if (type<res->type) hi=ck;
    else if (type>res->type) lo=ck+1;
    else if (id<res->id) hi=ck;
    else if (id>res->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct datan_reachability_res *datan_reachability_insert(
  struct datan_reachability_context *ctx,
  int p,uint16_t type,uint32_t id
) {
  if ((p<0)||(p>ctx->resc)) return 0;
  if (ctx->resc>=ctx->resa) {
    int na=ctx->resa+256;
    if (na>INT_MAX/sizeof(struct datan_reachability_res)) return 0;
    void *nv=realloc(ctx->resv,sizeof(struct datan_reachability_res)*na);
    if (!nv) return 0;
    ctx->resv=nv;
    ctx->resa=na;
  }
  struct datan_reachability_res *res=ctx->resv+p;
  memmove(res+1,res,sizeof(struct datan_reachability_res)*(ctx->resc-p));
  ctx->resc++;
  res->type=type;
  res->id=id;
  res->obj=0;
  res->reachable=0;
  return res;
}

/* Init.
 */
 
static int datan_reachability_init_1(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct datan_reachability_context *ctx=userdata;
  int p=datan_reachability_search(ctx,type,id);
  if (p>=0) return 0; // must have already had it with a different qualifier, all good
  p=-p-1;
  struct datan_reachability_res *res=datan_reachability_insert(ctx,p,type,id);
  if (!res) return -1;
  res->obj=datan_res_get(type,qualifier,id);
  return 0;
}
 
static int datan_reachability_init(struct datan_reachability_context *ctx) {
  int err=fmn_datafile_for_each(datan.datafile,datan_reachability_init_1,ctx);
  if (err<0) return err;
  return 0;
}

/* Review.
 * At this point, all reachable resources have been visited.
 * Call out anything we missed.
 */
 
static int datan_reachability_review(struct datan_reachability_context *ctx) {

  // First, gather the total count unreachable. If zero, we're done.
  int unreachablec=0;
  const struct datan_reachability_res *res=ctx->resv;
  int i=ctx->resc;
  for (;i-->0;res++) if (!res->reachable) unreachablec++;
  if (!unreachablec) return 0;
  fprintf(stderr,"%s:WARNING: %d unreachable resources:\n",datan.arpath,unreachablec);
  
  // Bucket by type, and log the first few ids for that type.
  uint16_t current_type=0;
  int hidden_count=0;
  int linelen=0;
  for (res=ctx->resv,i=ctx->resc;i-->0;res++) {
    if (res->reachable) continue;
    if (res->type!=current_type) {
      if (current_type) {
        if (hidden_count) {
          fprintf(stderr," ...%d more\n",hidden_count);
          hidden_count=0;
        } else {
          fprintf(stderr,"\n");
        }
      }
      current_type=res->type;
      linelen=fprintf(stderr,"  %s:",fmn_restype_repr(current_type));
    }
    if (linelen<100) {
      linelen+=fprintf(stderr," %d",res->id);
    } else {
      hidden_count++;
    }
  }
  if (current_type) {
    if (hidden_count) {
      fprintf(stderr," ...%d more\n",hidden_count);
      hidden_count=0;
    } else {
      fprintf(stderr,"\n");
    }
  }

  return 0;
}

/* Map.
 */
 
static int datan_reachable_map_1(uint8_t opcode,const uint8_t *v,int c,void *userdata) {
  struct datan_reachability_context *ctx=userdata;
  switch (opcode) {
    case 0x20: return datan_reachable(ctx,FMN_RESTYPE_SONG,v[0]);
    case 0x21: {
        if (datan_reachable(ctx,FMN_RESTYPE_IMAGE,v[0])<0) return -1;
        if (datan_reachable(ctx,FMN_RESTYPE_TILEPROPS,v[0])<0) return -1;
      } break;
    case 0x40: return datan_reachable(ctx,FMN_RESTYPE_MAP,(v[0]<<8)|v[1]);
    case 0x41: return datan_reachable(ctx,FMN_RESTYPE_MAP,(v[0]<<8)|v[1]);
    case 0x42: return datan_reachable(ctx,FMN_RESTYPE_MAP,(v[0]<<8)|v[1]);
    case 0x43: return datan_reachable(ctx,FMN_RESTYPE_MAP,(v[0]<<8)|v[1]);
    case 0x60: return datan_reachable(ctx,FMN_RESTYPE_MAP,(v[1]<<8)|v[2]);
    case 0x80: {
        uint16_t spriteid=(v[1]<<8)|v[2];
        if (datan_reachable(ctx,FMN_RESTYPE_SPRITE,spriteid)<0) return -1;
        if (v[3]||v[4]||v[5]) {
          struct datan_sprite *sprite=datan_res_get(FMN_RESTYPE_SPRITE,0,spriteid);
          if (sprite) {
            if (datan_reachable(ctx,sprite->arg0type,v[3])<0) return -1;
            if (datan_reachable(ctx,sprite->arg1type,v[4])<0) return -1;
            if (datan_reachable(ctx,sprite->arg2type,v[5])<0) return -1;
          }
        }
      } break;
    case 0x81: return datan_reachable(ctx,FMN_RESTYPE_MAP,(v[3]<<8)|v[4]);
  }
  return 0;
}
 
static int datan_reachable_map(struct datan_reachability_context *ctx,struct datan_map *map) {
  return datan_map_for_each_command(map,datan_reachable_map_1,ctx);
}

/* Sprite.
 */
 
static int datan_reachable_sprite_1(uint8_t opcode,const uint8_t *v,int c,void *userdata) {
  struct datan_reachability_context *ctx=userdata;
  switch (opcode) {
    case 0x20: return datan_reachable(ctx,FMN_RESTYPE_IMAGE,v[0]);
  }
  return 0;
}
 
static int datan_reachable_sprite(struct datan_reachability_context *ctx,struct datan_sprite *sprite) {
  return datan_sprite_for_each_command(sprite,datan_reachable_sprite_1,ctx);
}

/* Song.
 */
 
static int datan_reachable_song(struct datan_reachability_context *ctx,struct midi_file *file) {
  // Read only up to the first delay. We nonstandardly require that all Program Change be at time zero.
  // (and that is asserted with a warning, at the time we decode the song).
  midi_file_restart(file);
  struct midi_event event={0};
  while (!midi_file_next(&event,file,0)) {
  
    if (event.opcode==MIDI_OPCODE_PROGRAM) {
      int err=datan_reachable(ctx,FMN_RESTYPE_INSTRUMENT,event.a);
      if (err<0) return err;
  
    } else if ((event.opcode==MIDI_OPCODE_NOTE_ON)&&(event.chid==0x0f)) {
      int err=datan_reachable(ctx,FMN_RESTYPE_SOUND,event.a);
      if (err<0) return err;
    }
  }
  return 0;
}

/* Mark one resource reachable, and if we're seeing it for the first time, recur to its references.
 */
 
static int datan_reachable(struct datan_reachability_context *ctx,uint16_t type,uint32_t id) {

  // First, easy, mark it reachable, and get out if it already was, or if it doesn't have an object.
  if (!type||!id) return 0;
  int p=datan_reachability_search(ctx,type,id);
  if (p<0) return 0;
  struct datan_reachability_res *res=ctx->resv+p;
  if (res->reachable) return 0;
  res->reachable=1;
  if (!res->obj) return 0;
  
  // Locate references per type.
  switch (type) {
    case FMN_RESTYPE_MAP: return datan_reachable_map(ctx,res->obj);
    case FMN_RESTYPE_SPRITE: return datan_reachable_sprite(ctx,res->obj);
    case FMN_RESTYPE_SONG: return datan_reachable_song(ctx,res->obj);
  }
  return 0;
}

/* Mark known resources as reachable, and let it fan out from there.
 */
 
static int datan_reachability_visit_known(struct datan_reachability_context *ctx) {
  int err,i;

  if ((err=datan_reachable(ctx,FMN_RESTYPE_MAP,1))<0) return err;
  // Also, any map that's teleportable is "reachable". This matters for the magic door.
  {
    struct datan_res *res=datan.resv;
    int i=datan.resc;
    for (;i-->0;res++) {
      if (res->obj&&(res->type==FMN_RESTYPE_MAP)&&((struct datan_map*)res->obj)->spellid) {
        if ((err=datan_reachable(ctx,FMN_RESTYPE_MAP,res->id))<0) return err;
      }
    }
  }
  
  if ((err=datan_reachable(ctx,FMN_RESTYPE_IMAGE,2))<0) return err; // hero
  if ((err=datan_reachable(ctx,FMN_RESTYPE_IMAGE,4))<0) return err; // items splash
  if ((err=datan_reachable(ctx,FMN_RESTYPE_IMAGE,14))<0) return err; // menu bits
  if ((err=datan_reachable(ctx,FMN_RESTYPE_IMAGE,16))<0) return err; // uitiles (eg hello menu cursor)
  if ((err=datan_reachable(ctx,FMN_RESTYPE_IMAGE,17))<0) return err; // spotlight (door transitions)
  if ((err=datan_reachable(ctx,FMN_RESTYPE_IMAGE,18))<0) return err; // logo
  if ((err=datan_reachable(ctx,FMN_RESTYPE_IMAGE,19))<0) return err; // logo overlay
  if ((err=datan_reachable(ctx,FMN_RESTYPE_IMAGE,20))<0) return err; // font
  
  if ((err=datan_reachable(ctx,FMN_RESTYPE_SONG,1))<0) return err; // tangled_vine (hello menu)
  if ((err=datan_reachable(ctx,FMN_RESTYPE_SONG,6))<0) return err; // truffles_in_forbidden_sauce (gameover menu)
  if ((err=datan_reachable(ctx,FMN_RESTYPE_SONG,7))<0) return err; // seventh_roots_of_unity (victory menu)
  
  for (i=3;i<=27;i++) { // hard-coded for menus
    if ((err=datan_reachable(ctx,FMN_RESTYPE_STRING,i))<0) return err;
  }
  
  for (i=1;i<=FMN_SFX_KICK_1;i++) {
    if ((err=datan_reachable(ctx,FMN_RESTYPE_SOUND,i))<0) return err;
  }
  // skip GM drums: Those are reachable only if a song uses them.
  for (i=FMN_SFX_COWBELL+1;i<=FMN_SFX_PANDA_CRY;i++) { // sound effects. must keep up to date with fmn_platform.h
    if ((err=datan_reachable(ctx,FMN_RESTYPE_SOUND,i))<0) return err;
  }

  return 0;
}

/* Validate reachability.
 * Arguably the most important thing this program does.
 * Given a few known hard-coded reachable resources, walk out thru their references to determine the entire reachable set.
 * Anything untouched after that, warrants a warning.
 */
 
int datan_validate_reachability() {
  struct datan_reachability_context ctx={0};
  int err=0;
  if ((err=datan_reachability_init(&ctx))<0) goto _done_;
  if ((err=datan_reachability_visit_known(&ctx))<0) goto _done_;
  if ((err=datan_reachability_review(&ctx))<0) goto _done_;
 _done_:
  datan_reachability_context_cleanup(&ctx);
  return err;
}

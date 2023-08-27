#include "mkd_internal.h"
#include "mkd_ar.h"
#include "app/fmn_platform.h"

/* Context.
 * Note that we don't track qualifiers, though the archive may have qualified resources.
 * This is by design. Anything reachable by *any* version of a resource, it's "reachable".
 */
 
/* Keep no more than so many versions of one resource.
 * In reality, there shouldn't be more than 3 qualifiers for any type (instrument and sound both have 3).
 * Strings could have many more, but we filter those out in advance (strings do not refer to other resources so don't need to participate).
 */
#define MKD_TREESHAKE_SERIAL_LIMIT 8
 
struct mkd_treeshake {
  struct mkd_ar *ar;
  const char *path;
  int resc;
  struct mkd_apple {
    int type,id;
    int reachable;
    struct mkd_serial {
      const void *v;
      int c;
    } serialv[MKD_TREESHAKE_SERIAL_LIMIT];
    int serialc;
  } *applev;
  int applec,applea;
};

static void mkd_treeshake_cleanup(struct mkd_treeshake *ts) {
  if (ts->applev) {
    free(ts->applev);
  }
}

/* Get or add an apple.
 */
 
static int mkd_treeshake_search(const struct mkd_treeshake *ts,int type,int id) {
  int lo=0,hi=ts->applec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    struct mkd_apple *apple=ts->applev+ck;
         if (type<apple->type) hi=ck;
    else if (type>apple->type) lo=ck+1;
    else if (id<apple->id) hi=ck;
    else if (id>apple->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}
 
static struct mkd_apple *mkd_treeshake_get_apple(struct mkd_treeshake *ts,int type,int id,int add) {
  int p=mkd_treeshake_search(ts,type,id);
  if (p>=0) return ts->applev+p;
  if (!add) return 0;
  p=-p-1;
  if (ts->applec>=ts->applea) {
    int na=ts->applea+256;
    if (na>INT_MAX/sizeof(struct mkd_apple)) return 0;
    void *nv=realloc(ts->applev,sizeof(struct mkd_apple)*na);
    if (!nv) return 0;
    ts->applev=nv;
    ts->applea=na;
  }
  struct mkd_apple *apple=ts->applev+p;
  memmove(apple+1,apple,sizeof(struct mkd_apple)*(ts->applec-p));
  ts->applec++;
  memset(apple,0,sizeof(struct mkd_apple));
  apple->type=type;
  apple->id=id;
  return apple;
}

static int mkd_treeshake_is_reachable(struct mkd_treeshake *ts,int type,int id) {
  const struct mkd_apple *apple=mkd_treeshake_get_apple(ts,type,id,0);
  if (!apple) return 0;
  return apple->reachable;
}

/* Add serial to one apple.
 */
 
static int mkd_apple_add_serial(struct mkd_apple *apple,const void *v,int c) {
  if (apple->serialc>=MKD_TREESHAKE_SERIAL_LIMIT) {
    fprintf(stderr,
      "!!! Too many versions of resource %s:%d, hard-coded limit of %d in %s !!!\n",
      mkd_restype_repr(apple->type),apple->id,
      MKD_TREESHAKE_SERIAL_LIMIT,__FILE__
    );
    return -1;
  }
  struct mkd_serial *serial=apple->serialv+apple->serialc++;
  serial->v=v;
  serial->c=c;
  return 0;
}

/* All resources get recorded as apples.
 * But for certain types that we know are always leaf nodes, don't bother recording serial.
 * This is important for string, since there can be many different qualifiers, one per language.
 */
 
static int mkd_treeshake_can_ignore_type(int type) {
  switch (type) {
    case FMN_RESTYPE_STRING:
    case FMN_RESTYPE_INSTRUMENT: // in theory, instruments could refer to sounds, but our synths don't do that.
    case FMN_RESTYPE_SOUND:
    case FMN_RESTYPE_IMAGE:
    case FMN_RESTYPE_TILEPROPS:
      return 1;
  }
  return 0;
}

/* Add an unreachable record for each (type,id).
 */
 
static int mkd_treeshake_gather_cb(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata) {
  struct mkd_treeshake *ts=userdata;
  struct mkd_apple *apple=mkd_treeshake_get_apple(ts,type,id,1);
  if (!apple) return -1;
  if (!mkd_treeshake_can_ignore_type(type)) {
    if (mkd_apple_add_serial(apple,v,c)<0) return -1;
  }
  return 0;
}
 
static int mkd_treeshake_gather(struct mkd_treeshake *ts) {
  return mkd_ar_for_each(ts->ar,mkd_treeshake_gather_cb,ts);
}

/* Remove from the archive anything that remains unreachable.
 */
 
static int mkd_treeshake_eliminate_cb(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata) {
  struct mkd_treeshake *ts=userdata;
  struct mkd_apple *apple=mkd_treeshake_get_apple(ts,type,id,0);
  if (!apple) return 1; // Error but we can't report it. Keep the resource.
  if (apple->reachable) return 1;
  return 0;
}
 
static int mkd_treeshake_eliminate(struct mkd_treeshake *ts) {
  return mkd_ar_filter(ts->ar,mkd_treeshake_eliminate_cb,ts);
}

/* Mark one resource reachable, then discover and reenter its referenced resources.
 */
 
static int mkd_treeshake_reachable(struct mkd_treeshake *ts,int type,int id);
 
static int mkd_treeshake_reachable_cb(int type,int id,void *userdata) {
  return mkd_treeshake_reachable(userdata,type,id);
}
 
static int mkd_treeshake_reachable(struct mkd_treeshake *ts,int type,int id) {
  struct mkd_apple *apple=mkd_treeshake_get_apple(ts,type,id,0);
  if (!apple) return 0;
  if (apple->reachable) return 0;
  apple->reachable=1;
  const struct mkd_serial *serial=apple->serialv;
  int i=apple->serialc;
  for (;i-->0;serial++) {
    int err=mkd_res_for_each_reference(apple->type,serial->v,serial->c,mkd_treeshake_reachable_cb,ts);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error tracing references.\n",mkd_restype_repr(apple->type),apple->id);
      return -2;
    }
  }
  return 0;
}

/* Special handler to search for teleportable maps -- they are all implicitly reachable.
 */
 
static int mkd_map_is_teleportable_cb(uint8_t opcode,const uint8_t *argv,int argc,void *userdata) {
  if (opcode!=0x45) return 0; // hero
  if (!argv[1]) return 0; // spellid
  return 1;
}
 
static int mkd_map_is_teleportable(const uint8_t *v,int c) {
  return fmn_map_for_each_command(v,c,mkd_map_is_teleportable_cb,0);
}
 
static int mkd_treeshake_teleportable_maps(struct mkd_treeshake *ts) {
  int p=mkd_treeshake_search(ts,FMN_RESTYPE_MAP,0);
  if (p<0) p=-p-1;
  const struct mkd_apple *apple=ts->applev+p;
  for (;(p<ts->applec)&&(apple->type==FMN_RESTYPE_MAP);p++,apple++) {
    const struct mkd_serial *serial=apple->serialv;
    int i=apple->serialc;
    for (;i-->0;serial++) {
      if (mkd_map_is_teleportable(serial->v,serial->c)) {
        int err=mkd_treeshake_reachable(ts,apple->type,apple->id);
        if (err<0) return err;
        break;
      }
    }
  }
  return 0;
}

/* Shake tree of one archive in memory. Public entry point.
 * Caller should strip undesired qualifiers first.
 */
 
int mkd_archive_shake_tree(struct mkd_ar *ar,const char *path) {
  int err=0,i;
  struct mkd_treeshake ts={.path=path,.ar=ar};
  ts.resc=mkd_ar_count(ar);
  
  if ((err=mkd_treeshake_gather(&ts))<0) goto _done_;
  
  /* ----- Begin schedule of known reachable resources. ----- */
  
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_MAP,1))<0) goto _done_;
  // Also, any map that's teleportable is "reachable". This matters for the magic door.
  if ((err=mkd_treeshake_teleportable_maps(&ts))<0) goto _done_;
  
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_IMAGE,2))<0) goto _done_; // hero
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_IMAGE,4))<0) goto _done_; // items splash
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_IMAGE,14))<0) goto _done_; // menu bits
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_IMAGE,16))<0) goto _done_; // uitiles (eg hello menu cursor)
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_IMAGE,17))<0) goto _done_; // spotlight (door transitions)
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_IMAGE,18))<0) goto _done_; // logo
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_IMAGE,19))<0) goto _done_; // logo overlay
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_IMAGE,20))<0) goto _done_; // font
  
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_SONG,1))<0) goto _done_; // tangled_vine (hello menu)
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_SONG,6))<0) goto _done_; // truffles_in_forbidden_sauce (gameover menu)
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_SONG,7))<0) goto _done_; // seventh_roots_of_unity (victory/credits menu)
  
  if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_INSTRUMENT,1))<0) goto _done_; // Dot's violin
  
  for (i=3;i<=27;i++) { // hard-coded for menus
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_STRING,i))<0) goto _done_;
  }
  for (i=34;i<=69;i++) { // hard-coded for menus
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_STRING,i))<0) goto _done_;
  }
  
  for (i=1;i<=FMN_SFX_KICK_1;i++) {
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_SOUND,i))<0) goto _done_;
  }
  // skip GM drums: Those are reachable only if a song uses them.
  for (i=FMN_SFX_COWBELL+1;i<=FMN_SFX_COIN_LAND;i++) { // sound effects. must keep up to date with fmn_platform.h
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_SOUND,i))<0) goto _done_;
  }
  
  /* ----- Now some second-order tests depending on what else is present. ----- */
  
  if (mkd_treeshake_is_reachable(&ts,FMN_RESTYPE_SPRITE,55)) { // musicteacher...
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_INSTRUMENT,42))<0) goto _done_; // ...saxophone
  }
  if (mkd_treeshake_is_reachable(&ts,FMN_RESTYPE_SPRITE,70)) { // arcade...
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_SONG,14))<0) goto _done_; // ...sky-gardening
  }
  if (mkd_treeshake_is_reachable(&ts,FMN_RESTYPE_MAP,100)) { // Demo has 51 maps and full 150. If map:100 exists, it's the full version.
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_IMAGE,15))<0) goto _done_; // credits
    // Maps referenced programmatically by credits:
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_MAP,101))<0) goto _done_;
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_MAP,146))<0) goto _done_;
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_MAP,147))<0) goto _done_;
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_MAP,148))<0) goto _done_;
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_MAP,149))<0) goto _done_;
    if ((err=mkd_treeshake_reachable(&ts,FMN_RESTYPE_MAP,150))<0) goto _done_;
  } else {
  }
  
  /* ----- End schedule of known reachable resources. ----- */
  
  if ((err=mkd_treeshake_eliminate(&ts))<0) goto _done_;
  
 _done_:;
  mkd_treeshake_cleanup(&ts);
  return err;
}

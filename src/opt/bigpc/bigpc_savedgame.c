#include "bigpc_internal.h"
#include <string.h>

#define BIGPC_SAVEDGAME_DEBOUNCE_TIME_S 3

#if 0 //XXX bigpc_config_ready takes care of this now
/* Guess path, at init.
 */
 
static void bigpc_savedgame_guess_path() {
  if (bigpc.savedgame_path) return; // why did you call me?
  
  // If exename contains a slash, try its dirname plus "/data".
  int eslashp=-1;
  int i=0;
  for (;bigpc.exename[i];i++) {
    if (bigpc.exename[i]=='/') eslashp=i;
  }
  if (eslashp>=0) {

    // Little different for Macs: If dir ends "/Contents/MacOS", back out one step beyond that.
    // Aim for the directory hosting the application bundle, don't save inside the bundle.
    if ((eslashp>=15)&&!memcmp(bigpc.exename+eslashp-15,"/Contents/MacOS",15)) {
      eslashp-=15;
      // Read backward over what is hopefully something like "/Users/mistermustard/my/games/FullMoon.app..."
      while ((eslashp>=0)&&(bigpc.exename[eslashp]!='/')) eslashp--;
      if (eslashp<0) return;
    }
  
    // With that directory path in hand, append "/save".
    int pathlen=eslashp+5;
    if (!(bigpc.savedgame_path=malloc(pathlen+1))) return;
    memcpy(bigpc.savedgame_path,bigpc.exename,eslashp);
    memcpy(bigpc.savedgame_path+eslashp,"/save",6);
    return;
  }
  
  // Doubtless plenty of other guesses we could make, but I'm leaving it here for now.
}
#endif

/* Init.
 */
 
void bigpc_savedgame_init() {
  bigpc.savedgame_dirty=0;
  bigpc.saveto_recent=FMN_SPELLID_HOME;

  if (!bigpc.savedgame_path) { // should not be set yet
    if (bigpc.config.savedgame_path) { // should always be set after bigpc_config_ready
      if (!(bigpc.savedgame_path=strdup(bigpc.config.savedgame_path))) return;
    }
  }
  
  // Null or empty path, whatever, saving will not be an option.
  if (!bigpc.savedgame_path||!bigpc.savedgame_path[0]) {
    fprintf(stderr,"%s:WARNING: Saved game path unset and we failed to guess. Will not be able to save. Launch with '--save=...' to specify a file.\n",bigpc.exename);
    return;
  }
  
  if (bigpc.savedgame_serial) { // should not be set yet
    free(bigpc.savedgame_serial);
    bigpc.savedgame_serial=0;
  }
  bigpc.savedgame_serialc=0;
  bigpc.savedgame_seriala=0;
  if ((bigpc.savedgame_serialc=fmn_file_read(&bigpc.savedgame_serial,bigpc.savedgame_path))<0) {
    // Failure to read the save file is not a hard error, and I'm not sure it even warrants logging.
    fprintf(stderr,"%s: Failed to read saved game.\n",bigpc.savedgame_path);
    bigpc.savedgame_serialc=0;
    return;
  }
  bigpc.savedgame_seriala=bigpc.savedgame_serialc;
  fprintf(stderr,"%s: Acquired saved game, %d bytes.\n",bigpc.savedgame_path,bigpc.savedgame_serialc);
}

/* Delete.
 */
 
void bigpc_savedgame_delete() {
  if (bigpc.savedgame_path) fmn_file_delete(bigpc.savedgame_path);
  bigpc.savedgame_serialc=0;
  bigpc.savedgame_dirty=0;
  // Don't free the path. User may start a new campaign, and would save in the same place as the old one.
}

/* spellid <~> mapid
 * This is fuzzy and complicated. We can always fall back to FMN_SPELLID_HOME.
 */
 
static int bigpc_savedgame_spellid_map_cb(uint8_t opcode,const uint8_t *argv,int argc,void *userdata) {
  if (opcode==0x22) return argv[0]; // saveto
  if (opcode==0x45) return argv[1]; // hero
  return 0;
}
 
static uint8_t bigpc_savedgame_spellid() {
  // This happens a lot, we save at least on every map transition.
  // So I'm taking pains to avoid having to search across different maps.
  
  // Spell can be named by a map command. This is the preferred case, and I plan to use this command on every map except the save points themselves.
  if (fmn_global.saveto) {
    return fmn_global.saveto;
  }
  
  // Read the current map, looking for a 'hero' command with nonzero spell id.
  {
    const void *map=0;
    int mapc=fmn_datafile_get_any(&map,bigpc.datafile,FMN_RESTYPE_MAP,bigpc.mapid);
    int spellid=fmn_map_for_each_command(map,mapc,bigpc_savedgame_spellid_map_cb,0);
    if (spellid>0) {
      return spellid;
    }
  }
  
  // If we have a nonzero 'saveto_recent', use it.
  if (bigpc.saveto_recent) {
    return bigpc.saveto_recent;
  }
  
  // saveto_recent is supposed to always be nonzero, so this shouldn't happen.
  return FMN_SPELLID_HOME;
}

static int bigpc_savedgame_mapid_from_spellid_cmd_cb(uint8_t opcode,const uint8_t *argv,int argc,void *userdata) {
  if (opcode==0x45) {
    if (argv[1]==*(uint8_t*)userdata) return 1;
  }
  return 0;
}

static int bigpc_savedgame_mapid_from_spellid_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  if (fmn_map_for_each_command(v,c,bigpc_savedgame_mapid_from_spellid_cmd_cb,userdata)==1) return id;
  return 0;
}

static uint16_t bigpc_savedgame_mapid_from_spellid(uint8_t spellid) {
  // This only happens during saved game load, typically once per session.
  // So it's not a big concern, that we potentially examine every command in every map.
  int mapid=fmn_datafile_for_each_of_type(bigpc.datafile,FMN_RESTYPE_MAP,bigpc_savedgame_mapid_from_spellid_cb,&spellid);
  if (mapid) return mapid;
  // If we fail, it's not the end of the world: map 1 is always a valid starting point.
  return 1;
}
 
static uint8_t bigpc_savedgame_itembits(const uint8_t *v) {
  return (
    (v[0]?0x01:0)|
    (v[1]?0x02:0)|
    (v[2]?0x04:0)|
    (v[3]?0x08:0)|
    (v[4]?0x10:0)|
    (v[5]?0x20:0)|
    (v[6]?0x40:0)|
    (v[7]?0x80:0)|
  0);
}

static void bigpc_savedgame_itemv_from_bits(uint8_t *dst,uint16_t src) {
  uint16_t mask=1;
  for (;mask;mask<<=1,dst++) *dst=(src&mask)?1:0;
}

/* Split chunks.
 */
 
static int bigpc_savedgame_iterate(
  const uint8_t *src,int srcc,
  int (*cb)(uint8_t chunkid,const uint8_t *src,int srcc,int offset,void *userdata),
  void *userdata
) {

  // First two bytes are a chunk header but double as a signature.
  if (!src||(srcc<2)||memcmp(src,"\x01\x1b",2)) return -1;

  int srcp=0,err;
  while (srcp<srcc) {
    if (srcp>srcc-2) return -1;
    uint8_t chunkid=src[srcp++];
    if (!chunkid) break; // 0x00 = EOF
    uint8_t chunklen=src[srcp++];
    if (srcp>srcc-chunklen) return -1;
    if (err=cb(chunkid,src+srcp,chunklen,srcp,userdata)) return err;
    srcp+=chunklen;
  }
  return 0;
}

/* Validate.
 * In general, bad data is ok. We ignore or auto-correct at load in most cases.
 * Do assert framing: Signature, chunk headers, size of known fixed-length chunks.
 * Iterator asserts the signature, chunk headers, and presence of FIXED01 for us.
 */
 
static int bigpc_savedgame_validate_1(uint8_t chunkid,const uint8_t *src,int srcc,int offset,void *userdata) {
  switch (chunkid) {
    case 0x01: if (srcc!=27) return -1; break; // FIXED01
    case 0x02: if (srcc<2) return -1; break; // GSBIT
    case 0x03: if (srcc!=9) return -1; break; // PLANT
    case 0x04: if (srcc!=6) return -1; break; // SKETCH
  } 
  return 0;
}
 
int bigpc_savedgame_validate(const void *src,int srcc) {
  return bigpc_savedgame_iterate(src,srcc,bigpc_savedgame_validate_1,0);
}

/* Load.
 */
 
static int bigpc_savedgame_load_1(uint8_t chunkid,const uint8_t *src,int srcc,int offset,void *userdata) {
  switch (chunkid) {
  
    case 0x01: { // FIXED01
        if (srcc!=27) return -1;
        *(uint16_t*)userdata=bigpc_savedgame_mapid_from_spellid(src[0]);
        bigpc.clock.last_game_time_ms=(src[1]<<24)|(src[2]<<16)|(src[3]<<8)|src[4];
        fmn_global.damage_count=(src[5]<<8)|src[6];
        fmn_global.transmogrification=src[7];
        fmn_global.selected_item=src[8];
        bigpc_savedgame_itemv_from_bits(fmn_global.itemv,(src[9]<<8)|src[10]);
        memcpy(fmn_global.itemqv,src+11,16);
        fmn_reset_clock(bigpc.clock.last_game_time_ms);
      } break;
      
    case 0x02: { // GSBIT
        if (srcc<2) return -1;
        uint16_t startp=(src[0]<<8)|src[1];
        if (startp<sizeof(fmn_global.gs)) {
          src+=2;
          srcc-=2;
          int cpc=sizeof(fmn_global.gs)-startp;
          if (cpc>srcc) cpc=srcc;
          memcpy(fmn_global.gs+startp,src,srcc);
        }
      } break;
      
    case 0x03: { // PLANT
        if (srcc!=9) return -1;
        struct fmn_plant *plant=fmstore_add_plant(bigpc.fmstore,(src[0]<<8)|src[1],src[2]%FMN_COLC,src[2]/FMN_COLC);
        if (!plant) return 0;
        plant->state=src[3];
        plant->fruit=src[4];
        plant->flower_time=(src[5]<<24)|(src[6]<<16)|(src[7]<<8)|src[8];
      } break;
      
    case 0x04: { // SKETCH
        if (srcc!=6) return -1;
        struct fmn_sketch *sketch=fmstore_add_sketch(bigpc.fmstore,(src[0]<<8)|src[1],src[2]%FMN_COLC,src[2]/FMN_COLC);
        if (!sketch) return 0;
        sketch->bits=(src[3]<<16)|(src[4]<<8)|src[5];
      } break;
  
  }
  return 0;
}
 
uint16_t bigpc_savedgame_load(const void *src,int srcc) {
  uint16_t mapid=0;
  fmstore_clear_plants(bigpc.fmstore);
  fmstore_clear_sketches(bigpc.fmstore);
  if (bigpc_savedgame_iterate(src,srcc,bigpc_savedgame_load_1,&mapid)<0) return 0;
  return mapid;
}

/* Encode to provided buffer from global state.
 */
 
static int bigpc_savedgame_encode_header(uint8_t *dst,int dsta) {
  // Length is fixed. 2 bytes chunk header and 27 bytes payload.
  if (dsta<29) return 29;
  dst[0]=0x01; // FIXED01
  dst[1]=27;
  dst[2]=bigpc_savedgame_spellid();
  dst[3]=bigpc.clock.last_game_time_ms>>24;
  dst[4]=bigpc.clock.last_game_time_ms>>16;
  dst[5]=bigpc.clock.last_game_time_ms>>8;
  dst[6]=bigpc.clock.last_game_time_ms;
  dst[7]=fmn_global.damage_count>>8;
  dst[8]=fmn_global.damage_count;
  dst[9]=fmn_global.transmogrification;
  dst[10]=fmn_global.selected_item;
  dst[11]=bigpc_savedgame_itembits(fmn_global.itemv+8);
  dst[12]=bigpc_savedgame_itembits(fmn_global.itemv);
  memcpy(dst+13,fmn_global.itemqv,16);
  return 29;
}
 
static int bigpc_savedgame_encode_gsbit(uint8_t *dst,int dsta) {
  // Doing it the right way. If fmn_global.gs ever grows beyond 253 bytes, no problemo, we'll put it in more than one chunk.
  // (if it does get really big, consider truncating zeroes from the absolute start and end).
  int dstc=0;
  const uint8_t *src=fmn_global.gs;
  int srcc=sizeof(fmn_global.gs),offset=0;
  while (srcc>0) {
    int cpc=(srcc>253)?253:srcc;
    if (dstc<dsta) dst[dstc]=0x02; dstc++;
    if (dstc<dsta) dst[dstc]=2+cpc; dstc++;
    if (dstc<dsta) dst[dstc]=offset>>8; dstc++;
    if (dstc<dsta) dst[dstc]=offset; dstc++;
    if (dstc<=dsta-cpc) memcpy(dst+dstc,src,cpc); dstc+=cpc;
    src+=cpc;
    srcc-=cpc;
    offset+=cpc;
  }
  return dstc;
}

struct bigpc_savedgame_encode_context {
  uint8_t *dst;
  int dstc,dsta;
};

#define APPEND(v) if (ctx->dstc<ctx->dsta) ctx->dst[ctx->dstc]=(v); ctx->dstc++;

static int bigpc_savedgame_encode_sketch(uint16_t mapid,struct fmn_sketch *sketch,void *userdata) {
  struct bigpc_savedgame_encode_context *ctx=userdata;
  APPEND(0x04)
  APPEND(6)
  APPEND(mapid>>8)
  APPEND(mapid)
  APPEND(sketch->y*FMN_COLC+sketch->x)
  APPEND(sketch->bits>>16)
  APPEND(sketch->bits>>8)
  APPEND(sketch->bits)
  return 0;
}

static int bigpc_savedgame_encode_plant(uint16_t mapid,struct fmn_plant *plant,void *userdata) {
  struct bigpc_savedgame_encode_context *ctx=userdata;
  APPEND(0x03)
  APPEND(9)
  APPEND(mapid>>8)
  APPEND(mapid)
  APPEND(plant->y*FMN_COLC+plant->x)
  APPEND(plant->state)
  APPEND(plant->fruit)
  APPEND(plant->flower_time>>24)
  APPEND(plant->flower_time>>16)
  APPEND(plant->flower_time>>8)
  APPEND(plant->flower_time)
  return 0;
}

#undef APPEND
 
static int bigpc_savedgame_encode_sketches(uint8_t *dst,int dsta) {
  struct bigpc_savedgame_encode_context ctx={.dst=dst,.dsta=dsta};
  if (fmstore_for_each_sketch(bigpc.fmstore,bigpc_savedgame_encode_sketch,&ctx)<0) return -1;
  return ctx.dstc;
}
 
static int bigpc_savedgame_encode_plants(uint8_t *dst,int dsta) {
  struct bigpc_savedgame_encode_context ctx={.dst=dst,.dsta=dsta};
  if (fmstore_for_each_plant(bigpc.fmstore,bigpc_savedgame_encode_plant,&ctx)<0) return -1;
  return ctx.dstc;
}
 
static int bigpc_savedgame_encode(uint8_t *dst,int dsta) {
  int dstc=0,err;
  if ((err=bigpc_savedgame_encode_header(dst+dstc,dsta-dstc))<0) return err; dstc+=err;
  if ((err=bigpc_savedgame_encode_gsbit(dst+dstc,dsta-dstc))<0) return err; dstc+=err;
  if ((err=bigpc_savedgame_encode_sketches(dst+dstc,dsta-dstc))<0) return err; dstc+=err;
  if ((err=bigpc_savedgame_encode_plants(dst+dstc,dsta-dstc))<0) return err; dstc+=err;
  return dstc;
}

/* Encode and save.
 */
 
static int bigpc_savedgame_save_now() {
  fmstore_read_plants_from_globals(bigpc.fmstore,bigpc.mapid);
  fmstore_read_sketches_from_globals(bigpc.fmstore,bigpc.mapid);
  while (1) {
    int err=bigpc_savedgame_encode(bigpc.savedgame_serial,bigpc.savedgame_seriala);
    if (err<0) {
      //fprintf(stderr,"bigpc_savedgame_encode() failed!\n");
      return -1;
    }
    if (err<=bigpc.savedgame_seriala) {
      bigpc.savedgame_serialc=err;
      break;
    }
    int na=err;
    if (na<INT_MAX-1024) na=(na+1024)&~1023; // 1024 should be way more than we need. (but in theory it's open-ended)
    void *nv=realloc(bigpc.savedgame_serial,na);
    if (!nv) return -1;
    bigpc.savedgame_serial=nv;
    bigpc.savedgame_seriala=na;
  }
  if (fmn_file_write(bigpc.savedgame_path,bigpc.savedgame_serial,bigpc.savedgame_serialc)<0) {
    //fprintf(stderr,"%s: Failed to write saved game, %d bytes.\n",bigpc.savedgame_path,bigpc.savedgame_serialc);
    return -1;
  }
  return 0;
}

/* Set dirty.
 */
 
void bigpc_savedgame_dirty() {
  if (bigpc.savedgame_dirty) return;
  if (!bigpc.savedgame_path||!bigpc.savedgame_path[0]) return;
  bigpc.savedgame_dirty=1;
  bigpc.savedgame_update_time=bigpc.clock.last_real_time_us+BIGPC_SAVEDGAME_DEBOUNCE_TIME_S*1000000ll;
}

/* Check dirty timeout, and save when it expires.
 */
 
void bigpc_savedgame_update() {
  if (!bigpc.savedgame_dirty) return;
  if (bigpc.clock.last_real_time_us<bigpc.savedgame_update_time) return;
  if (bigpc.savedgame_suppress) return;
  if (bigpc_savedgame_save_now()<0) {
    fprintf(stderr,"%s: Failed to save game!\n",bigpc.savedgame_path);
  } else {
    
  }
  // Pass or fail, don't try again until it dirties again.
  bigpc.savedgame_dirty=0;
}

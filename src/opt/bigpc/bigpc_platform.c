#include "bigpc_internal.h"
#include "app/fmn_game.h"
#include <stdio.h>
#include <stdarg.h>

/* Hard abort.
 */
 
void fmn_abort() {
  fprintf(stderr,"%s! Exitting due to error.\n",__func__);
  bigpc.aborted=1;
  bigpc.sigc++;
}

int8_t fmn_quit() {
  bigpc.sigc++;
  return 0;
}

uint8_t fmn_can_quit() {
  return 1;
}

/* Reset, eg after the victory menu.
 */
 
void fmn_reset() {

  bigpc.savedgame_suppress=1;
  memset(&fmn_global,0,sizeof(fmn_global));
  fmstore_del(bigpc.fmstore);
  bigpc.fmstore=fmstore_new();
  
  if (fmn_init()<0) {
    fprintf(stderr,"Error reinitializing game.\n");
    bigpc.sigc++;
    return;
  }
  bigpc_clock_reset(&bigpc.clock);
  bigpc_savedgame_dirty();
}

/* Map load helpers.
 */
 
static void bigpc_clear_map_commands() {
  fmn_global.maptsid=0;
  fmn_global.songid=0;
  fmn_global.neighborw=0;
  fmn_global.neighbore=0;
  fmn_global.neighborn=0;
  fmn_global.neighbors=0;
  fmn_global.mapflag=0;
  fmn_global.herostartp=0;
  fmn_global.doorc=0;
  fmn_global.plantc=0;
  fmn_global.sketchc=0;
  fmn_global.wind_dir=0;
  fmn_global.facedir_gsbit_horz=0;
  fmn_global.facedir_gsbit_vert=0;
  fmn_global.saveto=0;
  
  bigpc.map_callbackc=0;
}
 
static int bigpc_cb_pick_sprite(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  *(const void**)userdata=v;
  return c;
}

static void bigpc_add_door(uint8_t cellp,uint16_t gsbit,uint16_t dstmapid,uint8_t dstx,uint8_t dsty) {
  if (fmn_global.doorc>FMN_DOOR_LIMIT) return;
  struct fmn_door *door=fmn_global.doorv+fmn_global.doorc++;
  door->x=cellp%FMN_COLC;
  door->y=cellp/FMN_COLC;
  door->mapid=dstmapid;
  if (dstx>=FMN_COLC) { // eg buried_treasure
    door->dstx=dstx;
    door->dsty=dsty;
  } else if (dsty>=FMN_ROWC) {
    door->dstx=-1;
    door->dsty=-1;
  } else {
    door->dstx=dstx;
    door->dsty=dsty;
  }
  door->extra=gsbit;
  // Expose buried doors if we got them already:
  if (door->extra&&door->mapid&&fmn_gs_get_bit(door->extra)) {
    fmn_global.map[cellp]=0x3f;
  }
}

static void bigpc_add_transmogrify(uint8_t cellp,uint8_t flags) {
  bigpc_add_door(cellp,0,0,flags&0xc0,flags&0x3f);
}

static void bigpc_add_sketch(uint8_t cellp,uint32_t bits) {
  if (fmn_global.sketchc>FMN_SKETCH_LIMIT) return;
  struct fmn_sketch *sketch=fmn_global.sketchv+fmn_global.sketchc++;
  sketch->x=cellp%FMN_COLC;
  sketch->y=cellp/FMN_COLC;
  sketch->bits=bits;
  sketch->time=0;
}

static void bigpc_add_callback(uint8_t evid,uint16_t cbid,uint8_t param) {
  if (bigpc.map_callbackc>=bigpc.map_callbacka) {
    int na=bigpc.map_callbacka+8;
    if (na>INT_MAX/sizeof(struct bigpc_map_callback)) return;
    void *nv=realloc(bigpc.map_callbackv,sizeof(struct bigpc_map_callback)*na);
    if (!nv) return;
    bigpc.map_callbackv=nv;
    bigpc.map_callbacka=na;
  }
  struct bigpc_map_callback *cb=bigpc.map_callbackv+bigpc.map_callbackc++;
  cb->evid=evid;
  cb->cbid=cbid;
  cb->param=param;
}

static void bigpc_load_cellphysics() {
  const void *serial=0;
  int serialc=fmn_datafile_for_each_of_id(bigpc.datafile,FMN_RESTYPE_TILEPROPS,fmn_global.maptsid,bigpc_cb_pick_sprite,&serial);
  if (serialc>sizeof(fmn_global.cellphysics)) serialc=sizeof(fmn_global.cellphysics);
  else if (serialc<0) serialc=0;
  memcpy(fmn_global.cellphysics,serial,serialc);
  if (serialc<sizeof(fmn_global.cellphysics)) memset(fmn_global.cellphysics+serialc,0,sizeof(fmn_global.cellphysics)-serialc);
}

void bigpc_play_song(uint8_t songid,uint8_t loop) {
  if (!songid) return; // 0 means "don't change", as opposed to "nothing" (TODO not sure that's correct)
  if (songid==fmn_global.songid) return;
  fmn_global.songid=songid;
  const void *serial=0;
  int serialc=fmn_datafile_get_any(&serial,bigpc.datafile,FMN_RESTYPE_SONG,songid);
  if (serialc<0) serialc=0;
  if (bigpc_audio_lock(bigpc.audio)>=0) {
    bigpc_synth_play_song(bigpc.synth,serial,serialc,0,loop);
    bigpc_audio_unlock(bigpc.audio);
  }
}

static void bigpc_autobloom_plants() {
  int dirty=0;
  struct fmn_plant *plant=fmn_global.plantv;
  int i=fmn_global.plantc;
  for (;i-->0;plant++) {
    if (plant->state!=FMN_PLANT_STATE_GROW) {
    } else if (!plant->flower_time) {
    } else if (plant->flower_time>bigpc.clock.last_game_time_ms) {
    } else {
      plant->state=FMN_PLANT_STATE_FLOWER;
      dirty=1;
    }
  }
  if (dirty) {
    fmstore_read_plants_from_globals(bigpc.fmstore,bigpc.mapid);
  }
}

/* Load map: dispatch one command.
 */
 
struct fmn_load_map_context {
  int use_initial_sketches;
  void (*cb_spawn)(
    int8_t x,int8_t y,
    uint16_t spriteid,uint8_t arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3,
    const uint8_t *cmdv,uint16_t cmdc
  );
};
 
static int fmn_load_map_cb_command(uint8_t opcode,const uint8_t *arg,int argc,void *userdata) {
  struct fmn_load_map_context *ctx=userdata;
  switch (opcode) {
      
      case 0x20: bigpc_play_song(arg[0],1); break;
      case 0x21: {
          fmn_global.maptsid=arg[0]; 
          bigpc_load_cellphysics();
        } break;
      case 0x22: bigpc.saveto_recent=fmn_global.saveto=arg[0]; break;
      case 0x23: if (fmn_global.wind_dir=arg[0]) fmn_global.wind_time=86400.0f; else fmn_global.wind_time=0.0f; break;
      case 0x24: fmn_global.mapflag=arg[0]; break;
      
      case 0x40: fmn_global.neighborw=(arg[0]<<8)|arg[1]; break;
      case 0x41: fmn_global.neighbore=(arg[0]<<8)|arg[1]; break;
      case 0x42: fmn_global.neighborn=(arg[0]<<8)|arg[1]; break;
      case 0x43: fmn_global.neighbors=(arg[0]<<8)|arg[1]; break;
      case 0x44: bigpc_add_transmogrify(arg[0],arg[1]); break;
      case 0x45: fmn_global.herostartp=arg[0]; if (arg[1]) bigpc.saveto_recent=arg[1]; break;
      
      case 0x60: bigpc_add_door(arg[0],0,(arg[1]<<8)|arg[2],arg[3]%FMN_COLC,arg[3]/FMN_COLC); break;
      case 0x61: if (ctx->use_initial_sketches) bigpc_add_sketch(arg[0],(arg[1]<<16)|(arg[2]<<8)|arg[3]); break;
      case 0x62: bigpc_add_door(arg[0],(arg[1]<<16)|arg[2],0,0x30,arg[3]); break;
      case 0x63: bigpc_add_callback(arg[0],(arg[1]<<8)|arg[2],arg[3]); break;
      case 0x64: bigpc_add_door(arg[0],(arg[1]<<16)|arg[2],0,0x20,0); break;
      case 0x65: fmn_global.facedir_gsbit_horz=(arg[0]<<8)|arg[1]; fmn_global.facedir_gsbit_vert=(arg[2]<<8)|arg[3]; break;
      
      case 0x80: if (ctx->cb_spawn) {
          int8_t x=arg[0]%FMN_COLC,y=arg[0]/FMN_COLC;
          uint16_t spriteid=(arg[1]<<8)|arg[2];
          uint8_t arg0=arg[3],arg1=arg[4],arg2=arg[5],arg3=0;
          const uint8_t *cmdv=0;
          int cmdc=fmn_datafile_for_each_of_id(bigpc.datafile,FMN_RESTYPE_SPRITE,spriteid,bigpc_cb_pick_sprite,&cmdv);
          if ((cmdc<0)||(cmdc>0xffff)) cmdc=0;
          ctx->cb_spawn(x,y,spriteid,arg0,arg1,arg2,arg3,cmdv,cmdc);
        } break;
        
      case 0x81: bigpc_add_door(arg[0],(arg[1]<<8)|arg[2],(arg[3]<<8)|arg[4],arg[5]%FMN_COLC,arg[5]/FMN_COLC); break;
  }
  return 0;
}

/* Load map.
 */
 
int8_t fmn_load_map(
  uint16_t mapid,
  void (*cb_spawn)(
    int8_t x,int8_t y,
    uint16_t spriteid,uint8_t arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3,
    const uint8_t *cmdv,uint16_t cmdc
  )
) {
  bigpc.savedgame_suppress=0;
  
  const uint8_t *serial=0;
  int serialc=fmn_datafile_get_any(&serial,bigpc.datafile,FMN_RESTYPE_MAP,mapid);
  if (serialc<FMN_COLC*FMN_ROWC) return -1;
  
  fmstore_read_plants_from_globals(bigpc.fmstore,bigpc.mapid);
  fmstore_read_sketches_from_globals(bigpc.fmstore,bigpc.mapid);
  
  bigpc.mapid=mapid;
  memcpy(fmn_global.map,serial,FMN_COLC*FMN_ROWC);
  bigpc_clear_map_commands();
  
  // Apply plants and sketches from session store before decoding the map's commands.
  // This is important for sketches: We want the map-command ones only if there are none from the store.
  struct fmn_load_map_context ctx={
    .use_initial_sketches=1,
    .cb_spawn=cb_spawn,
  };
  fmstore_write_plants_to_globals(bigpc.fmstore,bigpc.mapid);
  fmstore_write_sketches_to_globals(bigpc.fmstore,bigpc.mapid);
  if (fmn_global.sketchc) ctx.use_initial_sketches=0;

  fmn_map_for_each_command(serial,serialc,fmn_load_map_cb_command,&ctx);
  
  if (!(fmn_global.mapflag&FMN_MAPFLAG_INDOORS)) bigpc_autobloom_plants();
  
  bigpc_savedgame_dirty();
  
  return 1;
}

int16_t fmn_get_map(void *dst,int16_t dsta,uint16_t mapid) {
  const void *src=0;
  int srcc=fmn_datafile_get_any(&src,bigpc.datafile,FMN_RESTYPE_MAP,mapid);
  if (srcc<1) return 0;
  if (srcc<=dsta) memcpy(dst,src,srcc);
  return srcc;
}

/* Add a plant.
 */
 
int8_t fmn_add_plant(uint16_t x,uint16_t y) {

  // The tile should be 0x0f. Proceed anyway if it's not, but if it is, reset to 0x00.
  if ((x<0)||(y<0)||(x>=FMN_COLC)||(y>=FMN_ROWC)) return -1;

  // Reject if there's one here already.
  { struct fmn_plant *already=fmn_global.plantv;
    int i=fmn_global.plantc;
    for (;i-->0;already++) {
      if (already->state==FMN_PLANT_STATE_NONE) continue;
      if (already->x!=x) continue;
      if (already->y!=y) continue;
      return -1;
    }
  }
  
  // Find an available plant.
  struct fmn_plant *plant=0;
  if (fmn_global.plantc<FMN_PLANT_LIMIT) {
    plant=fmn_global.plantv+fmn_global.plantc++;
  } else {
    struct fmn_plant *q=fmn_global.plantv;
    int i=fmn_global.plantc;
    for (;i-->0;q++) {
      if ((q->state==FMN_PLANT_STATE_NONE)||(q->state==FMN_PLANT_STATE_DEAD)) {
        plant=q;
        break;
      }
    }
  }
  if (!plant) return -1;
  
  if (fmn_global.map[y*FMN_COLC+x]==0x0f) {
    fmn_global.map[y*FMN_COLC+x]=0x00;
  }
  
  plant->x=x;
  plant->y=y;
  plant->state=FMN_PLANT_STATE_SEED;
  plant->fruit=0;
  plant->flower_time=0;
  
  fmstore_read_plants_from_globals(bigpc.fmstore,bigpc.mapid);
  
  bigpc_savedgame_dirty();
  
  return 0;
}

/* Begin a sketch.
 */
 
uint32_t fmn_begin_sketch(uint16_t x,uint16_t y) {

  // If there's one at this cell already, cool, we'll use it.
  struct fmn_sketch *sketch=0;
  { struct fmn_sketch *q=fmn_global.sketchv;
    int i=fmn_global.sketchc;
    for (;i-->0;q++) {
      if (q->x!=x) continue;
      if (q->y!=y) continue;
      sketch=q;
      break;
    }
  }
  
  // Find an available sketch.
  if (!sketch) {
    if (fmn_global.sketchc<FMN_SKETCH_LIMIT) {
      sketch=fmn_global.sketchv+fmn_global.sketchc++;
    } else {
      // Yoink one if its bits are zero (what's it still doing there?). But otherwise we have to reject.
      struct fmn_sketch *q=fmn_global.sketchv;
      int i=fmn_global.sketchc;
      for (;i-->0;q++) {
        if (!q->bits) {
          sketch=q;
          break;
        }
      }
    }
    if (!sketch) return 0xffffffff;
    sketch->x=x;
    sketch->y=y;
    sketch->pad=0;
    sketch->bits=0;
  }
  
  sketch->time=bigpc.clock.last_game_time_ms;
  
  fmstore_read_sketches_from_globals(bigpc.fmstore,bigpc.mapid);
  
  return sketch->bits;
}

/* Audio events.
 */
 
void fmn_sound_effect(uint16_t sfxid) {
  if (!bigpc_check_sound_blackout(sfxid)) return;
  if (bigpc_audio_lock(bigpc.audio)>=0) {
    uint8_t chid=0x0f;
    uint8_t opcode=0x98;
    uint8_t a=sfxid;
    uint8_t b=0;
    bigpc_synth_event(bigpc.synth,chid,opcode,a,b);
    bigpc_audio_unlock(bigpc.audio);
  }
}

void fmn_synth_event(uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  if (bigpc_audio_lock(bigpc.audio)>=0) {
    bigpc_synth_event(bigpc.synth,chid,opcode,a,b);
    bigpc_audio_unlock(bigpc.audio);
  }
}

void fmn_play_song(uint8_t songid,uint8_t loop) {
  bigpc_play_song(songid,loop);
}

/* Get a string resource.
 */
 
uint8_t fmn_get_string(char *dst,uint8_t dsta,uint16_t id) {
  const uint16_t langid=bigpc.config.lang;
  const char *src=0;
  int srcc=fmn_datafile_get_qualified(&src,bigpc.datafile,FMN_RESTYPE_STRING,langid,id);
  if ((srcc<1)&&(langid!=0x656e)) srcc=fmn_datafile_get_qualified(&src,bigpc.datafile,FMN_RESTYPE_STRING,0x656e,id);
  if (srcc<0) srcc=0;
  if (srcc>0xff) srcc=0xff;
  if (srcc<=dsta) {
    memcpy(dst,src,srcc);
    if (srcc<dsta) dst[srcc]=0;
  }
  return srcc;
}

/* Trigger map callbacks.
 */
 
void fmn_map_callbacks(uint8_t evid,void (*cb)(uint16_t cbid,uint8_t param,void *userdata),void *userdata) {
  struct bigpc_map_callback *reg=bigpc.map_callbackv;
  int i=bigpc.map_callbackc;
  for (;i-->0;reg++) {
    if (reg->evid!=evid) continue;
    cb(reg->cbid,reg->param,userdata);
  }
}

/* Saved game.
 */
 
uint8_t fmn_has_saved_game() {
  // We'll have read it already, at bigpc_savedgame_init.
  if (bigpc.savedgame_serialc) return 1;
  return 0;
}

void fmn_delete_saved_game() {
  bigpc_savedgame_delete();
}

void fmn_saved_game_dirty() {
  bigpc_savedgame_dirty();
}

/* Load saved game.
 */

uint16_t fmn_load_saved_game() {
  if (!bigpc.savedgame_serialc) return 0;
  if (bigpc_savedgame_validate(bigpc.savedgame_serial,bigpc.savedgame_serialc)<0) {
    fprintf(stderr,"%s: Have %d-byte saved game, but it failed validation!\n",bigpc.savedgame_path,bigpc.savedgame_serialc);
    return 0;
  }
  bigpc.savedgame_dirty=0;
  return bigpc_savedgame_load(bigpc.savedgame_serial,bigpc.savedgame_serialc);
}

uint8_t fmn_is_demo() {
  return (fmn_datafile_count_type(bigpc.datafile,FMN_RESTYPE_MAP)<100)?1:0;
}

/* Settings.
 */
 
void fmn_platform_get_settings(struct fmn_platform_settings *settings) {
  memcpy(settings,&bigpc.settings,sizeof(struct fmn_platform_settings));
}

void fmn_platform_set_settings(struct fmn_platform_settings *settings) {
  if (settings->fullscreen_enable!=bigpc.settings.fullscreen_enable) {
    bigpc_video_set_fullscreen(bigpc.video,settings->fullscreen_enable);
    bigpc.settings.fullscreen_enable=settings->fullscreen_enable;
  }
  if (settings->music_enable!=bigpc.settings.music_enable) {
    if (bigpc.synth->type->enable_music) {
      if (bigpc_audio_lock(bigpc.audio)>=0) {
        bigpc.synth->type->enable_music(bigpc.synth,settings->music_enable);
        bigpc_audio_unlock(bigpc.audio);
        bigpc.settings.music_enable=settings->music_enable;
      }
    }
  }
  if (settings->language!=bigpc.settings.language) {
    bigpc.config.lang=settings->language;
    bigpc.settings.language=settings->language;
    fmn_language_changed();
  }
}

uint16_t fmn_platform_get_next_language(uint16_t language) {
  if (!bigpc.langc) return language;
  if (!language) return bigpc.langv[0];
  int i=0; for (;i<bigpc.langc;i++) {
    if (bigpc.langv[i]!=language) continue;
    if (i+1<bigpc.langc) return bigpc.langv[i+1];
    return bigpc.langv[0];
  }
  return language;
}

uint16_t fmn_platform_get_prev_language(uint16_t language) {
  if (!bigpc.langc) return language;
  if (!language) return bigpc.langv[0];
  int i=0; for (;i<bigpc.langc;i++) {
    if (bigpc.langv[i]!=language) continue;
    if (i) return bigpc.langv[i-1];
    return bigpc.langv[bigpc.langc-1];
  }
  return language;
}

static int bigpc_rebuild_language_list_cb(uint16_t lang,void *userdata) {
  uint8_t a=lang>>8,b=lang;
  if ((a<'a')||(a>'z')) return 0;
  if ((b<'a')||(b>'z')) return 0;
  const uint16_t *existing=bigpc.langv;
  int i=bigpc.langc;
  for (;i-->0;existing++) if (*existing==lang) return 0;
  if (bigpc.langc>=bigpc.langa) {
    int na=bigpc.langa+8;
    if (na>INT_MAX/sizeof(uint16_t)) return -1;
    void *nv=realloc(bigpc.langv,sizeof(uint16_t)*na);
    if (!nv) return -1;
    bigpc.langv=nv;
    bigpc.langa=na;
  }
  bigpc.langv[bigpc.langc++]=lang;
  return 0;
}

void bigpc_rebuild_language_list() {
  bigpc.langc=0;
  fmn_datafile_for_each_qualifier(bigpc.datafile,FMN_RESTYPE_STRING,bigpc_rebuild_language_list_cb,0);
}

/* Input configuration.
 */
 
struct fmn_platform_get_input_device_by_index_context {
  uint8_t p;
  struct bigpc_input_driver **driver;
  int devid;
};
 
static int fmn_platform_get_input_device_by_index_cb(struct bigpc_input_driver *driver,int devid,void *userdata) {
  struct fmn_platform_get_input_device_by_index_context *ctx=userdata;
  if (ctx->p--) return 0;
  *(ctx->driver)=driver;
  ctx->devid=devid;
  return 1;
}
 
static int bigpc_get_input_device_by_index(struct bigpc_input_driver **driver,uint8_t p) {
  // If a system keyboard is present, it is always the first device.
  if (bigpc.devid_keyboard) {
    if (!p--) {
      *driver=0;
      return bigpc.devid_keyboard;
    }
  }
  // Generic devices.
  struct fmn_platform_get_input_device_by_index_context ctx={
    .p=p,
    .driver=driver,
    .devid=0,
  };
  int inputi=0;
  for (;inputi<bigpc.inputc;inputi++) {
    struct bigpc_input_driver *driver=bigpc.inputv[inputi];
    if (!driver->type->for_each_device) continue;
    int result=driver->type->for_each_device(driver,fmn_platform_get_input_device_by_index_cb,&ctx);
    if (result) return ctx.devid;
  }
  return 0;
}

uint8_t fmn_platform_get_input_device_name(char *dst,uint8_t dsta,uint8_t p) {
  struct bigpc_input_driver *driver=0;
  int devid=bigpc_get_input_device_by_index(&driver,p);
  if (devid<1) return 0;
  if (driver) {
    uint16_t vid=0,pid=0;
    char fallback[9];
    const char *name=bigpc_input_device_get_ids(&vid,&pid,driver,devid);
    if (!name||!name[0]) {
      if (dsta>=9) {
        dst[0]="0123456789abcdef"[(vid>>12)&0x0f];
        dst[1]="0123456789abcdef"[(vid>> 8)&0x0f];
        dst[2]="0123456789abcdef"[(vid>> 4)&0x0f];
        dst[3]="0123456789abcdef"[(vid    )&0x0f];
        dst[4]=':';
        dst[5]="0123456789abcdef"[(pid>>12)&0x0f];
        dst[6]="0123456789abcdef"[(pid>> 8)&0x0f];
        dst[7]="0123456789abcdef"[(pid>> 4)&0x0f];
        dst[8]="0123456789abcdef"[(pid    )&0x0f];
      } else memset(dst,'?',dsta);
      return 9;
    }
    int namec=0; while (name[namec]&&(namec<dsta)) namec++;
    memcpy(dst,name,namec);
    return namec;
  } else {
    const char src[]="System Keyboard";
    int srcc=sizeof(src)-1;
    if (srcc<=dsta) memcpy(dst,src,srcc);
    else memcpy(dst,src,dsta);
    return srcc;
  }
}

void fmn_platform_begin_input_configuration(uint8_t p) {
  fmn_log("%s p=%d",__func__,p);
  if (
    (bigpc.incfg_devid=bigpc_get_input_device_by_index(&bigpc.incfg_driver,p))&&
    (inmgr_begin_live_config(bigpc.inmgr,bigpc.incfg_devid)>=0)
  ) {
    fmn_log("...ok begin config devid=%d",bigpc.incfg_devid);
    bigpc.incfg_state=FMN_INCFG_STATE_READY;
    bigpc.incfg_p=p;
    bigpc.incfg_btnid=FMN_INPUT_LEFT;
    bigpc.incfg_incoming_btnid=0;
    //bigpc.input_state=0;
  } else {
    fmn_log("...invalid device index");
    bigpc.incfg_state=FMN_INCFG_STATE_NONE;
    bigpc.incfg_p=0;
    bigpc.incfg_btnid=0;
    bigpc.incfg_driver=0;
    bigpc.incfg_incoming_btnid=0;
  }
}

void fmn_platform_cancel_input_configuration() {
  fmn_log("%s",__func__);
  bigpc.incfg_state=FMN_INCFG_STATE_NONE;
  bigpc.incfg_p=0;
  bigpc.incfg_btnid=0;
  bigpc.incfg_driver=0;
  bigpc.incfg_devid=0;
  bigpc.incfg_incoming_btnid=0;
  inmgr_end_live_config(bigpc.inmgr);
}

uint8_t fmn_platform_get_input_configuration_state(uint8_t *p,uint8_t *btnid) {
  *p=bigpc.incfg_p;
  *btnid=bigpc.incfg_btnid;
  return bigpc.incfg_state;
}

/* fmn_find_map_command, fmn_find_direction_to_item, fmn_find_direction_to_map
 * are part of the platform API, but have their own home in bigpc_map_analysis.c.
 * fmn_log and fmn_log_event live in bigpc_log.c.
 * fmn_video_* and fmn_draw_* in bigpc_video_api.c.
 */

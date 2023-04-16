#include "bigpc_internal.h"
#include "app/fmn_game.h"
#include <stdio.h>
#include <stdarg.h>

/* Log.
 */
 
void fmn_log(const char *fmt,...) {
  if (!fmt||!fmt[0]) return;
  char buf[256];
  va_list vargs;
  va_start(vargs,fmt);
  int bufc=vsnprintf(buf,sizeof(buf),fmt,vargs);
  if ((bufc<0)||(bufc>=sizeof(buf))) { // sic >= not > because vsnprintf can negative-terminate
    fprintf(stderr,"fmn_log: Message too long. Format string: %.100s\n",fmt);
  } else {
    fprintf(stderr,"%.*s\n",bufc,buf);
  }
}

/* Hard abort.
 */
 
void fmn_abort() {
  bigpc.sigc++;
}

/* Begin modal menu.
 */
 
void _fmn_begin_menu(int prompt,...) {
  
  if (bigpc.menuc>=bigpc.menua) {
    int na=bigpc.menua+8;
    if (na>INT_MAX/sizeof(void*)) return;
    void *nv=realloc(bigpc.menuv,sizeof(void*)*na);
    if (!nv) return;
    bigpc.menuv=nv;
    bigpc.menua=na;
  }
  
  struct bigpc_menu *menu=0;
  if (prompt>0) {
    menu=bigpc_menu_new_prompt(prompt);
  } else switch (prompt) {
    case FMN_MENU_PAUSE: menu=bigpc_menu_new_PAUSE(); break;
    case FMN_MENU_CHALK: menu=bigpc_menu_new_CHALK(); break;
    case FMN_MENU_TREASURE: menu=bigpc_menu_new_TREASURE(); break;
    case FMN_MENU_VICTORY: menu=bigpc_menu_new_VICTORY(); break;
    case FMN_MENU_GAMEOVER: menu=bigpc_menu_new_GAMEOVER(); break;
  }
  if (!menu) return;
  
  va_list vargs;
  va_start(vargs,prompt);
  while (1) {
    int stringid=va_arg(vargs,int);
    if (!stringid) break;
    void (*cb)()=va_arg(vargs,void*);
    if (bigpc_menu_add_option(menu,stringid,cb)<0) {
      bigpc_menu_del(menu);
      return;
    }
  }
  if (bigpc_menu_ready(menu)<0) {
    bigpc_menu_del(menu);
    return;
  }
  
  bigpc.menuv[bigpc.menuc++]=menu;
  
  // VICTORY and GAMEOVER menus also entail a song change, which the client can't do.
  switch (prompt) {
    case FMN_MENU_VICTORY: bigpc_play_song(7); break;
    case FMN_MENU_GAMEOVER: bigpc_play_song(6); break;
  }
}

/* Transitions.
 */
 
void fmn_prepare_transition(int transition) {
  bigpc_render_transition(bigpc.render,transition);
}

void fmn_commit_transition() {
  bigpc_render_transition(bigpc.render,FMN_TRANSITION_COMMIT);
}

void fmn_cancel_transition() {
  bigpc_render_transition(bigpc.render,FMN_TRANSITION_CANCEL);
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
  fmn_global.mapdark=0;
  fmn_global.indoors=0;
  fmn_global.herostartp=0;
  fmn_global.doorc=0;
  fmn_global.plantc=0;
  fmn_global.sketchc=0;
  fmn_global.blowback=0;
  fmn_global.wind_dir=0;
  
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
  door->dstx=dstx;
  door->dsty=dsty;
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

void bigpc_play_song(uint8_t songid) {
  if (!songid) return; // 0 means "don't change", as opposed to "nothing" (TODO not sure that's correct)
  if (songid==fmn_global.songid) return;
  fmn_global.songid=songid;
  const void *serial=0;
  int serialc=fmn_datafile_get_any(&serial,bigpc.datafile,FMN_RESTYPE_SONG,songid);
  if (serialc<0) serialc=0;
  if (bigpc_audio_lock(bigpc.audio)>=0) {
    bigpc_synth_play_song(bigpc.synth,serial,serialc,0);
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

/* Load map.
 */
 
static int fmn_load_map_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  *(const void**)userdata=v;
  return c;
}
 
int8_t fmn_load_map(
  uint16_t mapid,
  void (*cb_spawn)(
    int8_t x,int8_t y,
    uint16_t spriteid,uint8_t arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3,
    const uint8_t *cmdv,uint16_t cmdc
  )
) {
  const uint8_t *serial=0;
  int serialp=FMN_COLC*FMN_ROWC;
  int serialc=fmn_datafile_for_each_of_id(bigpc.datafile,FMN_RESTYPE_MAP,mapid,fmn_load_map_cb,&serial);
  if (serialc<serialp) return -1;
  
  fmstore_read_plants_from_globals(bigpc.fmstore,bigpc.mapid);
  fmstore_read_sketches_from_globals(bigpc.fmstore,bigpc.mapid);
  
  bigpc.mapid=mapid;
  memcpy(fmn_global.map,serial,serialp);
  bigpc_clear_map_commands();
  
  // Apply plants and sketches from session store before decoding the map's commands.
  // This is important for sketches: We want the map-command ones only if there are none from the store.
  int use_initial_sketches=1;
  fmstore_write_plants_to_globals(bigpc.fmstore,bigpc.mapid);
  fmstore_write_sketches_to_globals(bigpc.fmstore,bigpc.mapid);
  if (fmn_global.sketchc) use_initial_sketches=0;

  while (serialp<serialc) {
    uint8_t opcode=serial[serialp++];
    int paylen;
    switch (opcode&0xe0) {
      case 0x00: paylen=0; break;
      case 0x20: paylen=1; break;
      case 0x40: paylen=2; break;
      case 0x60: paylen=4; break;
      case 0x80: paylen=6; break;
      case 0xa0: paylen=8; break;
      case 0xc0: if (serialp>=serialc) paylen=opcode=0; else paylen=serial[serialp++]; break;
      default: paylen=opcode=0;
    }
    if (!opcode) break;
    if (serialp>serialc-paylen) break;
    const uint8_t *arg=serial+serialp;
    serialp+=paylen;
    switch (opcode) {
    
      case 0x01: fmn_global.mapdark=1; break;
      case 0x02: fmn_global.indoors=1; break;
      case 0x03: fmn_global.blowback=1; break;
      
      case 0x20: bigpc_play_song(arg[0]); break;
      case 0x21: fmn_global.maptsid=arg[0]; break;
      case 0x22: fmn_global.herostartp=arg[0]; break;
      case 0x23: if (fmn_global.wind_dir=arg[0]) fmn_global.wind_time=86400.0f; else fmn_global.wind_time=0.0f; break;
      
      case 0x40: fmn_global.neighborw=(arg[0]<<8)|arg[1]; break;
      case 0x41: fmn_global.neighbore=(arg[0]<<8)|arg[1]; break;
      case 0x42: fmn_global.neighborn=(arg[0]<<8)|arg[1]; break;
      case 0x43: fmn_global.neighbors=(arg[0]<<8)|arg[1]; break;
      case 0x44: bigpc_add_transmogrify(arg[0],arg[1]); break;
      
      case 0x60: bigpc_add_door(arg[0],0,(arg[1]<<8)|arg[2],arg[3]%FMN_COLC,arg[3]/FMN_COLC); break;
      case 0x61: if (use_initial_sketches) bigpc_add_sketch(arg[0],(arg[1]<<16)|(arg[2]<<8)|arg[3]); break;
      case 0x62: bigpc_add_door(arg[0],(arg[1]<<16)|arg[2],0,0x30,arg[3]); break;
      case 0x63: bigpc_add_callback(arg[0],(arg[1]<<8)|arg[2],arg[3]); break;
      
      case 0x80: if (cb_spawn) {
          int8_t x=arg[0]%FMN_COLC,y=arg[0]/FMN_COLC;
          uint16_t spriteid=(arg[1]<<8)|arg[2];
          uint8_t arg0=arg[3],arg1=arg[4],arg2=arg[5],arg3=0;
          const uint8_t *cmdv=0;
          int cmdc=fmn_datafile_for_each_of_id(bigpc.datafile,FMN_RESTYPE_SPRITE,spriteid,bigpc_cb_pick_sprite,&cmdv);
          if ((cmdc<0)||(cmdc>0xffff)) cmdc=0;
          cb_spawn(x,y,spriteid,arg0,arg1,arg2,arg3,cmdv,cmdc);
        } break;
        
      case 0x81: bigpc_add_door(arg[0],(arg[1]<<8)|arg[2],(arg[3]<<8)|arg[4],arg[5]%FMN_COLC,arg[5]/FMN_COLC); break;
    }
  }
  
  bigpc_load_cellphysics();
  bigpc_autobloom_plants();
  bigpc_render_map_dirty(bigpc.render);
  return 1;
}

/* When the client indicates map dirty, we check plants for required but unset flower_time.
 * I guess this ought to be done by the client on his own, but this is how I did it for web, so hey don't make waves.
 */
 
static void bigpc_set_flower_times() {
  struct fmn_plant *plant=fmn_global.plantv;
  int i=fmn_global.plantc;
  for (;i-->0;plant++) {
    if (plant->state!=FMN_PLANT_STATE_GROW) continue;
    if (plant->flower_time) continue;
    plant->flower_time=bigpc.clock.last_game_time_ms+BIGPC_PLANT_FLOWER_TIME;
  }
}

/* Map dirty.
 */
 
void fmn_map_dirty() {
  bigpc_set_flower_times();
  bigpc_render_map_dirty(bigpc.render);
  fmstore_read_plants_from_globals(bigpc.fmstore,bigpc.mapid);
  fmstore_read_sketches_from_globals(bigpc.fmstore,bigpc.mapid);
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
  
  bigpc_render_map_dirty(bigpc.render);
  fmstore_read_plants_from_globals(bigpc.fmstore,bigpc.mapid);
  return 0;
}

/* Begin a sketch.
 */
 
static void fmn_cb_sketch() {
  if (!bigpc.sketch_in_progress) return;
  if (bigpc.menuc<1) return;
  struct bigpc_menu *menu=bigpc.menuv[bigpc.menuc-1];
  if (menu->prompt!=FMN_MENU_CHALK) return;
  bigpc.sketch_in_progress->bits=menu->extra[0];
  bigpc.sketch_in_progress=0;
  bigpc_render_map_dirty(bigpc.render);
  fmstore_read_sketches_from_globals(bigpc.fmstore,bigpc.mapid);
}
 
int8_t fmn_begin_sketch(uint16_t x,uint16_t y) {

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
    if (!sketch) return -1;
    sketch->x=x;
    sketch->y=y;
    sketch->pad=0;
    sketch->bits=0;
  }
  
  sketch->time=bigpc.clock.last_game_time_ms;
  
  bigpc.sketch_in_progress=sketch;
  fmn_begin_menu(FMN_MENU_CHALK,sketch->bits|0x80000000,fmn_cb_sketch);
  fmstore_read_sketches_from_globals(bigpc.fmstore,bigpc.mapid);
  
  return 0;
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

/* Get a string resource.
 */
 
uint8_t fmn_get_string(char *dst,uint8_t dsta,uint16_t id) {
  const char *src=0;
  int srcc=fmn_datafile_get_any(&src,bigpc.datafile,FMN_RESTYPE_STRING,id);
  if (srcc<0) srcc=0;
  if (srcc>0xff) srcc=0xff;
  if (srcc<=dsta) {
    memcpy(dst,src,srcc);
    if (srcc<dsta) dst[srcc]=0;
  }
  return srcc;
}

/* Find a command in nearby maps, for the compass.
 */
 
uint8_t fmn_find_map_command(int16_t *xy,uint8_t mask,const uint8_t *v) {
  fmn_log("TODO %s",__func__);
  xy[0]=xy[1]=0;
  return 0;
}

/* Directions to item or map, for the crow.
 */
 
uint8_t fmn_find_direction_to_item(uint8_t itemid) {
  fmn_log("TODO %s",__func__);
  return 0;
}

uint8_t fmn_find_direction_to_map(uint16_t mapid) {
  fmn_log("TODO %s",__func__);
  return 0;
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

#include "bigpc_internal.h"
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
  fmn_log("TODO %s",__func__);
  bigpc.sigc++;
}

/* Begin modal menu.
 */
 
void _fmn_begin_menu(int prompt,...) {
  fmn_log("TODO %s %d",__func__,prompt);
  
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
    case FMN_MENU_CHALK: menu=bigpc_menu_new_CHALK(0); break;
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
  //TODO register callbacks
}

static void bigpc_load_cellphysics() {
  const void *serial=0;
  int serialc=fmn_datafile_for_each_of_id(bigpc.datafile,FMN_RESTYPE_TILEPROPS,fmn_global.maptsid,bigpc_cb_pick_sprite,&serial);
  if (serialc>sizeof(fmn_global.cellphysics)) serialc=sizeof(fmn_global.cellphysics);
  else if (serialc<0) serialc=0;
  memcpy(fmn_global.cellphysics,serial,serialc);
  if (serialc<sizeof(fmn_global.cellphysics)) memset(fmn_global.cellphysics+serialc,0,sizeof(fmn_global.cellphysics)-serialc);
}

static void bigpc_play_song(uint8_t songid) {
  if (!songid) return; // 0 means "don't change", as opposed to "nothing" (TODO not sure that's correct)
  if (songid==fmn_global.songid) return;
  fmn_global.songid=songid;
  const void *serial=0;
  int serialc=fmn_datafile_get_any(&serial,bigpc.datafile,FMN_RESTYPE_SONG,songid);
  if (serialc<0) serialc=0;
  fprintf(stderr,"Play song %d, %d bytes\n",songid,serialc);
  bigpc_synth_play_song(bigpc.synth,serial,serialc,0);
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
  
  memcpy(fmn_global.map,serial,serialp);
  
  bigpc_clear_map_commands();
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
      case 0x23: fmn_global.wind_dir=arg[0]; break;
      
      case 0x40: fmn_global.neighborw=(arg[0]<<8)|arg[1]; break;
      case 0x41: fmn_global.neighbore=(arg[0]<<8)|arg[1]; break;
      case 0x42: fmn_global.neighborn=(arg[0]<<8)|arg[1]; break;
      case 0x43: fmn_global.neighbors=(arg[0]<<8)|arg[1]; break;
      case 0x44: bigpc_add_transmogrify(arg[0],arg[1]); break;
      
      case 0x60: bigpc_add_door(arg[0],0,(arg[1]<<8)|arg[2],arg[3]%FMN_COLC,arg[3]/FMN_COLC); break;
      case 0x61: bigpc_add_sketch(arg[0],(arg[1]<<16)|(arg[2]<<8)|arg[3]); break;
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
  bigpc_render_map_dirty(bigpc.render);
  return 1;
}

/* Map dirty.
 */
 
void fmn_map_dirty() {
  bigpc_render_map_dirty(bigpc.render);
}

/* Add a plant.
 */
 
int8_t fmn_add_plant(uint16_t x,uint16_t y) {
  fmn_log("TODO %s (%d,%d)",__func__,x,y);
  return -1;
}

/* Begin a sketch.
 */
 
int8_t fmn_begin_sketch(uint16_t x,uint16_t y) {
  fmn_log("TODO %s (%d,%d)",__func__,x,y);
  return -1;
}

/* Audio events.
 */
 
void fmn_sound_effect(uint16_t sfxid) {
  fmn_log("TODO %s %d",__func__,sfxid);
}

void fmn_synth_event(uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  fmn_log("TODO %s %d 0x%02x 0x%02x 0x%02x",__func__,chid,opcode,a,b);
}

/* Get a string resource.
 */
 
uint8_t fmn_get_string(char *dst,uint8_t dsta,uint16_t id) {
  fmn_log("TODO %s %d",__func__,id);
  return 0;
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
  fmn_log("TODO %s",__func__);
}

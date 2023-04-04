#include "bigpc_internal.h"
#include <stdio.h>

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
}

/* Begin modal menu.
 */
 
void _fmn_begin_menu(int prompt,...) {
  fmn_log("TODO %s %d",__func__,prompt);
}

/* Transitions.
 */
 
void fmn_prepare_transition(int transition) {
  fmn_log("TODO %s %d",__func__,transition);
}

void fmn_commit_transition() {
  fmn_log("TODO %s",__func__);
}

void fmn_cancel_transition() {
  fmn_log("TODO %s",__func__);
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
  fmn_log("TODO %s %d",__func__,mapid);
  return 1;
  return -1;
}

/* Map dirty.
 */
 
void fmn_map_dirty() {
  fmn_log("TODO %s",__func__);
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

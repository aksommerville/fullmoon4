#include "app/fmn_platform.h"

int main(int argc,char **argv) {
  return 0;
}

void fmn_log(const char *fmt,...) {
}

void fmn_abort() {
}

void _fmn_begin_menu(int prompt,.../*int opt1,void (*cb1)(),...,int optN,void (*cbN)()*/) {
}

void fmn_prepare_transition(int transition) {
}

void fmn_commit_transition() {
}

void fmn_cancel_transition() {
}

int8_t fmn_load_map(
  uint16_t mapid,
  void (*cb_spawn)(
    int8_t x,int8_t y,
    uint16_t spriteid,uint8_t arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3,
    const uint8_t *cmdv,uint16_t cmdc
  )
) {
  return 0;
}

void fmn_map_dirty() {
}

int8_t fmn_add_plant(uint16_t x,uint16_t y) {
  return 0;
}

int8_t fmn_begin_sketch(uint16_t x,uint16_t y) {
  return 0;
}

void fmn_sound_effect(uint16_t sfxid) {
}

void fmn_synth_event(uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
}

uint8_t fmn_get_string(char *dst,uint8_t dsta,uint16_t id) {
  return 0;
}

uint8_t fmn_find_map_command(int16_t *xy,uint8_t mask,const uint8_t *v) {
  return 0;
}

uint8_t fmn_find_direction_to_item(uint8_t itemid) {
  return 0;
}

uint8_t fmn_find_direction_to_map(uint16_t mapid) {
  return 0;
}

void fmn_map_callbacks(uint8_t evid,void (*cb)(uint16_t cbid,uint8_t param,void *userdata),void *userdata) {
}

void fmn_log_event(const char *key,const char *fmt,...) {
}

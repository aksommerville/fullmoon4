#ifndef FMN_EATWQ_INTERNAL_H
#define FMN_EATWQ_INTERNAL_H

#include "fmn_eatwq.h"
#include "app/fmn_platform.h"

#define EATWQ_FB_W 160
#define EATWQ_FB_H 96

#define EATWQ_DROP_LIMIT 32
#define EATWQ_PLANT_LIMIT 16
#define EATWQ_BOOM_LIMIT 8

extern struct eatwq {
  uint8_t creditc;
  uint8_t hiscore;
  void (*cb_creditc)(uint8_t creditc,void *userdata);
  void (*cb_hiscore)(uint8_t hiscore,void *userdata);
  void *userdata;
  int running;
  
  int umbrella;
  int playtime; // frames; counts down
  int dx,dy; // dpad state: -1,0,1
  int flop; // nonzero for RIGHT. LEFT is default.
  int16_t herox,heroy;
  int dead;
  
  int score; // starts tallying at summary, after play
  int summaryttl;
  
  struct eatwq_drop {
    int16_t x,y;
    uint8_t tileid; // c2=rain; c3,c4=bomb
  } dropv[EATWQ_DROP_LIMIT];
  int dropc;
  
  struct eatwq_plant {
    int16_t x,y;
    uint8_t tileid; // d0=sprout; e2=poison; d1,d2,d3,d4=flower
  } plantv[EATWQ_PLANT_LIMIT];
  int plantc;
  
  struct eatwq_boom {
    int16_t x,y;
    uint8_t ttl;
  } boomv[EATWQ_BOOM_LIMIT];
  int boomc;
  
} eatwq;

#endif

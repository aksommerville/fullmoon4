#include "fmn_eatwq_internal.h"

/* Add vertices for a decimal integer.
 */
 
static inline uint8_t fmn_eatwq_tileid_for_digit(int digit) {
  if (digit<5) return 0xae +digit*16;
  return 0xaf+(digit-5)*16;
}
 
static int fmn_eatwq_render_decint(struct fmn_draw_mintile *vtxv,int vtxa,int16_t x,int16_t y,int v,int digitc) {
  if (digitc>vtxa) return 0;
  const int16_t glyphw=6;
  int p=digitc;
  x+=digitc*glyphw;
  while (p-->0) {
    struct fmn_draw_mintile *vtx=vtxv+p;
    vtx->x=x;
    vtx->y=y;
    vtx->tileid=fmn_eatwq_tileid_for_digit(v%10);
    vtx->xform=0;
    x-=glyphw;
    v/=10;
  }
  return digitc;
}

/* Running.
 */
 
static int fmn_eatwq_render_running(struct fmn_draw_mintile *vtxv,int vtxa) {
  int vtxc=0;
  struct fmn_draw_mintile *vtx;
  
  // Plants.
  struct eatwq_plant *plant=eatwq.plantv;
  int i=eatwq.plantc;
  for (;i-->0;plant++) {
    if (vtxc<vtxa) {
      vtx=vtxv+vtxc++;
      vtx->x=plant->x;
      vtx->y=plant->y;
      vtx->tileid=plant->tileid;
      vtx->xform=0;
    }
  }
  
  // Umbrella.
  if (eatwq.umbrella&&!eatwq.dead) {
    if (vtxc<vtxa) {
      vtx=vtxv+vtxc++;
      vtx->tileid=0xc1;
      vtx->xform=0;
      vtx->x=eatwq.herox-4;
      vtx->y=eatwq.heroy-3;
      if (eatwq.flop) {
        vtx->x+=8;
        vtx->xform=FMN_XFORM_XREV;
      }
    }
  }
  
  // Hero.
  if (vtxc<vtxa) {
    vtx=vtxv+vtxc++;
    vtx->tileid=eatwq.dead?0xe3:0xc0;
    vtx->xform=eatwq.flop?FMN_XFORM_XREV:0;
    vtx->x=eatwq.herox;
    vtx->y=eatwq.heroy;
  }
  
  // Raindrops and bombs.
  struct eatwq_drop *drop=eatwq.dropv;
  for (i=eatwq.dropc;i-->0;drop++) {
    if (vtxc<vtxa) {
      vtx=vtxv+vtxc++;
      vtx->x=drop->x;
      vtx->y=drop->y;
      vtx->tileid=drop->tileid;
      vtx->xform=0;
    }
  }
  
  // Booms.
  struct eatwq_boom *boom=eatwq.boomv;
  for (i=eatwq.boomc;i-->0;boom++) {
    if (vtxc<=vtxa-4) {
      vtx=vtxv+vtxc++;
      vtx->x=boom->x-4;
      vtx->y=boom->y-4;
      vtx->tileid=0xe0;
      vtx->xform=0;
      vtx=vtxv+vtxc++;
      vtx->x=boom->x+4;
      vtx->y=boom->y-4;
      vtx->tileid=0xe1;
      vtx->xform=0;
      vtx=vtxv+vtxc++;
      vtx->x=boom->x-4;
      vtx->y=boom->y+4;
      vtx->tileid=0xf0;
      vtx->xform=0;
      vtx=vtxv+vtxc++;
      vtx->x=boom->x+4;
      vtx->y=boom->y+4;
      vtx->tileid=0xf1;
      vtx->xform=0;
    }
  }
  
  // Clock.
  vtxc+=fmn_eatwq_render_decint(vtxv+vtxc,vtxa-vtxc,77,5,eatwq.playtime/60,2);
  
  // Score, if summarizing.
  if (eatwq.dead) {
    vtxc+=fmn_eatwq_render_decint(vtxv+vtxc,vtxa-vtxc,77,50,eatwq.score,2);
  }
  
  return vtxc;
}

/* Hello.
 * Mostly managed by the menu. We only draw the high score and credits.
 */
 
static int fmn_eatwq_render_hello(struct fmn_draw_mintile *vtxv,int vtxa) {
  int vtxc=0;
  struct fmn_draw_mintile *vtx;
  
  if (vtxc<vtxa) {
    vtx=vtxv+vtxc++;
    vtx->tileid=0xff; // trophy
    vtx->x=60;
    vtx->y=68;
    vtx->xform=0;
  }
  vtxc+=fmn_eatwq_render_decint(vtxv+vtxc,vtxa-vtxc,68,68,eatwq.hiscore,3);
  
  if (eatwq.creditc) {
    if (vtxc<vtxa) {
      vtx=vtxv+vtxc++;
      vtx->tileid=0xfe; // coin
      vtx->x=60;
      vtx->y=83;
      vtx->xform=0;
    }
    vtxc+=fmn_eatwq_render_decint(vtxv+vtxc,vtxa-vtxc,68,83,eatwq.creditc,1);
  }
  
  return vtxc;
}

/* Render.
 */
 
int fmn_eatwq_render(struct fmn_draw_mintile *vtxv,int vtxa) {
  if (eatwq.running) return fmn_eatwq_render_running(vtxv,vtxa);
  else return fmn_eatwq_render_hello(vtxv,vtxa);
}

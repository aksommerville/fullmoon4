#include "fmn_render_internal.h"

/* Context.
 * There's a ton of temporary data during the render.
 * Easier to keep it here than passing around to every function.
 * (fmn_render_sprite_WEREWOLF is guaranteed non-re-entrant).
 */
 
static struct fmn_ww {
  struct fmn_draw_mintile *vtxv;
  int vtxc,vtxa;
  struct fmn_sprite *sprite;
  int16_t tilesize;
  int16_t pxx,pxy;
  int framec;
  uint8_t ffchargeframe;
} fmn_ww={0};

/* Add vertex.
 */
 
static inline void fmn_ww_vtx(int16_t x,int16_t y,uint8_t tileid,uint8_t xform) {
  if (fmn_ww.vtxc<fmn_ww.vtxa) {
    struct fmn_draw_mintile *vtx=fmn_ww.vtxv+fmn_ww.vtxc;
    vtx->x=x;
    vtx->y=y;
    vtx->tileid=tileid;
    vtx->xform=xform;
  }
  fmn_ww.vtxc++;
}

/* Body parts.
 */
 
static void fmn_WEREWOLF_body(int16_t dx,int16_t dy,uint8_t tileid) {
  int16_t ts=fmn_ww.tilesize;
  int16_t pxy=fmn_ww.pxy+dy-ts;
  if (fmn_ww.sprite->xform&FMN_XFORM_XREV) {
    int16_t pxx=fmn_ww.pxx+dx-ts;
    fmn_ww_vtx(pxx   ,pxy   ,tileid+0x01,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts,pxy   ,tileid     ,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx   ,pxy+ts,tileid+0x11,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts,pxy+ts,tileid+0x10,fmn_ww.sprite->xform);
  } else {
    int16_t pxx=fmn_ww.pxx+dx;
    fmn_ww_vtx(pxx   ,pxy   ,tileid     ,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts,pxy   ,tileid+0x01,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx   ,pxy+ts,tileid+0x10,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts,pxy+ts,tileid+0x11,fmn_ww.sprite->xform);
  }
}

static void fmn_WEREWOLF_body3x2(int16_t dx,int16_t dy) {
  int16_t ts=fmn_ww.tilesize;
  int16_t pxx=fmn_ww.pxx+dx-ts;
  int16_t pxy=fmn_ww.pxy+dy-((ts*3)>>1);
  uint8_t tileid=0x80;
  if (fmn_ww.sprite->xform&FMN_XFORM_XREV) {
    fmn_ww_vtx(pxx     ,pxy   ,tileid+0x02,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts  ,pxy   ,tileid+0x01,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts*2,pxy   ,tileid+0x00,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx     ,pxy+ts,tileid+0x12,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts  ,pxy+ts,tileid+0x11,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts*2,pxy+ts,tileid+0x10,fmn_ww.sprite->xform);
  } else {
    fmn_ww_vtx(pxx     ,pxy   ,tileid+0x00,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts  ,pxy   ,tileid+0x01,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts*2,pxy   ,tileid+0x02,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx     ,pxy+ts,tileid+0x10,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts  ,pxy+ts,tileid+0x11,fmn_ww.sprite->xform);
    fmn_ww_vtx(pxx+ts*2,pxy+ts,tileid+0x12,fmn_ww.sprite->xform);
  }
}

static void fmn_WEREWOLF_head(int16_t dx,int16_t dy,uint8_t column) {
  int16_t ts=fmn_ww.tilesize;
  int16_t dstx;
  if (fmn_ww.sprite->xform&FMN_XFORM_XREV) dstx=fmn_ww.pxx-dx-((ts*3)>>1);
  else dstx=fmn_ww.pxx+dx+(ts>>1);
  int16_t dsty=fmn_ww.pxy+dy-((ts*3)>>1)+3;
  if (fmn_ww.sprite->xform&FMN_XFORM_XREV) {
    fmn_ww_vtx(dstx   ,dsty,(column<<1)+1,fmn_ww.sprite->xform);
    fmn_ww_vtx(dstx+ts,dsty,(column<<1)  ,fmn_ww.sprite->xform);
  } else {
    fmn_ww_vtx(dstx   ,dsty,(column<<1)  ,fmn_ww.sprite->xform);
    fmn_ww_vtx(dstx+ts,dsty,(column<<1)+1,fmn_ww.sprite->xform);
  }
}

static void fmn_WEREWOLF_head1x2(int16_t dx,int16_t dy,uint8_t column) {
  int16_t dstx;
  if (fmn_ww.sprite->xform&FMN_XFORM_XREV) dstx=fmn_ww.pxx-dx-fmn_ww.tilesize;
  else dstx=fmn_ww.pxx+dx;
  dy+=-(fmn_ww.tilesize<<1)+3;
  int16_t dsty=fmn_ww.pxy+dy;
  dstx+=fmn_ww.tilesize>>1;
  dsty+=fmn_ww.tilesize>>1;
  fmn_ww_vtx(dstx,dsty,0x80+column,fmn_ww.sprite->xform);
  dsty+=fmn_ww.tilesize;
  fmn_ww_vtx(dstx,dsty,0x90+column,fmn_ww.sprite->xform);
}

static void fmn_WEREWOLF_back_arm(uint8_t frame) {
  int16_t x=0,y=0;
  switch (frame) {
    case 0: x=0; y=-fmn_ww.tilesize-1; break;
    case 1: x=1; y=-((fmn_ww.tilesize*3)>>1); break;
    case 2: x=-(fmn_ww.tilesize>>1)+2; y=-fmn_ww.tilesize; break;
    case 3: x=1; y=-fmn_ww.tilesize+4; break;
  }
  if (fmn_ww.sprite->xform&FMN_XFORM_XREV) x=-x;
  x+=fmn_ww.pxx;
  y+=fmn_ww.pxy;
  fmn_ww_vtx(x,y,0x60+frame,fmn_ww.sprite->xform);
}

static void fmn_WEREWOLF_front_arm(uint8_t frame) {
  int16_t x=0,y=0;
  switch (frame) {
    case 0: x=(fmn_ww.tilesize>>1)+2; y=-fmn_ww.tilesize; break;
    case 1: x=fmn_ww.tilesize-4; y=-((fmn_ww.tilesize*3)>>1); break;
    case 2: x=(fmn_ww.tilesize>>1)-3; y=-fmn_ww.tilesize; break;
    case 3: x=fmn_ww.tilesize-4; y=-(fmn_ww.tilesize>>1)-1; break;
  }
  if (fmn_ww.sprite->xform&FMN_XFORM_XREV) x=-x;
  x+=fmn_ww.pxx;
  y+=fmn_ww.pxy;
  fmn_ww_vtx(x,y,0x50+frame,fmn_ww.sprite->xform);
}

static void fmn_WEREWOLF_erect(uint8_t animate_legs,uint8_t arm_frame,uint8_t head_frame) {
  uint8_t tileid=0x30;
  if (animate_legs) switch ((fmn_ww.framec>>3)&3) {
    case 1: tileid=0x12; break;
    case 3: tileid=0x14; break;
  }
  fmn_WEREWOLF_back_arm(arm_frame);
  fmn_WEREWOLF_body(0,0,tileid);
  fmn_WEREWOLF_head(-fmn_ww.tilesize,-7,head_frame);
  fmn_WEREWOLF_front_arm(arm_frame);
}

static void fmn_WEREWOLF_hadouken() {
  const float CHARGE_TIME=0.5f;
  uint8_t tileid=0x70+(fmn_ww.sprite->fv[1]*4.0f)/CHARGE_TIME;
  if (tileid>0x73) tileid=0x73;
  fmn_ww_vtx(fmn_ww.pxx,fmn_ww.pxy-fmn_ww.tilesize,tileid,0);
}

static void fmn_WEREWOLF_ffcharge() {
  int16_t x=fmn_ww.pxx+((fmn_ww.sprite->xform&FMN_XFORM_XREV)?-(fmn_ww.tilesize>>1):(fmn_ww.tilesize>>1));
  int16_t y=fmn_ww.pxy-fmn_ww.tilesize;
  int16_t stop=-fmn_ww.tilesize;
  for (;y>stop;y-=fmn_ww.tilesize) {
    fmn_ww_vtx(x,y,0xa0+fmn_ww.ffchargeframe,0);
  }
  fmn_ww.ffchargeframe++;
  if (fmn_ww.ffchargeframe>=16) fmn_ww.ffchargeframe-=4;
}

static void fmn_WEREWOLF_all_fours(uint8_t animate_legs) {
  uint8_t tileid=0x10;
  if (animate_legs) switch ((fmn_ww.framec>>3)&3) {
    case 1: tileid=0x12; break;
    case 3: tileid=0x14; break;
  }
  fmn_WEREWOLF_body(0,0,tileid);
  fmn_WEREWOLF_head(-(fmn_ww.tilesize<<1)+3,0,fmn_ww.framec%80<15);
}

static void fmn_WEREWOLF_growl() {
  fmn_WEREWOLF_body(0,0,0x18);
  fmn_WEREWOLF_head(-(fmn_ww.tilesize<<1)+3,5,2);
}

static void fmn_WEREWOLF_pounce() {
  fmn_WEREWOLF_body3x2(0,0);
  fmn_WEREWOLF_head(-(fmn_ww.tilesize<<1)-4,0,5);
}

static void fmn_WEREWOLF_sleep() {
  fmn_WEREWOLF_body(0,8,0x16);
  fmn_WEREWOLF_head(-(fmn_ww.tilesize<<1)+3,fmn_ww.tilesize,3);
}

static void fmn_WEREWOLF_dead() {
  fmn_WEREWOLF_body(0,8,0x16);
  fmn_WEREWOLF_head(-(fmn_ww.tilesize<<1)+3,fmn_ww.tilesize,7);
}

static void fmn_WEREWOLF_shock() {
  uint8_t tileid=0x1c;
  if (fmn_ww.framec%50>=42) tileid=0x1e;
  fmn_WEREWOLF_body(0,0,tileid);
  fmn_WEREWOLF_head(-(fmn_ww.tilesize<<1)+1,fmn_ww.tilesize+2,6);
}

static void fmn_WEREWOLF_eat() {
  fmn_WEREWOLF_body(0,0,0x1a);
  uint8_t frame;
  switch ((fmn_ww.framec/10)%3) {
    case 0: frame=3; break;
    case 1: frame=4; break;
    case 2: frame=5; break;
  }
  fmn_WEREWOLF_head1x2(-fmn_ww.tilesize-6,6,frame);
}

/* Render werewolf, main entry point.
 */
 
int fmn_render_sprite_WEREWOLF(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite) {
  fmn_ww.vtxv=vtxv;
  fmn_ww.vtxa=vtxa;
  fmn_ww.vtxc=0;
  fmn_ww.sprite=sprite;
  fmn_ww.tilesize=fmn_render_global.tilesize;
  fmn_ww.pxx=sprite->x*fmn_ww.tilesize;
  fmn_ww.pxy=sprite->y*fmn_ww.tilesize;
  fmn_ww.framec=fmn_render_global.framec;
  
  // Cheat (pxx) over when upright, so it remains a sensible axis of reflection.
  switch (sprite->bv[0]) {
    case 2: // ERECT
    case 6: // HADOUKEN_CHARGE
    case 7: // HADOUKEN_FOLLOWTHRU
    case 8: // FLOORFIRE_CHARGE
    case 9: // FLOORFIRE_FOLLOWTHRU
      if (sprite->xform&FMN_XFORM_XREV) fmn_ww.pxx-=fmn_ww.tilesize>>1;
      else fmn_ww.pxx+=fmn_ww.tilesize>>1;
      break;
  }
  
  // Each stage of the wolf is pretty different from the others.
  switch (sprite->bv[0]) {
    case 0: fmn_WEREWOLF_all_fours(0); break; // IDLE
    case 1: fmn_WEREWOLF_sleep(); break; // SLEEP
    case 2: fmn_WEREWOLF_erect(0,0,0); break; // ERECT
    case 3: fmn_WEREWOLF_all_fours(1); break; // WALK
    case 4: fmn_WEREWOLF_growl(); break; // GROWL
    case 5: fmn_WEREWOLF_pounce(); break; // POUNCE
    case 6: fmn_WEREWOLF_erect(0,0,0); fmn_WEREWOLF_hadouken(); break; // HADOUKEN_CHARGE
    case 7: fmn_WEREWOLF_erect(0,2,0); break; // HADOUKEN_FOLLOWTHRU
    case 8: fmn_WEREWOLF_ffcharge(); fmn_WEREWOLF_erect(0,1,4); break; // FLOORFIRE_CHARGE
    case 9: fmn_WEREWOLF_erect(0,3,0); fmn_ww.ffchargeframe=0; break; // FLOORFIRE_FOLLOWTHRU
    case 10: fmn_WEREWOLF_eat(); break; // EAT
    case 11: fmn_WEREWOLF_shock(); break; // SHOCK
    case 12: fmn_WEREWOLF_dead(); break; // DEAD
  }
  
  return fmn_ww.vtxc;
}

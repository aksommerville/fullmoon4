/* fmn_gl2_sprites.c
 * A few styles are simple enough that we implement in the loop: HIDDEN, TILE, TWOFRAME, FOURFRAME, EIGHTFRAME.
 * One is complicated enough to get its own file: HERO.
 * Everything else is implemented here.
 * (including WEREWOLF, though one could certainly argue to separate him).
 */
 
#include "../fmn_gl2_internal.h"

/* FIRENOZZLE.
 */

void fmn_gl2_game_render_FIRENOZZLE(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  switch (sprite->bv[1]) {
    case 1: fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y,sprite->tileid+1,sprite->xform); break;
    case 2: {
        int16_t dstx=sprite->x*DRIVER->game.tilesize;
        int16_t dsty=sprite->y*DRIVER->game.tilesize;
        int16_t dx=0,dy=0,i;
        switch (sprite->xform) {
          case 0: dx=DRIVER->game.tilesize; break;
          case FMN_XFORM_XREV: dx=-DRIVER->game.tilesize; break;
          case FMN_XFORM_SWAP: dy=DRIVER->game.tilesize; break;
          case FMN_XFORM_SWAP|FMN_XFORM_XREV: dy=-DRIVER->game.tilesize; break;
        }
        fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx,dsty,sprite->tileid+2,sprite->xform);
        uint8_t flamebase=sprite->tileid+((DRIVER->game.framec&8)?3:6);
        dstx+=dx;
        dsty+=dy;
        fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx,dsty,flamebase,sprite->xform);
        dstx+=dx;
        dsty+=dy;
        for (i=sprite->bv[2];i-->2;dstx+=dx,dsty+=dy) {
          fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx,dsty,flamebase+1,sprite->xform);
        }
        fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx,dsty,flamebase+2,sprite->xform);
      } break;
    default: fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y,sprite->tileid,sprite->xform); break;
  }
}

/* FIREWALL.
 */
  
static void fmn_gl2_game_render_FIREWALL_1(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,
  int16_t x,int16_t y,int16_t w,int16_t h,uint8_t dir
) {
  uint8_t tileid=sprite->tileid+((DRIVER->game.framec&16)?0x20:0);
  int16_t dxminor=0,dyminor=0,dxmajor=0,dymajor=0;
  uint8_t xform=0;
  switch (dir) {
    case FMN_DIR_N: dxminor= DRIVER->game.tilesize; dymajor=-DRIVER->game.tilesize; y+=h-DRIVER->game.tilesize; break;
    case FMN_DIR_S: dxminor= DRIVER->game.tilesize; dymajor= DRIVER->game.tilesize; xform=FMN_XFORM_YREV; break;
    case FMN_DIR_W: dxmajor=-DRIVER->game.tilesize; dyminor= DRIVER->game.tilesize; x+=w-DRIVER->game.tilesize; xform=FMN_XFORM_SWAP; break;
    case FMN_DIR_E: dxmajor= DRIVER->game.tilesize; dyminor= DRIVER->game.tilesize; xform=FMN_XFORM_SWAP|FMN_XFORM_YREV; break;
    default: return;
  }
  int16_t minorc=(w*(dxminor?1:0)+h*(dyminor?1:0)+DRIVER->game.tilesize-1)/DRIVER->game.tilesize;
  int16_t majorc=(w*(dxmajor?1:0)+h*(dymajor?1:0)+DRIVER->game.tilesize-1)/DRIVER->game.tilesize;
  x+=DRIVER->game.tilesize>>1;
  y+=DRIVER->game.tilesize>>1;
  uint8_t tilerow=0x10;
  int16_t majori=majorc;
  for (;majori-->0;x+=dxmajor,y+=dymajor) {
    int16_t x1=x,y1=y,minori=minorc;
    for (;minori-->0;x1+=dxminor,y1+=dyminor) {
      uint8_t subtileid=tileid+tilerow+((minori==minorc-1)?0:minori?1:2);
      fmn_gl2_game_add_mintile_vtx_pixcoord(driver,x1,y1,subtileid,xform);
    }
    tilerow=0;
  }
}
 
void fmn_gl2_game_render_FIREWALL(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  int16_t extent=sprite->fv[0]*DRIVER->game.tilesize;
  int16_t sp=sprite->bv[1]*DRIVER->game.tilesize;
  int16_t sc=sprite->bv[2]*DRIVER->game.tilesize;
  switch (sprite->bv[0]) {
    case FMN_DIR_W: fmn_gl2_game_render_FIREWALL_1(driver,sprite,0,sp,extent,sc,sprite->bv[0]); break;
    case FMN_DIR_N: fmn_gl2_game_render_FIREWALL_1(driver,sprite,sp,0,sc,extent,sprite->bv[0]); break;
    case FMN_DIR_E: fmn_gl2_game_render_FIREWALL_1(driver,sprite,DRIVER->mainfb.texture.w-extent,sp,extent,sc,sprite->bv[0]); break;
    case FMN_DIR_S: fmn_gl2_game_render_FIREWALL_1(driver,sprite,sp,DRIVER->mainfb.texture.h-extent,sc,extent,sprite->bv[0]); break;
  }
}

/* DOUBLEWIDE.
 */
 
void fmn_gl2_game_render_DOUBLEWIDE(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  if (sprite->xform&FMN_XFORM_XREV) {
    fmn_gl2_game_add_mintile_vtx(driver,sprite->x-0.5f,sprite->y,sprite->tileid+1,sprite->xform);
    fmn_gl2_game_add_mintile_vtx(driver,sprite->x+0.5f,sprite->y,sprite->tileid,sprite->xform);
  } else {
    fmn_gl2_game_add_mintile_vtx(driver,sprite->x-0.5f,sprite->y,sprite->tileid,sprite->xform);
    fmn_gl2_game_add_mintile_vtx(driver,sprite->x+0.5f,sprite->y,sprite->tileid+1,sprite->xform);
  }
}

/* PITCHFORK.
 */
 
void fmn_gl2_game_render_PITCHFORK(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  int16_t bodydstx=sprite->x*DRIVER->game.tilesize;
  int16_t dsty=sprite->y*DRIVER->game.tilesize;
  int16_t dx=DRIVER->game.tilesize*((sprite->xform&FMN_XFORM_XREV)?1:-1);
  int16_t dstx=bodydstx+dx*(sprite->fv[0]+1.0f);
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx,dsty,sprite->tileid-4,sprite->xform);
  for (;;) {
    dstx-=dx;
    if ((dx>0)&&(dstx<=bodydstx)) break;
    if ((dx<0)&&(dstx>=bodydstx)) break;
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx,dsty,sprite->tileid-2,sprite->xform);
  }
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,bodydstx,dsty,sprite->tileid-((sprite->fv[0]>0.0f)?0:1),sprite->xform);
}

/* SCARYDOOR.
 */
 
void fmn_gl2_game_render_SCARYDOOR(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  int16_t ts2=DRIVER->game.tilesize<<1;
  int16_t halfh=sprite->fv[0]*ts2;
  if (halfh<=0) return;
  int16_t dstx=((int16_t)sprite->x)*DRIVER->game.tilesize;
  int16_t dsty=((int16_t)sprite->y)*DRIVER->game.tilesize;
  int16_t srcx=(sprite->tileid&15)*DRIVER->game.tilesize;
  int16_t srcy=(sprite->tileid>>4)*DRIVER->game.tilesize;
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  fmn_gl2_draw_decal(
    dstx,dsty,ts2,halfh,
    srcx,srcy+ts2-halfh,ts2,halfh
  );
  fmn_gl2_draw_decal(
    dstx,dsty+DRIVER->game.tilesize+ts2-halfh,ts2,halfh,
    srcx,srcy+ts2,ts2,halfh
  );
}

/* WEREWOLF.
 */
 
static void fmn_WEREWOLF_body(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy,
  int16_t dx,int16_t dy,uint8_t tileid
) {
  int16_t ts=DRIVER->game.tilesize;
  pxy+=dy-ts;
  if (sprite->xform&FMN_XFORM_XREV) {
    pxx+=dx-ts;
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx   ,pxy   ,tileid+0x01,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts,pxy   ,tileid     ,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx   ,pxy+ts,tileid+0x11,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts,pxy+ts,tileid+0x10,sprite->xform);
  } else {
    pxx+=dx;
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx   ,pxy   ,tileid     ,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts,pxy   ,tileid+0x01,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx   ,pxy+ts,tileid+0x10,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts,pxy+ts,tileid+0x11,sprite->xform);
  }
}

static void fmn_WEREWOLF_body3x2(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy,
  int16_t dx,int16_t dy
) {
  int16_t ts=DRIVER->game.tilesize;
  pxx+=dx-ts;
  pxy+=dy-((ts*3)>>1);
  uint8_t tileid=0x80;
  if (sprite->xform&FMN_XFORM_XREV) {
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx     ,pxy   ,tileid+0x02,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts  ,pxy   ,tileid+0x01,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts*2,pxy   ,tileid+0x00,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx     ,pxy+ts,tileid+0x12,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts  ,pxy+ts,tileid+0x11,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts*2,pxy+ts,tileid+0x10,sprite->xform);
  } else {
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx     ,pxy   ,tileid+0x00,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts  ,pxy   ,tileid+0x01,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts*2,pxy   ,tileid+0x02,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx     ,pxy+ts,tileid+0x10,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts  ,pxy+ts,tileid+0x11,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+ts*2,pxy+ts,tileid+0x12,sprite->xform);
  }
}

static void fmn_WEREWOLF_head(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy,
  int16_t dx,int16_t dy,uint8_t column
) {
  int16_t ts=DRIVER->game.tilesize;
  int16_t dstx;
  if (sprite->xform&FMN_XFORM_XREV) dstx=pxx-dx-((ts*3)>>1);
  else dstx=pxx+dx+(ts>>1);
  int16_t dsty=pxy+dy-((ts*3)>>1)+3;
  if (sprite->xform&FMN_XFORM_XREV) {
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx   ,dsty,(column<<1)+1,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx+ts,dsty,(column<<1)  ,sprite->xform);
  } else {
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx   ,dsty,(column<<1)  ,sprite->xform);
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx+ts,dsty,(column<<1)+1,sprite->xform);
  }
}

static void fmn_WEREWOLF_head1x2(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy,
  int16_t dx,int16_t dy,uint8_t column
) {
  int16_t dstx;
  if (sprite->xform&FMN_XFORM_XREV) dstx=pxx-dx-DRIVER->game.tilesize;
  else dstx=pxx+dx;
  dy+=-(DRIVER->game.tilesize<<1)+3;
  int16_t dsty=pxy+dy;
  dstx+=DRIVER->game.tilesize>>1;
  dsty+=DRIVER->game.tilesize>>1;
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx,dsty,0x80+column,sprite->xform);
  dsty+=DRIVER->game.tilesize;
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,dstx,dsty,0x90+column,sprite->xform);
}

static void fmn_WEREWOLF_back_arm(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy,
  uint8_t frame
) {
  int16_t x=0,y=0;
  switch (frame) {
    case 0: x=0; y=-DRIVER->game.tilesize-1; break;
    case 1: x=1; y=-((DRIVER->game.tilesize*3)>>1); break;
    case 2: x=-(DRIVER->game.tilesize>>1)+2; y=-DRIVER->game.tilesize; break;
    case 3: x=1; y=-DRIVER->game.tilesize+4; break;
  }
  if (sprite->xform&FMN_XFORM_XREV) x=-x;
  x+=pxx;
  y+=pxy;
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,x,y,0x60+frame,sprite->xform);
}

static void fmn_WEREWOLF_front_arm(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy,
  uint8_t frame
) {
  int16_t x=0,y=0;
  switch (frame) {
    case 0: x=(DRIVER->game.tilesize>>1)+2; y=-DRIVER->game.tilesize; break;
    case 1: x=DRIVER->game.tilesize-4; y=-((DRIVER->game.tilesize*3)>>1); break;
    case 2: x=(DRIVER->game.tilesize>>1)-3; y=-DRIVER->game.tilesize; break;
    case 3: x=DRIVER->game.tilesize-4; y=-(DRIVER->game.tilesize>>1)-1; break;
  }
  if (sprite->xform&FMN_XFORM_XREV) x=-x;
  x+=pxx;
  y+=pxy;
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,x,y,0x50+frame,sprite->xform);
}

static void fmn_WEREWOLF_erect(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy,
  uint8_t animate_legs,uint8_t arm_frame,uint8_t head_frame
) {
  uint8_t tileid=0x30;
  if (animate_legs) switch ((DRIVER->game.framec>>3)&3) {
    case 1: tileid=0x12; break;
    case 3: tileid=0x14; break;
  }
  fmn_WEREWOLF_back_arm(driver,sprite,pxx,pxy,arm_frame);
  fmn_WEREWOLF_body(driver,sprite,pxx,pxy,0,0,tileid);
  fmn_WEREWOLF_head(driver,sprite,pxx,pxy,-DRIVER->game.tilesize,-7,head_frame);
  fmn_WEREWOLF_front_arm(driver,sprite,pxx,pxy,arm_frame);
}

static void fmn_WEREWOLF_hadouken(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy
) {
  const float CHARGE_TIME=0.5f;
  uint8_t tileid=0x70+(sprite->fv[1]*4.0f)/CHARGE_TIME;
  if (tileid>0x73) tileid=0x73;
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx,pxy-DRIVER->game.tilesize,tileid,0);
}

static void fmn_WEREWOLF_ffcharge(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy
) {
  int16_t x=pxx+((sprite->xform&FMN_XFORM_XREV)?-(DRIVER->game.tilesize>>1):(DRIVER->game.tilesize>>1));
  int16_t y=pxy-DRIVER->game.tilesize;
  int16_t stop=-DRIVER->game.tilesize;
  for (;y>stop;y-=DRIVER->game.tilesize) {
    fmn_gl2_game_add_mintile_vtx_pixcoord(driver,x,y,0xa0+DRIVER->game.ffchargeframe,0);
  }
  DRIVER->game.ffchargeframe++;
  if (DRIVER->game.ffchargeframe>=16) DRIVER->game.ffchargeframe-=4;
}

static void fmn_WEREWOLF_all_fours(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy,
  uint8_t animate_legs
) {
  uint8_t tileid=0x10;
  if (animate_legs) switch ((DRIVER->game.framec>>3)&3) {
    case 1: tileid=0x12; break;
    case 3: tileid=0x14; break;
  }
  fmn_WEREWOLF_body(driver,sprite,pxx,pxy,0,0,tileid);
  fmn_WEREWOLF_head(driver,sprite,pxx,pxy,-(DRIVER->game.tilesize<<1)+3,0,DRIVER->game.framec%80<15);
}

static void fmn_WEREWOLF_growl(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy
) {
  fmn_WEREWOLF_body(driver,sprite,pxx,pxy,0,0,0x18);
  fmn_WEREWOLF_head(driver,sprite,pxx,pxy,-(DRIVER->game.tilesize<<1)+3,5,2);
}

static void fmn_WEREWOLF_pounce(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy
) {
  fmn_WEREWOLF_body3x2(driver,sprite,pxx,pxy,0,0);
  fmn_WEREWOLF_head(driver,sprite,pxx,pxy,-(DRIVER->game.tilesize<<1)-4,0,5);
}

static void fmn_WEREWOLF_sleep(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy
) {
  fmn_WEREWOLF_body(driver,sprite,pxx,pxy,0,8,0x16);
  fmn_WEREWOLF_head(driver,sprite,pxx,pxy,-(DRIVER->game.tilesize<<1)+3,DRIVER->game.tilesize,3);
}

static void fmn_WEREWOLF_dead(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy
) {
  fmn_WEREWOLF_body(driver,sprite,pxx,pxy,0,8,0x16);
  fmn_WEREWOLF_head(driver,sprite,pxx,pxy,-(DRIVER->game.tilesize<<1)+3,DRIVER->game.tilesize,7);
}

static void fmn_WEREWOLF_shock(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy
) {
  uint8_t tileid=0x1c;
  if (DRIVER->game.framec%50>=42) tileid=0x1e;
  fmn_WEREWOLF_body(driver,sprite,pxx,pxy,0,0,tileid);
  fmn_WEREWOLF_head(driver,sprite,pxx,pxy,-(DRIVER->game.tilesize<<1)+1,DRIVER->game.tilesize+2,6);
}

static void fmn_WEREWOLF_eat(
  struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,int16_t pxx,int16_t pxy
) {
  fmn_WEREWOLF_body(driver,sprite,pxx,pxy,0,0,0x1a);
  uint8_t frame;
  switch ((DRIVER->game.framec/10)%3) {
    case 0: frame=3; break;
    case 1: frame=4; break;
    case 2: frame=5; break;
  }
  fmn_WEREWOLF_head1x2(driver,sprite,pxx,pxy,-DRIVER->game.tilesize-6,6,frame);
}
 
void fmn_gl2_game_render_WEREWOLF(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  int16_t pxx=sprite->x*DRIVER->game.tilesize;
  int16_t pxy=sprite->y*DRIVER->game.tilesize;
  
  // Cheat (pxx) over when upright, so it remains a sensible axis of reflection.
  switch (sprite->bv[0]) {
    case 2: // ERECT
    case 6: // HADOUKEN_CHARGE
    case 7: // HADOUKEN_FOLLOWTHRU
    case 8: // FLOORFIRE_CHARGE
    case 9: // FLOORFIRE_FOLLOWTHRU
      if (sprite->xform&FMN_XFORM_XREV) pxx-=DRIVER->game.tilesize>>1;
      else pxx+=DRIVER->game.tilesize>>1;
      break;
  }
  
  // Each stage of the wolf is pretty different from the others.
  switch (sprite->bv[0]) {
    case 0: fmn_WEREWOLF_all_fours(driver,sprite,pxx,pxy,0); break; // IDLE
    case 1: fmn_WEREWOLF_sleep(driver,sprite,pxx,pxy); break; // SLEEP
    case 2: fmn_WEREWOLF_erect(driver,sprite,pxx,pxy,0,0,0); break; // ERECT
    case 3: fmn_WEREWOLF_all_fours(driver,sprite,pxx,pxy,1); break; // WALK
    case 4: fmn_WEREWOLF_growl(driver,sprite,pxx,pxy); break; // GROWL
    case 5: fmn_WEREWOLF_pounce(driver,sprite,pxx,pxy); break; // POUNCE
    case 6: fmn_WEREWOLF_erect(driver,sprite,pxx,pxy,0,0,0); fmn_WEREWOLF_hadouken(driver,sprite,pxx,pxy); break; // HADOUKEN_CHARGE
    case 7: fmn_WEREWOLF_erect(driver,sprite,pxx,pxy,0,2,0); break; // HADOUKEN_FOLLOWTHRU
    case 8: fmn_WEREWOLF_ffcharge(driver,sprite,pxx,pxy); fmn_WEREWOLF_erect(driver,sprite,pxx,pxy,0,1,4); break; // FLOORFIRE_CHARGE
    case 9: fmn_WEREWOLF_erect(driver,sprite,pxx,pxy,0,3,0); DRIVER->game.ffchargeframe=0; break; // FLOORFIRE_FOLLOWTHRU
    case 10: fmn_WEREWOLF_eat(driver,sprite,pxx,pxy); break; // EAT
    case 11: fmn_WEREWOLF_shock(driver,sprite,pxx,pxy); break; // SHOCK
    case 12: fmn_WEREWOLF_dead(driver,sprite,pxx,pxy); break; // DEAD
  }
}

/* FLOORFIRE.
 */
 
void fmn_gl2_game_render_FLOORFIRE(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  const float RING_SPACING=0.750f;
  const float RADIAL_SPACING=0.500f;
  const float xlimit=(FMN_COLC+1.0f);
  const float ylimit=(FMN_ROWC+1.0f);
  uint8_t i=sprite->bv[0];
  float radius=sprite->fv[2];
  uint8_t tileid=0xb0;
  for (;(i-->0)&&(tileid<0xbd);radius-=RING_SPACING,tileid++) {
    int firec=(radius*2.0f*M_PI)/RADIAL_SPACING;
    if (firec<1) break;
    float dt=(M_PI*2.0f)/firec;
    float t=0.0f;
    for (;firec-->0;t+=dt) {
      float x=sprite->x+cosf(t)*radius;
      if ((x<-1.0f)||(x>xlimit)) continue;
      float y=sprite->y+sinf(t)*radius;
      if ((y<-1.0f)||(y>ylimit)) continue;
      fmn_gl2_game_add_mintile_vtx(driver,x,y,tileid,0);
    }
  }
}

/* DEADWITCH.
 */
 
void fmn_gl2_game_render_DEADWITCH(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  int16_t pxx=sprite->x*DRIVER->game.tilesize;
  int16_t pxy=sprite->y*DRIVER->game.tilesize+2;
  float clock=sprite->fv[0];
  int frame=clock*7.0f;
  if (frame>=7) frame=6;
  
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx-8,pxy+1,sprite->tileid-0x0f+frame,0);
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx+8,pxy+2,sprite->tileid+0x01+frame,0);
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx-2,pxy+4,sprite->tileid+0x01+frame,0);
  
  fmn_gl2_game_add_mintile_vtx_pixcoord(driver,pxx,pxy,sprite->tileid,sprite->xform);
}

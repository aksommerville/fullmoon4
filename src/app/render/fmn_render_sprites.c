#include "fmn_render_internal.h"

/* Helpers to write mintile vertices.
 */
 
static inline void fmn_mintile_world(struct fmn_draw_mintile *vtx,float x,float y,uint8_t tileid,uint8_t xform) {
  vtx->x=x*fmn_render_global.tilesize;
  vtx->y=y*fmn_render_global.tilesize;
  vtx->tileid=tileid;
  vtx->xform=xform;
}
 
static inline void fmn_mintile_screen(struct fmn_draw_mintile *vtx,int16_t x,int16_t y,uint8_t tileid,uint8_t xform) {
  vtx->x=x;
  vtx->y=y;
  vtx->tileid=tileid;
  vtx->xform=xform;
}

static inline void fmn_mintile_sprite(struct fmn_draw_mintile *vtx,const struct fmn_sprite *sprite) {
  vtx->x=sprite->x*fmn_render_global.tilesize;
  vtx->y=sprite->y*fmn_render_global.tilesize;
  vtx->tileid=sprite->tileid;
  vtx->xform=sprite->xform;
}

/* Two sprite styles are complex enough to get their own file.
 */
 
int fmn_render_sprite_HERO(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite);
int fmn_render_sprite_WEREWOLF(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite);

/* Non-mintile sprite styles. SCARYDOOR, SLIDESHOW, DRAGON.
 */
 
static void fmn_render_sprite_SCARYDOOR(struct fmn_sprite *sprite) {
  const int16_t tilesize=fmn_render_global.tilesize;
  const int16_t ts2=tilesize<<1;
  int16_t halfh=sprite->fv[0]*ts2;
  if (halfh<=0) return;
  int16_t dstx=((int16_t)sprite->x)*tilesize;
  int16_t dsty=((int16_t)sprite->y)*tilesize;
  int16_t srcx=(sprite->tileid&15)*tilesize;
  int16_t srcy=(sprite->tileid>>4)*tilesize;
  struct fmn_draw_decal vtxv[2]={
    {dstx,dsty,ts2,halfh,srcx,srcy+ts2-halfh,ts2,halfh},
    {dstx,dsty+tilesize+ts2-halfh,ts2,halfh,srcx,srcy+ts2,ts2,halfh},
  };
  fmn_draw_decal(vtxv,2,sprite->imageid);
}
 
static void fmn_render_sprite_SLIDESHOW(struct fmn_sprite *sprite) {
  const int16_t tilesize=fmn_render_global.tilesize;
  // Flicker between tileid and tileid0 (bv[2])
  uint8_t tileid=(fmn_render_global.animframec%3)?sprite->tileid:sprite->bv[2];
  int16_t dstx=sprite->x*tilesize-((tilesize*3)/2);
  int16_t dsty=sprite->y*tilesize-(tilesize/2);
  int16_t srcx=(tileid&0x0f)*tilesize;
  int16_t srcy=(tileid>>4)*tilesize;
  struct fmn_draw_decal vtx={
    dstx,dsty,tilesize*3,tilesize*2,
    srcx,srcy,tilesize*3,tilesize*2,
  };
  fmn_draw_decal(&vtx,1,sprite->imageid);
}

static void fmn_render_sprite_DRAGON(struct fmn_sprite *sprite) {
  const int16_t tilesize=fmn_render_global.tilesize;
  // Dragon has 6 frames of different widths. Always 2 rows high. (x,y) is the middle of the lower-right cell always.
  // (tileid): IDLE, BLINK, FIRE1, FIRE2, PUMPKIN1, PUMPKIN2
  uint8_t tileid=sprite->bv[0];
  uint8_t colc;
  switch (sprite->tileid) {
    case 0: tileid+=0xc1; colc=3; break;
    case 1: tileid+=0xe1; colc=3; break;
    case 2: tileid+=0xe4; colc=6; break;
    case 3: tileid+=0x04; colc=6; break;
    case 4: tileid+=0xbf; colc=2; break;
    case 5: tileid+=0xdf; colc=2; break;
    default: return;
  }
  int16_t srcx=(tileid&0x0f)*tilesize;
  int16_t srcy=(tileid>>4)*tilesize;
  int16_t srcw=colc*tilesize;
  int16_t dstx=(int16_t)sprite->x; // cast first; force to cell boundary
  int16_t dsty=((int16_t)sprite->y-1)*tilesize;
  if (sprite->xform&FMN_XFORM_XREV) {
    srcx+=srcw;
    srcw=-srcw;
  } else {
    dstx-=colc-1;
  }
  dstx*=tilesize;
  struct fmn_draw_decal vtx={
    dstx,dsty,colc*tilesize,2*tilesize,
    srcx,srcy,srcw,2*tilesize,
  };
  fmn_draw_decal(&vtx,1,sprite->imageid);
}

/* Multiple-mintile: FIRENOZZLE
 */
 
static int fmn_render_sprite_FIRENOZZLE(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite) {
  switch (sprite->bv[1]) {
    case 1: fmn_mintile_world(vtxv,sprite->x,sprite->y,sprite->tileid+1,sprite->xform); return 1;
    case 2: {
        int vtxc=sprite->bv[2]+1;
        if (vtxc>vtxa) return vtxc;
        int16_t dstx=sprite->x*fmn_render_global.tilesize;
        int16_t dsty=sprite->y*fmn_render_global.tilesize;
        int16_t dx=0,dy=0,i;
        switch (sprite->xform) {
          case 0: dx=fmn_render_global.tilesize; break;
          case FMN_XFORM_XREV: dx=-fmn_render_global.tilesize; break;
          case FMN_XFORM_SWAP: dy=fmn_render_global.tilesize; break;
          case FMN_XFORM_SWAP|FMN_XFORM_XREV: dy=-fmn_render_global.tilesize; break;
        }
        fmn_mintile_screen(vtxv+0,dstx,dsty,sprite->tileid+2,sprite->xform);
        uint8_t flamebase=sprite->tileid+((fmn_render_global.animframec&8)?3:6);
        dstx+=dx;
        dsty+=dy;
        fmn_mintile_screen(vtxv+1,dstx,dsty,flamebase,sprite->xform);
        dstx+=dx;
        dsty+=dy;
        for (i=sprite->bv[2];i-->2;dstx+=dx,dsty+=dy) {
          fmn_mintile_screen(vtxv+i,dstx,dsty,flamebase+1,sprite->xform);
        }
        fmn_mintile_screen(vtxv+sprite->bv[2],dstx,dsty,flamebase+2,sprite->xform);
        return vtxc;
      }
    default: fmn_mintile_sprite(vtxv,sprite); return 1;
  }
  return 0;
}

/* Multiple-mintile: FIREWALL
 */
  
static int fmn_render_sprite_FIREWALL_1(
  struct fmn_draw_mintile *vtxv,int vtxa,
  struct fmn_sprite *sprite,
  int16_t x,int16_t y,int16_t w,int16_t h,uint8_t dir
) {
  int16_t tilesize=fmn_render_global.tilesize;
  uint8_t tileid=sprite->tileid+((fmn_render_global.animframec&16)?0x20:0);
  int16_t dxminor=0,dyminor=0,dxmajor=0,dymajor=0;
  uint8_t xform=0;
  switch (dir) {
    case FMN_DIR_N: dxminor= tilesize; dymajor=-tilesize; y+=h-tilesize; break;
    case FMN_DIR_S: dxminor= tilesize; dymajor= tilesize; xform=FMN_XFORM_YREV; break;
    case FMN_DIR_W: dxmajor=-tilesize; dyminor= tilesize; x+=w-tilesize; xform=FMN_XFORM_SWAP; break;
    case FMN_DIR_E: dxmajor= tilesize; dyminor= tilesize; xform=FMN_XFORM_SWAP|FMN_XFORM_YREV; break;
    default: return 0;
  }
  int16_t minorc=(w*(dxminor?1:0)+h*(dyminor?1:0)+tilesize-1)/tilesize;
  int16_t majorc=(w*(dxmajor?1:0)+h*(dymajor?1:0)+tilesize-1)/tilesize;
  int vtxc=minorc*majorc;
  if (vtxc>vtxa) return vtxc;
  struct fmn_draw_mintile *vtx=vtxv;
  x+=tilesize>>1;
  y+=tilesize>>1;
  uint8_t tilerow=0x10;
  int16_t majori=majorc;
  for (;majori-->0;x+=dxmajor,y+=dymajor) {
    int16_t x1=x,y1=y,minori=minorc;
    for (;minori-->0;x1+=dxminor,y1+=dyminor,vtx++) {
      uint8_t subtileid=tileid+tilerow+((minori==minorc-1)?0:minori?1:2);
      fmn_mintile_screen(vtx,x1,y1,subtileid,xform);
      //fmn_gl2_game_add_mintile_vtx_pixcoord(driver,x1,y1,subtileid,xform);
    }
    tilerow=0;
  }
  return vtxc;
}
 
static int fmn_render_sprite_FIREWALL(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite) {
  int16_t extent=sprite->fv[0]*fmn_render_global.tilesize;
  int16_t sp=sprite->bv[1]*fmn_render_global.tilesize;
  int16_t sc=sprite->bv[2]*fmn_render_global.tilesize;
  switch (sprite->bv[0]) {
    case FMN_DIR_W: return fmn_render_sprite_FIREWALL_1(vtxv,vtxa,sprite,0,sp,extent,sc,sprite->bv[0]);
    case FMN_DIR_N: return fmn_render_sprite_FIREWALL_1(vtxv,vtxa,sprite,sp,0,sc,extent,sprite->bv[0]);
    case FMN_DIR_E: return fmn_render_sprite_FIREWALL_1(vtxv,vtxa,sprite,fmn_render_global.fbw-extent,sp,extent,sc,sprite->bv[0]);
    case FMN_DIR_S: return fmn_render_sprite_FIREWALL_1(vtxv,vtxa,sprite,sp,fmn_render_global.fbh-extent,sc,extent,sprite->bv[0]);
  }
  return 0;
}

/* Multiple-mintile: DOUBLEWIDE
 */
 
static int fmn_render_sprite_DOUBLEWIDE(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite) {
  if (vtxa<2) return 2;
  int16_t x=sprite->x*fmn_render_global.tilesize;
  int16_t y=sprite->y*fmn_render_global.tilesize;
  int16_t halftile=fmn_render_global.tilesize>>1;
  if (sprite->xform&FMN_XFORM_XREV) {
    fmn_mintile_screen(vtxv+0,x-halftile,y,sprite->tileid+1,sprite->xform);
    fmn_mintile_screen(vtxv+1,x+halftile,y,sprite->tileid+0,sprite->xform);
  } else {
    fmn_mintile_screen(vtxv+0,x-halftile,y,sprite->tileid+0,sprite->xform);
    fmn_mintile_screen(vtxv+1,x+halftile,y,sprite->tileid+1,sprite->xform);
  }
  //TODO verify. rat uses this (cow does not!)
  return 2;
}

/* Multiple-mintile: PITCHFORK
 */
 
static int fmn_render_sprite_PITCHFORK(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite) {
  const int16_t tilesize=fmn_render_global.tilesize;
  int16_t bodydstx=sprite->x*tilesize;
  int16_t dsty=sprite->y*tilesize;
  int16_t dx=tilesize*((sprite->xform&FMN_XFORM_XREV)?1:-1);
  int16_t dstx=bodydstx+dx*(sprite->fv[0]+1.0f);
  fmn_mintile_screen(vtxv+0,dstx,dsty,sprite->tileid-4,sprite->xform);
  int vtxc=1;
  for (;;) {
    dstx-=dx;
    if ((dx>0)&&(dstx<=bodydstx)) break;
    if ((dx<0)&&(dstx>=bodydstx)) break;
    if (vtxc<vtxa) fmn_mintile_screen(vtxv+vtxc,dstx,dsty,sprite->tileid-2,sprite->xform);
    vtxc++;
  }
  if (vtxc<vtxa) fmn_mintile_screen(vtxv+vtxc,bodydstx,dsty,sprite->tileid-((sprite->fv[0]>0.0f)?0:1),sprite->xform);
  vtxc++;
  //TODO verify. no pitchforks in current maps
  return vtxc;
}

/* Multiple-mintile: FLOORFIRE
 */
 
static int fmn_render_sprite_FLOORFIRE(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite) {
  const float RING_SPACING=0.750f;
  const float RADIAL_SPACING=0.500f;
  const float xlimit=(FMN_COLC+1.0f);
  const float ylimit=(FMN_ROWC+1.0f);
  uint8_t i=sprite->bv[0];
  float radius=sprite->fv[2];
  uint8_t tileid=0xb0;
  int vtxc=0;
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
      
      // Don't render fire on solid tiles or offscreen.
      // The sky surrounding werewolf's tower is technically solid.
      int8_t xi=(int8_t)x;
      int8_t yi=(int8_t)y;
      if ((xi<0)||(yi<0)||(xi>=FMN_COLC)||(yi>=FMN_ROWC)) {
        continue;
      }
      switch (fmn_global.cellphysics[fmn_global.map[yi*FMN_COLC+xi]]) {
        case FMN_CELLPHYSICS_SOLID:
        case FMN_CELLPHYSICS_UNCHALKABLE:
        case FMN_CELLPHYSICS_SAP:
        case FMN_CELLPHYSICS_SAP_NOCHALK:
          continue;
      }
      
      if (vtxc<vtxa) fmn_mintile_world(vtxv+vtxc,x,y,tileid,0);
      vtxc++;
    }
  }
  return vtxc;
}

/* Multiple-mintile: DEADWITCH
 */
 
static int fmn_render_sprite_DEADWITCH(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite) {
  if (vtxa<4) return 4;
  int16_t pxx=sprite->x*fmn_render_global.tilesize;
  int16_t pxy=sprite->y*fmn_render_global.tilesize+2;
  float clock=sprite->fv[0];
  int frame=clock*7.0f;
  if (frame>=7) frame=6;
  fmn_mintile_screen(vtxv+0,pxx-8,pxy+1,sprite->tileid-0x0f+frame,0);
  fmn_mintile_screen(vtxv+1,pxx+8,pxy+2,sprite->tileid+0x01+frame,0);
  fmn_mintile_screen(vtxv+2,pxx-2,pxy+4,sprite->tileid+0x01+frame,0);
  fmn_mintile_screen(vtxv+3,pxx,pxy,sprite->tileid,sprite->xform);
  return 4;
}

/* Multiple-mintile: PANDA
 */
 
static int fmn_render_sprite_PANDA(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite) {
  const int16_t ts=fmn_render_global.tilesize;
  int vtxc=(sprite->bv[1]?4:6); // hands if awake, but if sleeping they are built-in to the body.
  if (vtxa<vtxc) return vtxc;
  const int16_t liftrange=ts-3;
  int16_t pxx=sprite->x*ts;
  int16_t pxy=sprite->y*ts;
  // body:
  vtxv[0].x=pxx;
  vtxv[0].y=pxy;
  vtxv[0].tileid=sprite->tileid;
  vtxv[0].xform=0;
  vtxv[1].x=pxx+ts;
  vtxv[1].y=pxy;
  vtxv[1].tileid=sprite->tileid+0x01;
  vtxv[1].xform=0;
  if (sprite->bv[1]) { // sleeping
    vtxv[0].tileid+=4;
    vtxv[1].tileid+=4;
    vtxv[2].x=pxx;
    vtxv[2].y=pxy+ts;
    vtxv[2].tileid=sprite->tileid+0x10;
    vtxv[2].xform=0;
    vtxv[3].x=pxx+ts;
    vtxv[3].y=pxy+ts;
    vtxv[3].tileid=sprite->tileid+0x11;
    vtxv[3].xform=0;
    return vtxc;
  }
  // hands:
  vtxv[2].x=pxx;
  vtxv[2].y=pxy+ts-3;
  vtxv[2].tileid=sprite->tileid+0x0e;
  vtxv[2].xform=0;
  vtxv[3].x=pxx+ts;
  vtxv[3].y=pxy+ts-3;
  vtxv[3].tileid=sprite->tileid+0x0f;
  vtxv[3].xform=0;
  // desk:
  vtxv[4].x=pxx;
  vtxv[4].y=pxy+ts;
  vtxv[4].tileid=sprite->tileid+0x10;
  vtxv[4].xform=0;
  vtxv[5].x=pxx+ts;
  vtxv[5].y=pxy+ts;
  vtxv[5].tileid=sprite->tileid+0x11;
  vtxv[5].xform=0;
  // Hands go up based on rating (fv[0]), <0=left >0=right.
  if (sprite->fv[0]<0.0f) {
    vtxv[2].y+=sprite->fv[0]*liftrange;
  } else {
    vtxv[3].y-=sprite->fv[0]*liftrange;
  }
  // Rage face and highlighted desk, fv[1]
  if (sprite->fv[1]>0.0f) {
    vtxv[0].tileid+=2;
    vtxv[1].tileid+=2;
    if (fmn_render_global.animframec&8) {
      vtxv[4].tileid+=2;
      vtxv[5].tileid+=2;
    }
  }
  return vtxc;
}

/* Multiple mintile: TALL2
 */
 
static int fmn_render_sprite_TALL2(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite) {
  if (vtxa>=2) {
    if (sprite->xform&FMN_XFORM_YREV) {
      fmn_mintile_world(vtxv+0,sprite->x,sprite->y,sprite->tileid+0x10,sprite->xform);
      fmn_mintile_world(vtxv+1,sprite->x,sprite->y+1.0f,sprite->tileid,sprite->xform);
    } else {
      fmn_mintile_world(vtxv+0,sprite->x,sprite->y,sprite->tileid,sprite->xform);
      fmn_mintile_world(vtxv+1,sprite->x,sprite->y+1.0f,sprite->tileid+0x10,sprite->xform);
    }
  }
  return 2;
}

/* Single-mintile sprite styles.
 */
 
static void fmn_render_sprite_TILE(struct fmn_draw_mintile *vtx,struct fmn_sprite *sprite) {
  fmn_mintile_sprite(vtx,sprite);
}
 
static void fmn_render_sprite_TWOFRAME(struct fmn_draw_mintile *vtx,struct fmn_sprite *sprite) {
  fmn_mintile_world(vtx,sprite->x,sprite->y,sprite->tileid+((fmn_render_global.animframec>>3)&1),sprite->xform);
}
 
static void fmn_render_sprite_FOURFRAME(struct fmn_draw_mintile *vtx,struct fmn_sprite *sprite) {
  fmn_mintile_world(vtx,sprite->x,sprite->y,sprite->tileid+((fmn_render_global.animframec>>3)&3),sprite->xform);
}
 
static void fmn_render_sprite_EIGHTFRAME(struct fmn_draw_mintile *vtx,struct fmn_sprite *sprite) {
  fmn_mintile_world(vtx,sprite->x,sprite->y,sprite->tileid+((fmn_render_global.animframec>>1)&7),sprite->xform);
}

/* Render one sprite with one of the mintile styles.
 * (vtxa) must be at least one. If we're producing more than one, we check.
 */
 
static int fmn_render_sprite_mintile(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite) {
  switch (sprite->style) {
    case FMN_SPRITE_STYLE_TILE: fmn_render_sprite_TILE(vtxv,sprite); return 1;
    case FMN_SPRITE_STYLE_HERO: return fmn_render_sprite_HERO(vtxv,vtxa,sprite);
    case FMN_SPRITE_STYLE_FOURFRAME: fmn_render_sprite_FOURFRAME(vtxv,sprite); return 1;
    case FMN_SPRITE_STYLE_FIRENOZZLE: return fmn_render_sprite_FIRENOZZLE(vtxv,vtxa,sprite);
    case FMN_SPRITE_STYLE_FIREWALL: return fmn_render_sprite_FIREWALL(vtxv,vtxa,sprite);
    case FMN_SPRITE_STYLE_DOUBLEWIDE: return fmn_render_sprite_DOUBLEWIDE(vtxv,vtxa,sprite);
    case FMN_SPRITE_STYLE_PITCHFORK: return fmn_render_sprite_PITCHFORK(vtxv,vtxa,sprite);
    case FMN_SPRITE_STYLE_TWOFRAME: fmn_render_sprite_TWOFRAME(vtxv,sprite); return 1;
    case FMN_SPRITE_STYLE_EIGHTFRAME: fmn_render_sprite_EIGHTFRAME(vtxv,sprite); return 1;
    case FMN_SPRITE_STYLE_WEREWOLF: return fmn_render_sprite_WEREWOLF(vtxv,vtxa,sprite);
    case FMN_SPRITE_STYLE_FLOORFIRE: return fmn_render_sprite_FLOORFIRE(vtxv,vtxa,sprite);
    case FMN_SPRITE_STYLE_DEADWITCH: return fmn_render_sprite_DEADWITCH(vtxv,vtxa,sprite);
    case FMN_SPRITE_STYLE_PANDA: return fmn_render_sprite_PANDA(vtxv,vtxa,sprite);
    case FMN_SPRITE_STYLE_TALL2: return fmn_render_sprite_TALL2(vtxv,vtxa,sprite);
  }
  return 0;
}

/* Render one batch of sprites.
 * Caller confirms they are batchable.
 */

#define FMN_MINTILE_VTXA 256
static struct fmn_draw_mintile fmn_mintile_vtxv[FMN_MINTILE_VTXA];
 
static void fmn_render_sprite_batch(struct fmn_sprite **v,int c,uint8_t include_hero) {
  if (c<1) return;
  
  // Pick off non-mintile sprites, which must arrive individually.
  if (c==1) switch (v[0]->style) {
    case FMN_SPRITE_STYLE_SCARYDOOR: fmn_render_sprite_SCARYDOOR(v[0]); return;
    case FMN_SPRITE_STYLE_SLIDESHOW: fmn_render_sprite_SLIDESHOW(v[0]); return;
    case FMN_SPRITE_STYLE_DRAGON: fmn_render_sprite_DRAGON(v[0]); return;
  }
  
  // Batchable mintile-only sprites.
  int imageid=v[0]->imageid;
  int vtxc=0;
  while (c>0) {
  
    // Hero can be turned off, if we're drawing her separately on top of transitions.
    // Other sprites don't get this treatment.
    if (!include_hero&&(v[0]->controller==FMN_SPRCTL_hero)) {
      v++;
      c--;
      continue;
    }
  
    int err=fmn_render_sprite_mintile(fmn_mintile_vtxv+vtxc,FMN_MINTILE_VTXA-vtxc,*v);
    if (err<=0) {
      v++;
      c--;
      continue;
    }
    if (vtxc<=FMN_MINTILE_VTXA-err) {
      vtxc+=err;
      v++;
      c--;
      if (vtxc>=FMN_MINTILE_VTXA) { // don't let it be full
        fmn_draw_mintile(fmn_mintile_vtxv,vtxc,imageid);
        vtxc=0;
      }
      continue;
    }
    if (vtxc) {
      // Flush the buffer and try again.
      fmn_draw_mintile(fmn_mintile_vtxv,vtxc,imageid);
      vtxc=0;
      continue;
    } else {
      // How the heck many vertices is this sprite making? Skip it.
      v++;
      c--;
    }
  }
  if (vtxc) fmn_draw_mintile(fmn_mintile_vtxv,vtxc,imageid);
}

/* Batching of sprites.
 * Anything that we know uses only mintile is batchable.
 * Two sprites can go in the same batch if they are both mintile-only and use the same source image.
 */
 
static inline int fmn_sprite_is_batchable(const struct fmn_sprite *sprite) {
  switch (sprite->style) {
    case FMN_SPRITE_STYLE_HIDDEN: // we'll skip it, but yes call it ok.
    case FMN_SPRITE_STYLE_TILE:
    case FMN_SPRITE_STYLE_HERO: // we might want maxtile eventually
    case FMN_SPRITE_STYLE_FOURFRAME:
    case FMN_SPRITE_STYLE_FIRENOZZLE:
    case FMN_SPRITE_STYLE_FIREWALL:
    case FMN_SPRITE_STYLE_DOUBLEWIDE:
    case FMN_SPRITE_STYLE_PITCHFORK:
    case FMN_SPRITE_STYLE_TWOFRAME:
    case FMN_SPRITE_STYLE_EIGHTFRAME:
    case FMN_SPRITE_STYLE_WEREWOLF:
    case FMN_SPRITE_STYLE_FLOORFIRE: // we have some discretion, could do this differently if we like.
    case FMN_SPRITE_STYLE_DEADWITCH:
    case FMN_SPRITE_STYLE_PANDA:
    case FMN_SPRITE_STYLE_TALL2:
      return 1;
  }
  return 0;
}

// (a) is presumed batchable, caller should check beforehand.
static inline int fmn_sprites_same_batch(const struct fmn_sprite *a,const struct fmn_sprite *b) {
  if (a->imageid!=b->imageid) return 0;
  return fmn_sprite_is_batchable(b);
}

/* Render all sprites.
 */
 
void fmn_render_sprites(uint8_t include_hero) {
  struct fmn_sprite **p=(struct fmn_sprite**)fmn_global.spritev;
  int i=0;
  while (i<fmn_global.spritec) {
    struct fmn_sprite **batchv=p;
    int batchc=1;
    if (fmn_sprite_is_batchable(batchv[0])) {
      while ((i+batchc<fmn_global.spritec)&&fmn_sprites_same_batch(batchv[0],batchv[batchc])) batchc++;
    }
    fmn_render_sprite_batch(batchv,batchc,include_hero);
    p+=batchc;
    i+=batchc;
  }
}

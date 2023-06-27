#include "fmn_render_internal.h"
#include "app/hero/fmn_hero.h"

/* Local context.
 * fmn_render_sprite_HERO() is non-re-entrant.
 */
 
static struct fmn_rh {
  struct fmn_draw_mintile *vtxv;
  int vtxc,vtxa;
  struct fmn_sprite *sprite;
  int tilesize;
  int framec;
  int itemtime;
} fmn_rh={0};

/* Add vertex.
 */
 
static inline void fmn_hero_vtx(float x,float y,uint8_t tileid,uint8_t xform) {
  if (fmn_rh.vtxc<fmn_rh.vtxa) {
    struct fmn_draw_mintile *vtx=fmn_rh.vtxv+fmn_rh.vtxc;
    vtx->x=x*fmn_rh.tilesize;
    vtx->y=y*fmn_rh.tilesize;
    vtx->tileid=tileid;
    vtx->xform=xform;
  }
  fmn_rh.vtxc++;
}

/* Two wee overlays that happen at sprite time, rather than the distinct "overlay" phase.
 * um
 * Can we not just do these in the overlay phase?
 */
 
static int fmn_render_hero_curse() {
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-1.0f,0x73,(fmn_rh.framec&0x10)?FMN_XFORM_XREV:0);
  return fmn_rh.vtxc;
}

static const struct fmn_hero_cheese_schedule {
  uint8_t dtile;
  uint8_t xform;
} fmn_hero_cheese_schedule[]={
  {0,0},
  {1,0},
  {2,0},
  {3,0},
  {4,0},
  {4,FMN_XFORM_XREV},
  {3,FMN_XFORM_XREV},
  {2,FMN_XFORM_XREV},
  {1,FMN_XFORM_XREV},
  {0,FMN_XFORM_XREV},
  {1,FMN_XFORM_XREV},
  {2,FMN_XFORM_XREV},
  {3,FMN_XFORM_XREV},
  {4,FMN_XFORM_XREV},
  {4,0},
  {3,0},
  {2,0},
  {1,0},
};

static int fmn_render_hero_cheese() {
  int schedulec=sizeof(fmn_hero_cheese_schedule)/sizeof(fmn_hero_cheese_schedule[0]);
  int frame=(fmn_rh.framec/2)%schedulec;
  const struct fmn_hero_cheese_schedule *src=fmn_hero_cheese_schedule+frame;
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-1.3f,0x80+src->dtile,src->xform);
  return fmn_rh.vtxc;
}

/* Injury, a unique state.
 */
 
static int fmn_render_hero_injury() {
  uint8_t tileid_body=0,tileid_head=0,tileid_hat=0;
  float hatdy=0.0f;
  int behatted=(fmn_global.selected_item==FMN_ITEM_HAT)&&fmn_global.itemv[FMN_ITEM_HAT];
  if (fmn_global.transmogrification==1) { // pumpkin
    if (fmn_rh.framec&4) {
      tileid_hat=behatted?0x0f:0x33;
      tileid_body=0x2e;
    } else {
      tileid_hat=behatted?0x0e:0x03;
      tileid_body=0x2d;
    }
    hatdy=-0.375f;
  } else { // normal
    if (behatted) {
      tileid_hat=(fmn_rh.framec&4)?0x0e:0x0f;
      tileid_head=(fmn_rh.framec&4)?0x13:0x43;
      tileid_body=(fmn_rh.framec&4)?0x23:0x53;
    } else {
      tileid_hat=(fmn_rh.framec&4)?0x03:0x33;
      tileid_head=tileid_hat+0x10;
      tileid_body=tileid_hat+0x20;
    }
    hatdy=-0.75f;
  }
  if (fmn_global.injury_time>=0.8f) ;
  else if (fmn_global.injury_time>=0.4f) hatdy+=(0.8f-fmn_global.injury_time)*-1.25f;
  else hatdy+=fmn_global.injury_time*-1.25f;
  if (tileid_body) fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y,tileid_body,fmn_rh.sprite->xform);
  if (tileid_head) fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.45f,tileid_head,fmn_rh.sprite->xform);
  if (tileid_hat) fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y+hatdy,tileid_hat,fmn_rh.sprite->xform);
  if (fmn_global.curse_time>0.0f) fmn_render_hero_curse();
  return fmn_rh.vtxc;
}
 
static int fmn_render_hero_pumpkin() {
  uint8_t xform=(fmn_global.last_horz_dir==FMN_DIR_W)?FMN_XFORM_XREV:0;
  uint8_t tileid_body=0x1d;
  uint8_t tileid_hat=0x00;
  float hatdy=-0.375f;
  if ((fmn_global.selected_item==FMN_ITEM_HAT)&&fmn_global.itemv[FMN_ITEM_HAT]) {
    switch (tileid_hat) {
      case 0x00: tileid_hat=0x3c; break;
      case 0x01: tileid_hat=0x3d; break;
      case 0x02: tileid_hat=0x3e; break;
      case 0x1b: tileid_hat=0x3c; break;
      case 0x1c: tileid_hat=0x3f; break;
    }
  }
  if (fmn_global.walking) switch (fmn_rh.framec&0x18) {
    case 0x08: tileid_body+=0x01; break;
    case 0x18: tileid_body+=0x02; break;
  }
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y,tileid_body,xform);
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y+hatdy,tileid_hat,xform);
  if (fmn_global.curse_time>0.0f) fmn_render_hero_curse();
  return fmn_rh.vtxc;
}

static int fmn_render_hero_spell_repudiation() {
  if (fmn_global.spell_repudiation>111) { // total run time in frames. should be 15 mod 16.
    fmn_global.spell_repudiation=111;
  }
  fmn_global.spell_repudiation--;
  uint8_t frame=(fmn_global.spell_repudiation&0x10)?1:0;
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.000f,0x20,0);
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.4375f,0x2b+frame,0);
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.750f,0x1b+frame,0);
  return fmn_rh.vtxc;
}

static int fmn_render_hero_broom() {
  uint8_t xform=(fmn_global.last_horz_dir==FMN_DIR_E)?FMN_XFORM_XREV:0;
  float dy=(fmn_rh.framec&0x20)?(-1.0f/fmn_rh.tilesize):0.0f;
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.1875f+dy,0x57,xform);
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.5625f+dy,0x12,xform);
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.8750f+dy,0x02,xform);
  return fmn_rh.vtxc;
}

static int fmn_render_hero_snowglobe() {
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.1875f,0x09,0);
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.75f,0x00,0);
  switch (fmn_global.wand_dir) {
    case FMN_DIR_W: fmn_hero_vtx(fmn_rh.sprite->x-0.1875f,fmn_rh.sprite->y-0.1875f,0x5c,0); break;
    case FMN_DIR_E: fmn_hero_vtx(fmn_rh.sprite->x+0.1875f,fmn_rh.sprite->y-0.1875f,0x6c,0); break;
    case FMN_DIR_S: fmn_hero_vtx(fmn_rh.sprite->x        ,fmn_rh.sprite->y-0.1250f,0x7c,0); break;
    case FMN_DIR_N: fmn_hero_vtx(fmn_rh.sprite->x        ,fmn_rh.sprite->y-0.5000f,0x8c,0); break;
    default: fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.1875f,0x4c,0); break;
  }
  return fmn_rh.vtxc;
}

static int fmn_render_hero_wand() {
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.1875f,0x09,0);
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.75f,0x00,0);
  switch (fmn_global.wand_dir) {
    case FMN_DIR_W: fmn_hero_vtx(fmn_rh.sprite->x-0.1875f,fmn_rh.sprite->y-0.1875f,0x29,0); break;
    case FMN_DIR_E: fmn_hero_vtx(fmn_rh.sprite->x+0.1875f,fmn_rh.sprite->y-0.1875f,0x39,0); break;
    case FMN_DIR_S: fmn_hero_vtx(fmn_rh.sprite->x        ,fmn_rh.sprite->y-0.1250f,0x49,0); break;
    case FMN_DIR_N: fmn_hero_vtx(fmn_rh.sprite->x        ,fmn_rh.sprite->y-0.5000f,0x59,0); break;
    default: fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.1875f,0x19,0); break;
  }
  return fmn_rh.vtxc;
}

static int fmn_render_hero_violin() {
  fmn_hero_vtx(fmn_rh.sprite->x+0.125f,fmn_rh.sprite->y-0.1875f,0x28,0);
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.750f,0x00,0);
  if (fmn_global.wand_dir) { // "wand_dir" nonzero when stroking violin
    const float xrange=5.0f/fmn_rh.tilesize;
    const float yrange=-2.0f/fmn_rh.tilesize;
    const int phaselen=60;
    int phase=fmn_rh.framec%phaselen;
    float displacement=((phase>=(phaselen>>1))?(phaselen-phase):phase)/(phaselen*0.5f);
    fmn_hero_vtx(fmn_rh.sprite->x+displacement*xrange,fmn_rh.sprite->y-0.250f+displacement*yrange,0x48,0);
  } else {
    fmn_hero_vtx(fmn_rh.sprite->x+0.125f,fmn_rh.sprite->y-0.1875f,0x38,0);
  }
  return fmn_rh.vtxc;
}

/* The easy bits: body, head, hat.
 */

static const uint8_t fmn_hero_walk_framev[]={0,0,1,1,2,2,2,1,1,0,0,3,3,4,4,4,3,3};

static void fmn_render_hero_body(uint8_t tileid,uint8_t xform) {
  tileid+=0x20;
  if (fmn_global.walking) {
    int bodyclock=(fmn_rh.framec>>1)%sizeof(fmn_hero_walk_framev);
    tileid+=0x10*fmn_hero_walk_framev[bodyclock];
  }
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y,tileid,xform);
}

static void fmn_render_hero_head(uint8_t tileid,uint8_t xform) {
  if (fmn_global.curse_time>0.0f) {
    tileid+=0x70;
  } else {
    tileid+=0x10;
    switch (fmn_global.active_item) {
      case FMN_ITEM_SHOVEL: switch (fmn_global.facedir) {
          case FMN_DIR_W: case FMN_DIR_E: tileid=0x69; break;
          case FMN_DIR_S: tileid=0x68; break;
        } break;
    }
  }
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-0.4375f,tileid,xform);
}

static void fmn_render_hero_hat(uint8_t tileid,uint8_t xform) {
  float dy=-0.75f;
  if ((fmn_global.selected_item==FMN_ITEM_HAT)&&fmn_global.itemv[FMN_ITEM_HAT]) {
    switch (tileid) {
      case 0x00: tileid=0x3c; break;
      case 0x01: tileid=0x3d; break;
      case 0x02: tileid=0x3e; break;
      case 0x1b: tileid=0x3c; break;
      case 0x1c: tileid=0x3f; break;
    }
  }
  fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y+dy,tileid,xform);
}

/* Items in hand.
 */

static const struct fmn_hero_item_layout {
  uint8_t tileid;
  int8_t dx,dy; // pixels, based on 16-pixel tiles
  uint8_t if_qualifier;
} fmn_hero_item_layoutv[FMN_ITEM_COUNT]={
  [FMN_ITEM_SNOWGLOBE]={0x74,-5,-3,0},
  [FMN_ITEM_PITCHER  ]={0x55,-5,-3,0},
  [FMN_ITEM_SEED     ]={0x34,-5,-3,1},
  [FMN_ITEM_COIN     ]={0x44,-5,-3,1},
  [FMN_ITEM_MATCH    ]={0x06,-5,-3,0}, // no (if_qualifier) because the last match must remain visible during illumination
  [FMN_ITEM_BROOM    ]={0x66,-5,-3,0},
  [FMN_ITEM_WAND     ]={0x08,-5,-3,0},
  [FMN_ITEM_UMBRELLA ]={0x46,-5,-8,0},
  [FMN_ITEM_FEATHER  ]={0x05,-5,-3,0},
  [FMN_ITEM_SHOVEL   ]={0x07,-5,-3,0},
  [FMN_ITEM_COMPASS  ]={0x64,-5,-3,0},
  [FMN_ITEM_VIOLIN   ]={0x18,-5, 0,0},
  [FMN_ITEM_CHALK    ]={0x58,-5,-3,0},
  [FMN_ITEM_BELL     ]={0x04,-5,-3,0},
  [FMN_ITEM_CHEESE   ]={0x54,-5,-3,1},
};

static void fmn_render_hero_item(uint8_t tileid,uint8_t xform) {
  if (!fmn_global.itemv[fmn_global.selected_item]) return;
  struct fmn_hero_item_layout altlayout;
  const struct fmn_hero_item_layout *layout=fmn_hero_item_layoutv+fmn_global.selected_item;
  if (!layout->tileid) return; // tile 0x00 is real (it's the hat) but we use it to mean "none"
  if (layout->if_qualifier) { // Some items disappear when q==0.
    if (!fmn_global.itemqv[fmn_global.selected_item]) return;
  }
  
  // Lots of items have more than one face, but weren't worth entirely special handling...
  #define ALT { altlayout=*layout; layout=&altlayout; }
  #define ACTIVE (fmn_global.active_item==fmn_global.selected_item)
  if (ACTIVE) fmn_rh.itemtime++;
  else fmn_rh.itemtime=0;
  switch (fmn_global.selected_item) {
  
    case FMN_ITEM_PITCHER: if (ACTIVE) {
        uint8_t tileid=0x65;
        if (fmn_global.wand_dir) tileid=0xe6+fmn_global.wand_dir;
        switch (fmn_global.facedir) {
          case FMN_DIR_N: fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-5.0f/16.0f,tileid,FMN_XFORM_XREV|FMN_XFORM_SWAP); break;
          case FMN_DIR_S: fmn_hero_vtx(fmn_rh.sprite->x-0.0625f,fmn_rh.sprite->y-0.0625f,tileid,FMN_XFORM_XREV); break;
          case FMN_DIR_W: fmn_hero_vtx(fmn_rh.sprite->x-5.0f/16.0f,fmn_rh.sprite->y-3.0f/16.0f,tileid,0); break;
          case FMN_DIR_E: fmn_hero_vtx(fmn_rh.sprite->x+5.0f/16.0f,fmn_rh.sprite->y-3.0f/16.0f,tileid,FMN_XFORM_XREV); break;
        }
        return;
      } break;
    
    case FMN_ITEM_MATCH: {
        if (fmn_global.match_illumination_time>0.0f) {
          ALT
          switch (fmn_rh.framec&0x18) {
            case 0x00: altlayout.tileid=0x16; break;
            case 0x08: altlayout.tileid=0x16; break;
            case 0x10: altlayout.tileid=0x36; break;
            case 0x18: altlayout.tileid=0x26; break;
          }
        } else if (!fmn_global.itemqv[FMN_ITEM_MATCH]) {
          return;
        }
      } break;
      
    case FMN_ITEM_UMBRELLA: if (ACTIVE) {
        switch (fmn_global.facedir) {
          case FMN_DIR_N: fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-10.0f/16.0f,0x56,FMN_XFORM_SWAP); return;
          case FMN_DIR_S: fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y+5.0f/16.0f,0x56,FMN_XFORM_SWAP|FMN_XFORM_XREV); return;
        }
        ALT
        altlayout.tileid=0x56;
        altlayout.dx=-9;
        altlayout.dy=-4;
      } break;
    
    case FMN_ITEM_FEATHER: if (ACTIVE) {
        int8_t dx=layout->dx,dy=layout->dy;
        uint8_t tileid=0x05;
        switch (fmn_global.facedir) {
          case FMN_DIR_N: dx=2; dy=-13; xform=FMN_XFORM_SWAP; break;
          case FMN_DIR_S: dx=-2; dy=3; xform=FMN_XFORM_SWAP|FMN_XFORM_XREV; break;
          case FMN_DIR_E: dx=-dx; xform=FMN_XFORM_XREV; break;
        }
        switch (fmn_rh.itemtime&0x18) {
          case 0x00: tileid+=0x10; break;
          case 0x08: tileid+=0x20; break;
          case 0x10: tileid+=0x30; break;
          case 0x18: tileid+=0x40; break;
        }
        fmn_hero_vtx(fmn_rh.sprite->x+dx/16.0f,fmn_rh.sprite->y+dy/16.0f,tileid,xform);
        return;
      } break;
      
    case FMN_ITEM_SHOVEL: if (ACTIVE) {
        uint8_t tileid=(fmn_rh.itemtime<15)?0x37:0x47;
        switch (fmn_global.facedir) {
          case FMN_DIR_N: fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-10.0f/16.0f,tileid,0); return;
          case FMN_DIR_S: fmn_hero_vtx(fmn_rh.sprite->x,fmn_rh.sprite->y-3.0f/16.0f,tileid,0); return;
          case FMN_DIR_W: fmn_hero_vtx(fmn_rh.sprite->x-2.0f/16.0f,fmn_rh.sprite->y-3.0f/16.0f,tileid,FMN_XFORM_XREV); return;
          case FMN_DIR_E: fmn_hero_vtx(fmn_rh.sprite->x+2.0f/16.0f,fmn_rh.sprite->y-3.0f/16.0f,tileid,0); return;
        }
      } break;
    
    case FMN_ITEM_BELL: ALT if (ACTIVE) altlayout.tileid=(fmn_rh.itemtime&16)?0x14:0x24; break;
  }
  #undef ALT
  #undef ACTIVE
  
  xform=0;
  float dstx=fmn_rh.sprite->x;
  float dsty=fmn_rh.sprite->y+layout->dy/16.0f;
  if ((fmn_global.facedir==FMN_DIR_E)||(fmn_global.facedir==FMN_DIR_N)) {
    dstx-=layout->dx/16.0f;
    xform=FMN_XFORM_XREV;
  } else {
    dstx+=layout->dx/16.0f;
  }
  fmn_hero_vtx(dstx,dsty,layout->tileid,xform);
}

/* Render hero, main entry point.
 */
 
int fmn_render_sprite_HERO(struct fmn_draw_mintile *vtxv,int vtxa,struct fmn_sprite *sprite) {

  // Skip every other frame if invisible.
  if ((fmn_global.invisibility_time>0.0f)&&(fmn_rh.framec&1)) return 0;
  
  // Populate local context.
  fmn_rh.vtxv=vtxv;
  fmn_rh.vtxc=0;
  fmn_rh.vtxa=vtxa;
  fmn_rh.sprite=sprite;
  fmn_rh.tilesize=fmn_render_global.tilesize;
  fmn_rh.framec=fmn_render_global.framec;
  
  // Injured is its own thing.
  if (fmn_global.injury_time>0.0f) return fmn_render_hero_injury();
  
  // Pumpkin is its own thing. (if not injured or invisible)
  if (fmn_global.transmogrification==1) return fmn_render_hero_pumpkin();
  
  // A few more special states. None of these needs to worry about curse.
  if (fmn_global.spell_repudiation) return fmn_render_hero_spell_repudiation();
  switch (fmn_global.active_item) {
    case FMN_ITEM_SNOWGLOBE: return fmn_render_hero_snowglobe();
    case FMN_ITEM_BROOM: return fmn_render_hero_broom();
    case FMN_ITEM_WAND: return fmn_render_hero_wand();;
    case FMN_ITEM_VIOLIN: return fmn_render_hero_violin();
  }
    
  // Mostly our tiles are arranged in three columns, with the third facing left.
  uint8_t tileid=0x00,xform=0;
  switch (fmn_global.facedir) {
    case FMN_DIR_N: tileid=0x01; break;
    case FMN_DIR_W: tileid=0x02; break;
    case FMN_DIR_E: tileid=0x02; xform=FMN_XFORM_XREV; break;
  }
  
  // A standard witch has 4 pieces: body, head, hat, item. These go in different orders depending on facedir.
  switch (fmn_global.facedir) {
    case FMN_DIR_N: {
        fmn_render_hero_item(tileid,xform);
        fmn_render_hero_body(tileid,xform);
        fmn_render_hero_head(tileid,xform);
        fmn_render_hero_hat(tileid,xform);
      } break;
    case FMN_DIR_S: {
        fmn_render_hero_body(tileid,xform);
        fmn_render_hero_head(tileid,xform);
        fmn_render_hero_hat(tileid,xform);
        fmn_render_hero_item(tileid,xform);
      } break;
    case FMN_DIR_W: case FMN_DIR_E: {
        fmn_render_hero_body(tileid,xform);
        fmn_render_hero_item(tileid,xform);
        fmn_render_hero_head(tileid,xform);
        fmn_render_hero_hat(tileid,xform);
      } break;
  }
  
  // Cheese and curse highlights.
  if (fmn_global.cheesing) fmn_render_hero_cheese();
  if (fmn_global.curse_time>0.0f) fmn_render_hero_curse();
  
  return fmn_rh.vtxc;
}

/* Render hero, no shared vertices.
 * I think the maximum vertex count is 6. TODO verify.
 */
 
void fmn_render_hero(struct fmn_sprite *sprite,int16_t addx,int16_t addy) {
  struct fmn_draw_mintile vtxv[8];
  int vtxa=sizeof(vtxv)/sizeof(vtxv[0]);
  int vtxc=fmn_render_sprite_HERO(vtxv,vtxa,sprite);
  if (vtxc>vtxa) {
    fmn_log("!!!!! %s %s:%d vtxc=%d",__func__,__FILE__,__LINE__,vtxc);
    return;
  }
  struct fmn_draw_mintile *v=vtxv;
  int i=vtxc;
  for (;i-->0;v++) {
    v->x+=addx;
    v->y+=addy;
  }
  fmn_draw_mintile(vtxv,vtxc,sprite->imageid);
}

/* Underlay.
 */
 
static int fmn_render_find_hero(struct fmn_sprite *sprite,void *userdata) {
  if (sprite->controller==FMN_SPRCTL_hero) {
    *(void**)userdata=sprite;
    return 1;
  }
  return 0;
}
 
static void fmn_render_hero_underlay_broom(int16_t addx,int16_t addy) {
  if (fmn_render_global.framec&1) return;
  struct fmn_sprite *sprite=0;
  fmn_sprites_for_each(fmn_render_find_hero,&sprite);
  if (!sprite) return;
  struct fmn_draw_mintile vtx={
    .x=sprite->x*fmn_render_global.tilesize+addx,
    .y=(sprite->y+0.3f)*fmn_render_global.tilesize+addy,
    .tileid=(fmn_render_global.framec&32)?0x67:0x77,
    .xform=0,
  };
  fmn_draw_mintile(&vtx,1,sprite->imageid);
}

// also used for seeds and pitcher
static void fmn_render_hero_underlay_shovel(int16_t addx,int16_t addy) {
  if (fmn_global.transmogrification) return; // No sense showing while pumpkinned.
  if (addx||addy) return; // don't draw this while transitioning
  if ((fmn_global.shovelx<0)||(fmn_global.shovelx>=FMN_COLC)) return;
  if ((fmn_global.shovely<0)||(fmn_global.shovely>=FMN_ROWC)) return;
  struct fmn_draw_mintile vtx={
    .x=fmn_global.shovelx*fmn_render_global.tilesize+(fmn_render_global.tilesize>>1),
    .y=fmn_global.shovely*fmn_render_global.tilesize+(fmn_render_global.tilesize>>1),
    .tileid=(fmn_render_global.framec&0x10)?0x17:0x27,
    .xform=0,
  };
  fmn_draw_mintile(&vtx,1,2);
}
 
void fmn_render_hero_underlay(int16_t addx,int16_t addy) {
  if (fmn_global.hero_dead) return;
  switch (fmn_global.active_item) {
    case FMN_ITEM_BROOM: fmn_render_hero_underlay_broom(addx,addy); return;
  }
  if (fmn_global.itemv[fmn_global.selected_item]) switch (fmn_global.selected_item) {
    case FMN_ITEM_SHOVEL: fmn_render_hero_underlay_shovel(addx,addy); return;
    case FMN_ITEM_SEED: if (fmn_global.itemqv[FMN_ITEM_SEED]) fmn_render_hero_underlay_shovel(addx,addy); return;
    case FMN_ITEM_PITCHER: if (fmn_global.itemqv[FMN_ITEM_PITCHER]) fmn_render_hero_underlay_shovel(addx,addy); return;
  }
}

/* Compass overlay.
 */
 
static void fmn_render_hero_overlay_compass() {
  
  struct fmn_sprite *sprite=0;
  fmn_sprites_for_each(fmn_render_find_hero,&sprite);
  if (!sprite) return;
  
  if (fmn_global.curse_time>0.0f) {
    fmn_render_global.compassangle-=FMN_RENDER_COMPASS_RATE_MIN;
  } else if (!fmn_global.compassx&&!fmn_global.compassy) {
    fmn_render_global.compassangle+=FMN_RENDER_COMPASS_RATE_MAX;
  } else {
    float targetx=fmn_global.compassx+0.5f;
    float targety=fmn_global.compassy+0.5f;
    float tdx=targetx-sprite->x;
    float tdy=targety-sprite->y;
    float distance=sqrtf(tdx*tdx+tdy*tdy);
    if (distance>=FMN_RENDER_COMPASS_DISTANCE_MAX) {
      fmn_render_global.compassangle+=FMN_RENDER_COMPASS_RATE_MAX;
    } else {
      float angle=atan2f(tdx,-tdy);
      float ndist=distance/FMN_RENDER_COMPASS_DISTANCE_MAX;
      float minrate=FMN_RENDER_COMPASS_RATE_MIN+(FMN_RENDER_COMPASS_RATE_MAX-FMN_RENDER_COMPASS_RATE_MIN)*ndist;
      float diff=fmn_render_global.compassangle-angle;
      if (diff>M_PI) diff-=M_PI*2.0f;
      else if (diff<-M_PI) diff+=M_PI*2.0f;
      if (diff<0.0f) diff=-diff;
      fmn_render_global.compassangle+=minrate+(diff*(FMN_RENDER_COMPASS_RATE_MAX-minrate))/M_PI;
    }
  }
  if (fmn_render_global.compassangle>M_PI) fmn_render_global.compassangle-=M_PI*2.0f;
  else if (fmn_render_global.compassangle<-M_PI) fmn_render_global.compassangle+=M_PI*2.0f;
  
  float dstx=sprite->x+sinf(fmn_render_global.compassangle);
  float dsty=sprite->y-cosf(fmn_render_global.compassangle);
  struct fmn_draw_maxtile vtx={
    .x=dstx*fmn_render_global.tilesize,
    .y=dsty*fmn_render_global.tilesize,
    .tileid=0x63,
    .rotate=(int8_t)((fmn_render_global.compassangle*128.0f)/M_PI),
    .size=fmn_render_global.tilesize,
    .xform=0,
    .tr=0,
    .tg=0,
    .tb=0,
    .ta=0,
    .pr=0x80,
    .pg=0x80,
    .pb=0x80,
    .alpha=0xff,
  };
  fmn_draw_maxtile(&vtx,1,sprite->imageid);
}

/* Showoff-item overlay.
 */

static void fmn_render_hero_overlay_showoff() {
  if ((fmn_render_global.framec&7)<2) return;
  struct fmn_sprite *sprite=0;
  fmn_sprites_for_each(fmn_render_find_hero,&sprite);
  if (!sprite) return;
  struct fmn_draw_mintile vtx={
    .x=sprite->x*fmn_render_global.tilesize,
    .y=(sprite->y-1.5f)*fmn_render_global.tilesize,
    .tileid=0xf0+fmn_global.show_off_item,
    .xform=0,
  };
  if (((fmn_global.show_off_item&0x0f)==FMN_ITEM_PITCHER)&&(fmn_global.show_off_item&0xf0)) {
    vtx.tileid=0xe2+(fmn_global.show_off_item>>4);
  }
  fmn_draw_mintile(&vtx,1,sprite->imageid);
}

/* Wand overlay (show spell in progress)
 */
 
/* The longest valid spell is 8 units, and our buffer goes up to 12.
 * We'll display up to 10.
 * Beyond that, we show 9 glyphs and an ellipsis in the first position.
 */
#define FMN_WAND_DISPLAY_LIMIT 10
#define FMN_WAND_VERTEX_LIMIT (FMN_WAND_DISPLAY_LIMIT*2+2)
 
static void fmn_render_hero_overlay_wand() {
  struct fmn_draw_mintile vtxv[FMN_WAND_VERTEX_LIMIT];
  int vtxc=0;
  uint8_t spellv[20]; // >FMN_WAND_DISPLAY_LIMIT, also >FMN_HERO_SPELL_LIMIT, ensure we can capture the whole thing.
  int spellc=fmn_hero_get_spell_in_progress(spellv,sizeof(spellv));
  if ((spellc<0)||(spellc>sizeof(spellv))) spellc=0;
  if (spellc>FMN_WAND_DISPLAY_LIMIT) {
    int cropc=spellc-FMN_WAND_DISPLAY_LIMIT;
    memmove(spellv,spellv+cropc,FMN_WAND_DISPLAY_LIMIT);
    spellc=FMN_WAND_DISPLAY_LIMIT;
    spellv[0]=0xff; // signal for ellipsis
  }
  
  // TODO Top or bottom, whichever has more room.
  // TODO Clamp to screen horizontally.
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  int16_t dsty=(heroy-1.0f)*fmn_render_global.tilesize;
  int16_t dstx=herox*fmn_render_global.tilesize;
  dstx-=3+((spellc*7)>>1);
  
  vtxv[vtxc++]=(struct fmn_draw_mintile){dstx,dsty,0xd7,0}; dstx+=7;
  int i=0; for (;i<spellc;i++) {
    uint8_t tileid=0xda,xform=0;
    int16_t cheatx=0,cheaty=0;
    switch (spellv[i]) {
      case 0xff: tileid=0xdb; break;
      case FMN_DIR_N: break;
      case FMN_DIR_S: xform=FMN_XFORM_YREV; break;
      case FMN_DIR_W: cheaty=-1; cheatx=1; xform=FMN_XFORM_SWAP; break;
      case FMN_DIR_E: cheaty=-1; cheatx=-1; xform=FMN_XFORM_SWAP|FMN_XFORM_YREV; break;
    }
    vtxv[vtxc++]=(struct fmn_draw_mintile){dstx,dsty,0xd8,0};
    vtxv[vtxc++]=(struct fmn_draw_mintile){dstx+cheatx,dsty+cheaty,tileid,xform};
    dstx+=7;
  }
  vtxv[vtxc++]=(struct fmn_draw_mintile){dstx,dsty,0xd9,0};
  fmn_draw_mintile(vtxv,vtxc,2);
}

/* Overlay dispatch.
 */
 
void fmn_render_hero_overlay(int16_t addx,int16_t addy) {
  if (addx||addy) return; // no overlay during transitions
  if (fmn_global.hero_dead) return;
  if (fmn_global.itemv[fmn_global.selected_item]) switch (fmn_global.selected_item) {
    case FMN_ITEM_COMPASS: fmn_render_hero_overlay_compass(); break;
    case FMN_ITEM_WAND: if (fmn_global.active_item==FMN_ITEM_WAND) fmn_render_hero_overlay_wand(); break;
  }
  if (fmn_global.show_off_item_time) {
    fmn_render_hero_overlay_showoff();
  }
}

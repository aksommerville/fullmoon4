#include "../fmn_gl2_internal.h"
#include "app/sprite/fmn_sprite.h"

/* Two wee overlays that happen at sprite time, rather than the distinct "overlay" phase.
 * um
 * Can we not just do these in the overlay phase?
 */
 
static void fmn_gl2_game_render_HERO_curse(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-1.0f,0x73,(DRIVER->game.framec&0x10)?FMN_XFORM_XREV:0);
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

static void fmn_gl2_game_render_HERO_cheese(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  int schedulec=sizeof(fmn_hero_cheese_schedule)/sizeof(fmn_hero_cheese_schedule[0]);
  int frame=(DRIVER->game.framec/2)%schedulec;
  const struct fmn_hero_cheese_schedule *src=fmn_hero_cheese_schedule+frame;
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-1.3f,0x80+src->dtile,src->xform);
}

/* Injury, a unique state.
 */
 
static void fmn_gl2_game_render_HERO_injury(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  uint8_t tileid_body=0,tileid_head=0,tileid_hat=0;
  float hatdy=0.0f;
  if (fmn_global.transmogrification==1) { // pumpkin
    if (DRIVER->game.framec&4) {
      tileid_hat=(fmn_global.selected_item==FMN_ITEM_HAT)?0x0f:0x33;
      tileid_body=0x2e;
    } else {
      tileid_hat=(fmn_global.selected_item==FMN_ITEM_HAT)?0x0e:0x03;
      tileid_body=0x2d;
    }
    hatdy=-0.375f;
  } else { // normal
    if (fmn_global.selected_item==FMN_ITEM_HAT) {
      tileid_hat=(DRIVER->game.framec&4)?0x0e:0x0f;
      tileid_head=(DRIVER->game.framec&4)?0x13:0x43;
      tileid_body=(DRIVER->game.framec&4)?0x23:0x53;
    } else {
      tileid_hat=(DRIVER->game.framec&4)?0x03:0x33;
      tileid_head=tileid_hat+0x10;
      tileid_body=tileid_hat+0x20;
    }
    hatdy=-0.75f;
  }
  if (fmn_global.injury_time>=0.8f) ;
  else if (fmn_global.injury_time>=0.4f) hatdy+=(0.8f-fmn_global.injury_time)*-1.25f;
  else hatdy+=fmn_global.injury_time*-1.25f;
  if (tileid_body) fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y,tileid_body,sprite->xform);
  if (tileid_head) fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.45f,tileid_head,sprite->xform);
  if (tileid_hat) fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y+hatdy,tileid_hat,sprite->xform);
  if (fmn_global.curse_time>0.0f) fmn_gl2_game_render_HERO_curse(driver,sprite);
}
 
static void fmn_gl2_game_render_HERO_pumpkin(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  uint8_t xform=(fmn_global.last_horz_dir==FMN_DIR_W)?FMN_XFORM_XREV:0;
  uint8_t tileid_body=0x1d;
  uint8_t tileid_hat=0x00;
  float hatdy=-0.375f;
  if (fmn_global.selected_item==FMN_ITEM_HAT) {
    switch (tileid_hat) {
      case 0x00: tileid_hat=0x3c; break;
      case 0x01: tileid_hat=0x3d; break;
      case 0x02: tileid_hat=0x3e; break;
      case 0x1b: tileid_hat=0x3c; break;
      case 0x1c: tileid_hat=0x3f; break;
    }
  }
  if (fmn_global.walking) switch (DRIVER->game.framec&0x18) {
    case 0x08: tileid_body+=0x01; break;
    case 0x18: tileid_body+=0x02; break;
  }
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y,tileid_body,xform);
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y+hatdy,tileid_hat,xform);
  if (fmn_global.curse_time>0.0f) fmn_gl2_game_render_HERO_curse(driver,sprite);
}

static void fmn_gl2_game_render_HERO_spell_repudiation(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  if (fmn_global.spell_repudiation>111) { // total run time in frames. should be 15 mod 16.
    fmn_global.spell_repudiation=111;
  }
  fmn_global.spell_repudiation--;
  uint8_t frame=(fmn_global.spell_repudiation&0x10)?1:0;
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.000f,0x20,0);
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.4375f,0x2b+frame,0);
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.750f,0x1b+frame,0);
}

static void fmn_gl2_game_render_HERO_BROOM(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  uint8_t xform=(fmn_global.last_horz_dir==FMN_DIR_E)?FMN_XFORM_XREV:0;
  float dy=(DRIVER->game.framec&0x20)?(-1.0f/DRIVER->game.tilesize):0.0f;
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.1875f+dy,0x57,xform);
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.5625f+dy,0x12,xform);
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.8750f+dy,0x02,xform);
}

static void fmn_gl2_game_render_HERO_SNOWGLOBE(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.1875f,0x09,0);
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.75f,0x00,0);
  switch (fmn_global.wand_dir) {
    case FMN_DIR_W: fmn_gl2_game_add_mintile_vtx(driver,sprite->x-0.1875f,sprite->y-0.1875f,0x5c,0); break;
    case FMN_DIR_E: fmn_gl2_game_add_mintile_vtx(driver,sprite->x+0.1875f,sprite->y-0.1875f,0x6c,0); break;
    case FMN_DIR_S: fmn_gl2_game_add_mintile_vtx(driver,sprite->x        ,sprite->y-0.1250f,0x7c,0); break;
    case FMN_DIR_N: fmn_gl2_game_add_mintile_vtx(driver,sprite->x        ,sprite->y-0.5000f,0x8c,0); break;
    default: fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.1875f,0x4c,0); break;
  }
}

static void fmn_gl2_game_render_HERO_WAND(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.1875f,0x09,0);
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.75f,0x00,0);
  switch (fmn_global.wand_dir) {
    case FMN_DIR_W: fmn_gl2_game_add_mintile_vtx(driver,sprite->x-0.1875f,sprite->y-0.1875f,0x29,0); break;
    case FMN_DIR_E: fmn_gl2_game_add_mintile_vtx(driver,sprite->x+0.1875f,sprite->y-0.1875f,0x39,0); break;
    case FMN_DIR_S: fmn_gl2_game_add_mintile_vtx(driver,sprite->x        ,sprite->y-0.1250f,0x49,0); break;
    case FMN_DIR_N: fmn_gl2_game_add_mintile_vtx(driver,sprite->x        ,sprite->y-0.5000f,0x59,0); break;
    default: fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.1875f,0x19,0); break;
  }
}

static void fmn_gl2_game_render_HERO_VIOLIN(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x+0.125f,sprite->y-0.1875f,0x28,0);
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.750f,0x00,0);
  if (fmn_global.wand_dir) { // "wand_dir" nonzero when stroking violin
    const float xrange=5.0f/DRIVER->game.tilesize;
    const float yrange=-2.0f/DRIVER->game.tilesize;
    const int phaselen=60;
    int phase=DRIVER->game.framec%phaselen;
    float displacement=((phase>=(phaselen>>1))?(phaselen-phase):phase)/(phaselen*0.5f);
    fmn_gl2_game_add_mintile_vtx(driver,sprite->x+displacement*xrange,sprite->y-0.250f+displacement*yrange,0x48,0);
  } else {
    fmn_gl2_game_add_mintile_vtx(driver,sprite->x+0.125f,sprite->y-0.1875f,0x38,0);
  }
}

/* The easy bits: body, head, hat.
 */

static const uint8_t fmn_gl2_hero_walk_framev[]={0,0,1,1,2,2,2,1,1,0,0,3,3,4,4,4,3,3};

static void fmn_gl2_game_render_HERO_body(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,uint8_t tileid,uint8_t xform) {
  tileid+=0x20;
  if (fmn_global.walking) {
    int bodyclock=(DRIVER->game.framec>>1)%sizeof(fmn_gl2_hero_walk_framev);
    tileid+=0x10*fmn_gl2_hero_walk_framev[bodyclock];
  }
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y,tileid,xform);
}

static void fmn_gl2_game_render_HERO_head(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,uint8_t tileid,uint8_t xform) {
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
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.4375f,tileid,xform);
}

static void fmn_gl2_game_render_HERO_hat(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,uint8_t tileid,uint8_t xform) {
  float dy=-0.75f;
  if (fmn_global.selected_item==FMN_ITEM_HAT) {
    switch (tileid) {
      case 0x00: tileid=0x3c; break;
      case 0x01: tileid=0x3d; break;
      case 0x02: tileid=0x3e; break;
      case 0x1b: tileid=0x3c; break;
      case 0x1c: tileid=0x3f; break;
    }
  }
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y+dy,tileid,xform);
}

/* Items in hand.
 */

static const struct fmn_gl2_hero_item_layout {
  uint8_t tileid;
  int8_t dx,dy; // pixels, based on 16-pixel tiles
  uint8_t if_qualifier;
} fmn_gl2_hero_item_layoutv[FMN_ITEM_COUNT]={
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

static void fmn_gl2_game_render_HERO_item(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite,uint8_t tileid,uint8_t xform) {
  if (!fmn_global.itemv[fmn_global.selected_item]) return;
  struct fmn_gl2_hero_item_layout altlayout;
  const struct fmn_gl2_hero_item_layout *layout=fmn_gl2_hero_item_layoutv+fmn_global.selected_item;
  if (!layout->tileid) return; // tile 0x00 is real (it's the hat) but we use it to mean "none"
  if (layout->if_qualifier) { // Some items disappear when q==0.
    if (!fmn_global.itemqv[fmn_global.selected_item]) return;
  }
  
  // Lots of items have more than one face, but weren't worth entirely special handling...
  #define ALT { altlayout=*layout; layout=&altlayout; }
  #define ACTIVE (fmn_global.active_item==fmn_global.selected_item)
  if (ACTIVE) DRIVER->game.itemtime++;
  else DRIVER->game.itemtime=0;
  switch (fmn_global.selected_item) {
  
    case FMN_ITEM_PITCHER: if (ACTIVE) {
        uint8_t tileid=0x65;
        if (fmn_global.wand_dir) tileid=0xe6+fmn_global.wand_dir;
        switch (fmn_global.facedir) {
          case FMN_DIR_N: fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-5.0f/16.0f,tileid,FMN_XFORM_XREV|FMN_XFORM_SWAP); break;
          case FMN_DIR_S: fmn_gl2_game_add_mintile_vtx(driver,sprite->x-0.0625f,sprite->y-0.0625f,tileid,FMN_XFORM_XREV); break;
          case FMN_DIR_W: fmn_gl2_game_add_mintile_vtx(driver,sprite->x-5.0f/16.0f,sprite->y-3.0f/16.0f,tileid,0); break;
          case FMN_DIR_E: fmn_gl2_game_add_mintile_vtx(driver,sprite->x+5.0f/16.0f,sprite->y-3.0f/16.0f,tileid,FMN_XFORM_XREV); break;
        }
        return;
      } break;
    
    case FMN_ITEM_MATCH: {
        if (fmn_global.match_illumination_time>0.0f) {
          ALT
          switch (DRIVER->game.framec&0x18) {
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
          case FMN_DIR_N: fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-10.0f/16.0f,0x56,FMN_XFORM_SWAP); return;
          case FMN_DIR_S: fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y+5.0f/16.0f,0x56,FMN_XFORM_SWAP|FMN_XFORM_XREV); return;
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
        switch (DRIVER->game.itemtime&0x18) {
          case 0x00: tileid+=0x10; break;
          case 0x08: tileid+=0x20; break;
          case 0x10: tileid+=0x30; break;
          case 0x18: tileid+=0x40; break;
        }
        fmn_gl2_game_add_mintile_vtx(driver,sprite->x+dx/16.0f,sprite->y+dy/16.0f,tileid,xform);
        return;
      } break;
      
    case FMN_ITEM_SHOVEL: if (ACTIVE) {
        uint8_t tileid=(DRIVER->game.itemtime<15)?0x37:0x47;
        switch (fmn_global.facedir) {
          case FMN_DIR_N: fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-10.0f/16.0f,tileid,0); return;
          case FMN_DIR_S: fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-3.0f/16.0f,tileid,0); return;
          case FMN_DIR_W: fmn_gl2_game_add_mintile_vtx(driver,sprite->x-2.0f/16.0f,sprite->y-3.0f/16.0f,tileid,FMN_XFORM_XREV); return;
          case FMN_DIR_E: fmn_gl2_game_add_mintile_vtx(driver,sprite->x+2.0f/16.0f,sprite->y-3.0f/16.0f,tileid,0); return;
        }
      } break;
    
    case FMN_ITEM_BELL: ALT if (ACTIVE) altlayout.tileid=(DRIVER->game.itemtime&16)?0x14:0x24; break;
  }
  #undef ALT
  #undef ACTIVE
  
  xform=0;
  float dstx=sprite->x;
  float dsty=sprite->y+layout->dy/16.0f;
  if ((fmn_global.facedir==FMN_DIR_E)||(fmn_global.facedir==FMN_DIR_N)) {
    dstx-=layout->dx/16.0f;
    xform=FMN_XFORM_XREV;
  } else {
    dstx+=layout->dx/16.0f;
  }
  fmn_gl2_game_add_mintile_vtx(driver,dstx,dsty,layout->tileid,xform);
}

/* Render hero: main entry point.
 */
 
void fmn_gl2_game_render_HERO(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {

  // Skip every other frame if invisible.
  if ((fmn_global.invisibility_time>0.0f)&&(DRIVER->game.framec&1)) return;
  
  // Injured is its own thing.
  if (fmn_global.injury_time>0.0f) {
    fmn_gl2_game_render_HERO_injury(driver,sprite);
    return;
  }
  
  // Pumpkin is its own thing. (if not injured or invisible)
  if (fmn_global.transmogrification==1) {
    fmn_gl2_game_render_HERO_pumpkin(driver,sprite);
    return;
  }
  
  // A few more special states. None of these needs to worry about curse.
  if (fmn_global.spell_repudiation) {
    fmn_gl2_game_render_HERO_spell_repudiation(driver,sprite);
    return;
  }
  switch (fmn_global.active_item) {
    case FMN_ITEM_SNOWGLOBE: fmn_gl2_game_render_HERO_SNOWGLOBE(driver,sprite); return;
    case FMN_ITEM_BROOM: fmn_gl2_game_render_HERO_BROOM(driver,sprite); return;
    case FMN_ITEM_WAND: fmn_gl2_game_render_HERO_WAND(driver,sprite); return;
    case FMN_ITEM_VIOLIN: fmn_gl2_game_render_HERO_VIOLIN(driver,sprite); return;
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
        fmn_gl2_game_render_HERO_item(driver,sprite,tileid,xform);
        fmn_gl2_game_render_HERO_body(driver,sprite,tileid,xform);
        fmn_gl2_game_render_HERO_head(driver,sprite,tileid,xform);
        fmn_gl2_game_render_HERO_hat(driver,sprite,tileid,xform);
      } break;
    case FMN_DIR_S: {
        fmn_gl2_game_render_HERO_body(driver,sprite,tileid,xform);
        fmn_gl2_game_render_HERO_head(driver,sprite,tileid,xform);
        fmn_gl2_game_render_HERO_hat(driver,sprite,tileid,xform);
        fmn_gl2_game_render_HERO_item(driver,sprite,tileid,xform);
      } break;
    case FMN_DIR_W: case FMN_DIR_E: {
        fmn_gl2_game_render_HERO_body(driver,sprite,tileid,xform);
        fmn_gl2_game_render_HERO_item(driver,sprite,tileid,xform);
        fmn_gl2_game_render_HERO_head(driver,sprite,tileid,xform);
        fmn_gl2_game_render_HERO_hat(driver,sprite,tileid,xform);
      } break;
  }
  
  // Cheese and curse highlights.
  if (fmn_global.cheesing) fmn_gl2_game_render_HERO_cheese(driver,sprite);
  if (fmn_global.curse_time>0.0f) fmn_gl2_game_render_HERO_curse(driver,sprite);
}

/* Underlay.
 */
 
static int fmn_gl2_find_hero(struct fmn_sprite *sprite,void *userdata) {
  if (sprite->controller==FMN_SPRCTL_hero) {
    *(void**)userdata=sprite;
    return 1;
  }
  return 0;
}
 
static void fmn_gl2_hero_underlay_broom(struct bigpc_render_driver *driver,int16_t addx,int16_t addy) {
  if (DRIVER->game.framec&1) return;
  struct fmn_sprite *sprite=0;
  fmn_sprites_for_each(fmn_gl2_find_hero,&sprite);
  if (!sprite) return;
  if (fmn_gl2_texture_use_imageid(driver,sprite->imageid)<0) return;
  fmn_gl2_program_use(driver,&DRIVER->program_mintile);
  struct fmn_gl2_vertex_mintile vtx={
    .x=sprite->x*DRIVER->game.tilesize+addx,
    .y=(sprite->y+0.3f)*DRIVER->game.tilesize+addy,
    .tileid=(DRIVER->game.framec&32)?0x67:0x77,
    .xform=0,
  };
  fmn_gl2_draw_mintile(&vtx,1);
}

// also used for seeds and pitcher
static void fmn_gl2_hero_underlay_shovel(struct bigpc_render_driver *driver,int16_t addx,int16_t addy) {
  if (addx||addy) return; // don't draw this while transitioning
  if ((fmn_global.shovelx<0)||(fmn_global.shovelx>=FMN_COLC)) return;
  if ((fmn_global.shovely<0)||(fmn_global.shovely>=FMN_ROWC)) return;
  if (fmn_gl2_texture_use_imageid(driver,2)<0) return;
  fmn_gl2_program_use(driver,&DRIVER->program_mintile);
  struct fmn_gl2_vertex_mintile vtx={
    .x=fmn_global.shovelx*DRIVER->game.tilesize+(DRIVER->game.tilesize>>1),
    .y=fmn_global.shovely*DRIVER->game.tilesize+(DRIVER->game.tilesize>>1),
    .tileid=(DRIVER->game.framec&0x10)?0x17:0x27,
    .xform=0,
  };
  fmn_gl2_draw_mintile(&vtx,1);
}
 
void fmn_gl2_render_hero_underlay(struct bigpc_render_driver *driver,int16_t addx,int16_t addy) {
  if (fmn_global.hero_dead) return;
  switch (fmn_global.active_item) {
    case FMN_ITEM_BROOM: fmn_gl2_hero_underlay_broom(driver,addx,addy); return;
  }
  if (fmn_global.itemv[fmn_global.selected_item]) switch (fmn_global.selected_item) {
    case FMN_ITEM_SHOVEL: fmn_gl2_hero_underlay_shovel(driver,addx,addy); return;
    case FMN_ITEM_SEED: if (fmn_global.itemqv[FMN_ITEM_SEED]) fmn_gl2_hero_underlay_shovel(driver,addx,addy); return;
    case FMN_ITEM_PITCHER: if (fmn_global.itemqv[FMN_ITEM_PITCHER]) fmn_gl2_hero_underlay_shovel(driver,addx,addy); return;
  }
}

/* Overlay.
 */
 
static void fmn_gl2_hero_overlay_compass(struct bigpc_render_driver *driver) {
  
  struct fmn_sprite *sprite=0;
  fmn_sprites_for_each(fmn_gl2_find_hero,&sprite);
  if (!sprite) return;
  if (fmn_gl2_texture_use_imageid(driver,sprite->imageid)<0) return;
  fmn_gl2_program_use(driver,&DRIVER->program_maxtile);
  
  if (fmn_global.curse_time>0.0f) {
    DRIVER->game.compassangle-=FMN_GL2_COMPASS_RATE_MIN;
  } else if (!fmn_global.compassx&&!fmn_global.compassy) {
    DRIVER->game.compassangle+=FMN_GL2_COMPASS_RATE_MAX;
  } else {
    float targetx=fmn_global.compassx+0.5f;
    float targety=fmn_global.compassy+0.5f;
    float tdx=targetx-sprite->x;
    float tdy=targety-sprite->y;
    float distance=sqrtf(tdx*tdx+tdy*tdy);
    if (distance>=FMN_GL2_COMPASS_DISTANCE_MAX) {
      DRIVER->game.compassangle+=FMN_GL2_COMPASS_RATE_MAX;
    } else {
      float angle=atan2f(tdx,-tdy);
      float ndist=distance/FMN_GL2_COMPASS_DISTANCE_MAX;
      float minrate=FMN_GL2_COMPASS_RATE_MIN+(FMN_GL2_COMPASS_RATE_MAX-FMN_GL2_COMPASS_RATE_MIN)*ndist;
      float diff=DRIVER->game.compassangle-angle;
      if (diff>M_PI) diff-=M_PI*2.0f;
      else if (diff<-M_PI) diff+=M_PI*2.0f;
      if (diff<0.0f) diff=-diff;
      DRIVER->game.compassangle+=minrate+(diff*(FMN_GL2_COMPASS_RATE_MAX-minrate))/M_PI;
    }
  }
  if (DRIVER->game.compassangle>M_PI) DRIVER->game.compassangle-=M_PI*2.0f;
  else if (DRIVER->game.compassangle<-M_PI) DRIVER->game.compassangle+=M_PI*2.0f;
  
  float dstx=sprite->x+sin(DRIVER->game.compassangle);
  float dsty=sprite->y-cos(DRIVER->game.compassangle);
  struct fmn_gl2_vertex_maxtile vtx={
    .x=dstx*DRIVER->game.tilesize,
    .y=dsty*DRIVER->game.tilesize,
    .tileid=0x63,
    .rotate=(DRIVER->game.compassangle*128.0f)/M_PI,
    .size=DRIVER->game.tilesize,
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
  fmn_gl2_draw_maxtile(&vtx,1);
}

static void fmn_gl2_hero_overlay_showoff(struct bigpc_render_driver *driver) {
  if (!(DRIVER->game.framec&3)) return;
  struct fmn_sprite *sprite=0;
  fmn_sprites_for_each(fmn_gl2_find_hero,&sprite);
  if (!sprite) return;
  if (fmn_gl2_texture_use_imageid(driver,sprite->imageid)<0) return;
  fmn_gl2_program_use(driver,&DRIVER->program_mintile);
  struct fmn_gl2_vertex_mintile vtx={
    .x=sprite->x*DRIVER->game.tilesize,
    .y=(sprite->y-1.5f)*DRIVER->game.tilesize,
    .tileid=0xf0+fmn_global.show_off_item,
    .xform=0,
  };
  if (((fmn_global.show_off_item&0x0f)==FMN_ITEM_PITCHER)&&(fmn_global.show_off_item&0xf0)) {
    vtx.tileid=0xe2+(fmn_global.show_off_item>>4);
  }
  fmn_gl2_draw_mintile(&vtx,1);
}
 
void fmn_gl2_render_hero_overlay(struct bigpc_render_driver *driver,int16_t addx,int16_t addy) {
  if (addx||addy) return;
  if (fmn_global.hero_dead) return;
  if (fmn_global.itemv[fmn_global.selected_item]) switch (fmn_global.selected_item) {
    case FMN_ITEM_COMPASS: fmn_gl2_hero_overlay_compass(driver); break;
  }
  if (fmn_global.show_off_item_time) {
    fmn_gl2_hero_overlay_showoff(driver);
  }
}

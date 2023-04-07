#include "../fmn_gl2_internal.h"

/* Cleanup.
 */
 
void fmn_gl2_game_cleanup(struct bigpc_render_driver *driver) {
  fmn_gl2_framebuffer_cleanup(&DRIVER->game.mapbits);
  if (DRIVER->game.mintile_vtxv) free(DRIVER->game.mintile_vtxv);
}

/* Init.
 */
 
int fmn_gl2_game_init(struct bigpc_render_driver *driver) {
  DRIVER->game.map_dirty=1;
  DRIVER->game.tilesize=16;//TODO consult config for image qualifier
  
  int err=fmn_gl2_framebuffer_init(&DRIVER->game.mapbits,DRIVER->game.tilesize*FMN_COLC,DRIVER->game.tilesize*FMN_ROWC);
  if (err<0) return err;
  
  return 0;
}

/* Draw map.
 */
 
static void fmn_gl2_game_freshen_map(struct bigpc_render_driver *driver) {
  struct fmn_gl2_vertex_mintile vtxv[FMN_COLC*FMN_ROWC*2];
  struct fmn_gl2_vertex_mintile *vtx=vtxv;
  int vtxc=0;
  const uint8_t *src=fmn_global.map;
  int y=DRIVER->game.tilesize>>1;
  int yi=FMN_ROWC;
  for (;yi-->0;y+=DRIVER->game.tilesize) {
    int x=DRIVER->game.tilesize>>1;
    int xi=FMN_COLC;
    for (;xi-->0;x+=DRIVER->game.tilesize,src++) {
      vtx->x=x;
      vtx->y=y;
      vtx->tileid=0x00;
      vtx->xform=0;
      vtx++;
      vtxc++;
      if (*src) {
        vtx->x=x;
        vtx->y=y;
        vtx->tileid=*src;
        vtx->xform=0;
        vtx++;
        vtxc++;
      }
    }
  }
  fmn_gl2_framebuffer_use(driver,&DRIVER->game.mapbits);
  fmn_gl2_texture_use(driver,fmn_global.maptsid);
  fmn_gl2_program_use(driver,&DRIVER->program_mintile);
  fmn_gl2_draw_mintile(vtxv,vtxc);
}

/* Add a mintile vertex to the cache.
 */
 
static int fmn_gl2_game_add_mintile_vtx_pixcoord(
  struct bigpc_render_driver *driver,
  int16_t x,int16_t y,uint8_t tileid,uint8_t xform
) {
  if (DRIVER->game.mintile_vtxc>=DRIVER->game.mintile_vtxa) {
    int na=DRIVER->game.mintile_vtxa+64;
    if (na>INT_MAX/sizeof(struct fmn_gl2_vertex_mintile)) return -1;
    void *nv=realloc(DRIVER->game.mintile_vtxv,sizeof(struct fmn_gl2_vertex_mintile)*na);
    if (!nv) return -1;
    DRIVER->game.mintile_vtxv=nv;
    DRIVER->game.mintile_vtxa=na;
  }
  struct fmn_gl2_vertex_mintile *vtx=DRIVER->game.mintile_vtxv+DRIVER->game.mintile_vtxc++;
  vtx->x=x;
  vtx->y=y;
  vtx->tileid=tileid;
  vtx->xform=xform;
  return 0;
}
 
static int fmn_gl2_game_add_mintile_vtx(
  struct bigpc_render_driver *driver,
  float sx,float sy,uint8_t tileid,uint8_t xform
) {
  return fmn_gl2_game_add_mintile_vtx_pixcoord(driver,sx*DRIVER->game.tilesize,sy*DRIVER->game.tilesize,tileid,xform);
}

/* Render HERO sprite.
 */
 
static void fmn_gl2_game_render_HERO_curse(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-1.0f,0x73,(DRIVER->game.framec&0x10)?FMN_XFORM_XREV:0);
}

static void fmn_gl2_game_render_HERO_cheese(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  //TODO RenderHero.js does this with ellipses. Use tiles instead. (lets us keep in the merged-mintile batches, and the web look of this is not so hot anyway).
}
 
static void fmn_gl2_game_render_HERO_injury(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {
  uint8_t tileid_body=0,tileid_head=0,tileid_hat=0;
  float hatdy=0.0f;
  if (fmn_global.transmogrification==1) { // pumpkin
    if (DRIVER->game.framec&4) {
      tileid_hat=0x33;
      tileid_body=0x2e;
    } else {
      tileid_hat=0x03;
      tileid_body=0x2d;
    }
    hatdy=-0.375f;
  } else { // normal
    tileid_hat=(DRIVER->game.framec&4)?0x03:0x33;
    tileid_head=tileid_hat+0x10;
    tileid_body=tileid_hat+0x20;
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
  fmn_gl2_game_add_mintile_vtx(driver,sprite->x,sprite->y-0.75f,tileid,xform);
}

static const struct fmn_gl2_hero_item_layout {
  uint8_t tileid;
  int8_t dx,dy; // pixels, based on 16-pixel tiles
  uint8_t if_qualifier;
} fmn_gl2_hero_item_layoutv[FMN_ITEM_COUNT]={
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
 
static void fmn_gl2_game_render_HERO(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite) {

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

/* Sprites.
 * Caller can and should request more than one sprite at a time,
 * but they must be on the same image and only use mintile shader.
 * A sprite using its own image, or a style that might not use mintile, must come here alone.
 */
 
static void fmn_gl2_game_render_sprites(
  struct bigpc_render_driver *driver,
  struct fmn_sprite_header **spritev,
  int spritec
) {
  if (fmn_gl2_texture_use(driver,(*spritev)->imageid)<0) return;
  DRIVER->game.mintile_vtxc=0;
  for (;spritec-->0;spritev++) {
    struct fmn_sprite_header *s=*spritev;
    switch (s->style) {
      case FMN_SPRITE_STYLE_TILE: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;
      case FMN_SPRITE_STYLE_HERO: fmn_gl2_game_render_HERO(driver,s); break;
      case FMN_SPRITE_STYLE_FOURFRAME: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;//TODO tileid
      case FMN_SPRITE_STYLE_FIRENOZZLE: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;//TODO
      case FMN_SPRITE_STYLE_FIREWALL: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;//TODO
      case FMN_SPRITE_STYLE_DOUBLEWIDE: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;//TODO
      case FMN_SPRITE_STYLE_PITCHFORK: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;//TODO
      case FMN_SPRITE_STYLE_TWOFRAME: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;//TODO tileid
      case FMN_SPRITE_STYLE_EIGHTFRAME: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;//TODO tileid
      case FMN_SPRITE_STYLE_SCARYDOOR: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;//TODO
      case FMN_SPRITE_STYLE_WEREWOLF: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;//TODO
      case FMN_SPRITE_STYLE_FLOORFIRE: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;//TODO
      case FMN_SPRITE_STYLE_DEADWITCH: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;//TODO
    }
  }
  if (DRIVER->game.mintile_vtxc) {
    fmn_gl2_program_use(driver,&DRIVER->program_mintile);
    fmn_gl2_draw_mintile(DRIVER->game.mintile_vtxv,DRIVER->game.mintile_vtxc);
  }
}

static inline int fmn_gl2_sprite_style_uses_mintile(int style) {
  switch (style) {
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
      return 1;
  }
  // No: SCARYDOOR
  return 0;
}

/* Render.
 */
 
void fmn_gl2_game_render(struct bigpc_render_driver *driver) {
  DRIVER->game.framec++;
  
  if (DRIVER->game.map_dirty) {
    DRIVER->game.map_dirty=0;
    fmn_gl2_game_freshen_map(driver);
  }

  fmn_gl2_framebuffer_use(driver,&DRIVER->mainfb);
  
  // Map.
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  fmn_gl2_texture_use_object(driver,&DRIVER->game.mapbits.texture);
  fmn_gl2_draw_decal(
    0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,
    0,DRIVER->game.mapbits.texture.h,DRIVER->game.mapbits.texture.w,-DRIVER->game.mapbits.texture.h
  );
  
  //TODO item underlay
  
  { // Sprites.
    struct fmn_sprite_header **p=fmn_global.spritev;
    int i=0;
    while (i<fmn_global.spritec) {
      int c=1;
      if (fmn_gl2_sprite_style_uses_mintile((*p)->style)) {
        while (i+c<fmn_global.spritec) {
          struct fmn_sprite_header *q=p[c];
          if ((*p)->imageid!=q->imageid) break;
          if (!fmn_gl2_sprite_style_uses_mintile(q->style)) break;
          c++;
        }
      }
      fmn_gl2_game_render_sprites(driver,p,c);
      p+=c;
      i+=c;
    }
  }
  
  //TODO item overlay
  //TODO weather
  //TODO transitions
  //TODO violin
  //TODO menus
}

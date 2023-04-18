#include "../fmn_gl2_internal.h"
#include "opt/fontosaur/fontosaur.h"

void fmn_gl2_game_freshen_map(struct bigpc_render_driver *driver);
void fmn_gl2_game_render_HERO(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite);
void fmn_gl2_render_hero_underlay(struct bigpc_render_driver *driver,int16_t addx,int16_t addy);
void fmn_gl2_render_hero_overlay(struct bigpc_render_driver *driver,int16_t addx,int16_t addy);
void fmn_gl2_game_render_FIRENOZZLE(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite);
void fmn_gl2_game_render_FIREWALL(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite);
void fmn_gl2_game_render_DOUBLEWIDE(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite);
void fmn_gl2_game_render_PITCHFORK(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite);
void fmn_gl2_game_render_SCARYDOOR(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite);
void fmn_gl2_game_render_WEREWOLF(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite);
void fmn_gl2_game_render_FLOORFIRE(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite);
void fmn_gl2_game_render_DEADWITCH(struct bigpc_render_driver *driver,struct fmn_sprite_header *sprite);
void fmn_gl2_game_render_weather(struct bigpc_render_driver *driver);
void fmn_gl2_render_mapdark(struct bigpc_render_driver *driver);
void fmn_gl2_render_violin(struct bigpc_render_driver *driver);
void fmn_gl2_render_menu(struct bigpc_render_driver *driver,struct bigpc_menu *menu);

/* Cleanup.
 */
 
void fmn_gl2_game_cleanup(struct bigpc_render_driver *driver) {
  fmn_gl2_framebuffer_cleanup(&DRIVER->game.mapbits);
  if (DRIVER->game.mintile_vtxv) free(DRIVER->game.mintile_vtxv);
  fmn_gl2_framebuffer_cleanup(&DRIVER->game.transitionfrom);
  fmn_gl2_framebuffer_cleanup(&DRIVER->game.transitionto);
  fmn_gl2_texture_cleanup(&DRIVER->game.idle_warning_texture);
}

/* Init.
 */
 
int fmn_gl2_game_init(struct bigpc_render_driver *driver) {
  DRIVER->game.map_dirty=1;
  DRIVER->game.tilesize=16;//TODO consult config for image qualifier
  
  int err=fmn_gl2_framebuffer_init(&DRIVER->game.mapbits,DRIVER->game.tilesize*FMN_COLC,DRIVER->game.tilesize*FMN_ROWC);
  if (err<0) return err;
  
  if ((err=fmn_gl2_framebuffer_init(
    &DRIVER->game.transitionfrom,DRIVER->game.mapbits.texture.w,DRIVER->game.mapbits.texture.h
  ))<0) return err;
  if ((err=fmn_gl2_framebuffer_init(
    &DRIVER->game.transitionto,DRIVER->game.mapbits.texture.w,DRIVER->game.mapbits.texture.h
  ))<0) return err;
  
  return 0;
}

/* Add a mintile vertex to the cache.
 */
 
int fmn_gl2_game_add_mintile_vtx_pixcoord(
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

/* Special case for the hero sprite, when we draw her above the transition in progress.
 */
 
static void fmn_gl2_find_and_render_hero_over_transition(struct bigpc_render_driver *driver) {
  struct fmn_sprite_header **spritev=fmn_global.spritev;
  int spritec=fmn_global.spritec;
  DRIVER->game.mintile_vtxc=0;
  for (;spritec-->0;spritev++) {
    struct fmn_sprite_header *s=*spritev;
    if (s->style==FMN_SPRITE_STYLE_HERO) {
      int16_t dx=0,dy=0;
      fmn_gl2_transition_get_hero_offset(&dx,&dy,driver);
      fmn_gl2_render_hero_underlay(driver,dx,dy);
      fmn_gl2_game_render_HERO(driver,s);
      fmn_gl2_render_hero_overlay(driver,dx,dy);
      if (DRIVER->game.mintile_vtxc) {
        if (fmn_gl2_texture_use(driver,s->imageid)<0) return;
        struct fmn_gl2_vertex_mintile *vtx=DRIVER->game.mintile_vtxv;
        int i=DRIVER->game.mintile_vtxc;
        for (;i-->0;vtx++) {
          vtx->x+=dx;
          vtx->y+=dy;
        }
        fmn_gl2_program_use(driver,&DRIVER->program_mintile);
        fmn_gl2_draw_mintile(DRIVER->game.mintile_vtxv,DRIVER->game.mintile_vtxc);
      }
      return;
    }
  }
}

/* Sprites.
 * Caller can and should request more than one sprite at a time,
 * but they must be on the same image and only use mintile shader.
 * A sprite using its own image, or a style that might not use mintile, must come here alone.
 */
 
static void fmn_gl2_game_render_sprites(
  struct bigpc_render_driver *driver,
  struct fmn_sprite_header **spritev,
  int spritec,
  int include_hero
) {
  if (fmn_gl2_texture_use(driver,(*spritev)->imageid)<0) return;
  DRIVER->game.mintile_vtxc=0;
  for (;spritec-->0;spritev++) {
    struct fmn_sprite_header *s=*spritev;
    switch (s->style) {
      case FMN_SPRITE_STYLE_TILE: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid,s->xform); break;
      case FMN_SPRITE_STYLE_HERO: if (!include_hero) continue; fmn_gl2_game_render_HERO(driver,s); break;
      case FMN_SPRITE_STYLE_FOURFRAME: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid+((DRIVER->game.framec>>3)&3),s->xform); break;
      case FMN_SPRITE_STYLE_FIRENOZZLE: fmn_gl2_game_render_FIRENOZZLE(driver,s); break;
      case FMN_SPRITE_STYLE_FIREWALL: fmn_gl2_game_render_FIREWALL(driver,s); break;
      case FMN_SPRITE_STYLE_DOUBLEWIDE: fmn_gl2_game_render_DOUBLEWIDE(driver,s); break;
      case FMN_SPRITE_STYLE_PITCHFORK: fmn_gl2_game_render_PITCHFORK(driver,s); break;
      case FMN_SPRITE_STYLE_TWOFRAME: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid+((DRIVER->game.framec>>3)&1),s->xform); break;
      case FMN_SPRITE_STYLE_EIGHTFRAME: fmn_gl2_game_add_mintile_vtx(driver,s->x,s->y,s->tileid+((DRIVER->game.framec>>1)&7),s->xform); break;
      case FMN_SPRITE_STYLE_SCARYDOOR: fmn_gl2_game_render_SCARYDOOR(driver,s); break;
      case FMN_SPRITE_STYLE_WEREWOLF: fmn_gl2_game_render_WEREWOLF(driver,s); break;
      case FMN_SPRITE_STYLE_FLOORFIRE: fmn_gl2_game_render_FLOORFIRE(driver,s); break;
      case FMN_SPRITE_STYLE_DEADWITCH: fmn_gl2_game_render_DEADWITCH(driver,s); break;
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

/* Render to the currently-bound framebuffer, all the diegetic stuff subject to transitions.
 * Normally (include_hero) is true, you want to draw her.
 * That's false during transitions, when we might very carefully draw her above the combined buffers
 */
 
void fmn_gl2_game_render_world(struct bigpc_render_driver *driver,int include_hero) {
  
  // Map.
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  fmn_gl2_texture_use_object(driver,&DRIVER->game.mapbits.texture);
  fmn_gl2_draw_decal(
    0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,
    0,DRIVER->game.mapbits.texture.h,DRIVER->game.mapbits.texture.w,-DRIVER->game.mapbits.texture.h
  );
  
  // Hero underlay: Shovel focus and broom shadow.
  if (!driver->transition_in_progress&&include_hero) {
    fmn_gl2_render_hero_underlay(driver,0,0);
  }
  
  // Darkness is weather, but it's below the sprites.
  fmn_gl2_render_mapdark(driver);
  
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
      fmn_gl2_game_render_sprites(driver,p,c,include_hero);
      p+=c;
      i+=c;
    }
  }
  
  // Hero overlay: Compass and show-off-item.
  if (!driver->transition_in_progress&&include_hero) {
    fmn_gl2_render_hero_overlay(driver,0,0);
  }
  
  // Weather.
  fmn_gl2_game_render_weather(driver);
  
  // Game-over fade out.
  if (fmn_global.werewolf_dead||fmn_global.hero_dead) {
    if (fmn_global.terminate_time>=1.0f) {
      // display as usual
    } else {
      // fade out
      int alpha=fmn_global.terminate_time*255.0f;
      if (alpha>0xff) alpha=0;
      else if (alpha<0) alpha=0xff;
      else alpha=0xff-alpha;
      uint32_t rgba=alpha;
      fmn_gl2_program_use(driver,&DRIVER->program_raw);
      fmn_gl2_draw_raw_rect(0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,rgba);
    }
  }
}

/* Idle warning.
 */
 
static void fmn_gl2_render_idle_warning(struct bigpc_render_driver *driver,int s) {

  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw_rect(0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,0xff000080);

  if (fmn_gl2_texture_use(driver,14)>=0) {
    fmn_gl2_program_use(driver,&DRIVER->program_decal);
    int16_t srcx=0,srcy=DRIVER->game.tilesize*12;
    int16_t w=DRIVER->game.tilesize*7,h=DRIVER->game.tilesize*4;
    int16_t dstx=(DRIVER->mainfb.texture.w>>1)-(w>>1);
    int16_t dsty=(DRIVER->mainfb.texture.h>>1)-(h>>1);
    fmn_gl2_draw_decal(dstx,dsty,w,h,srcx,srcy,w,h);
    
    if (DRIVER->game.idle_warning_time!=s) {
      DRIVER->game.idle_warning_time=s;
      char text[2];
      int textc;
      if (s>=10) {
        text[0]='0'+(s%100)/10;
        text[1]='0'+s%10;
        textc=2;
      } else {
        text[0]='0'+s;
        textc=1;
      }
      struct fontosaur_image image={0};
      if (fontosaur_render_text(&image,driver->datafile,16,32,0,0xffffffff,0,text,textc)>=0) {
        fmn_gl2_texture_cleanup(&DRIVER->game.idle_warning_texture);
        fmn_gl2_texture_init_rgba(&DRIVER->game.idle_warning_texture,image.w,image.h,image.v);
      }
      fontosaur_image_cleanup(&image);
    }
    
    // Countdown is centered at 2/3,3/4
    fmn_gl2_texture_use_object(driver,&DRIVER->game.idle_warning_texture);
    int16_t tw=DRIVER->game.idle_warning_texture.w;
    int16_t th=DRIVER->game.idle_warning_texture.h;
    int16_t tdstx=dstx+(w*2)/3-(tw>>1);
    int16_t tdsty=dsty+((h*3)>>2)-(th>>1);
    fmn_gl2_program_use(driver,&DRIVER->program_recal);
    fmn_gl2_draw_recal(&DRIVER->program_recal,tdstx,tdsty,tw,th,0,0,tw,th,(DRIVER->game.framec&8)?0xff0000ff:0x200000ff);
  }
}

/* Render.
 */
 
void fmn_gl2_game_render(struct bigpc_render_driver *driver) {
  DRIVER->game.framec++;
  
  // Redraw map if needed.
  if (DRIVER->game.map_dirty) {
    DRIVER->game.map_dirty=0;
    fmn_gl2_game_freshen_map(driver);
  }
  
  // Advance transition clock.
  if (DRIVER->game.transition) {
    DRIVER->game.transitionp++;
    if (DRIVER->game.transitionp>=DRIVER->game.transitionc) {
      DRIVER->game.transition=0;
      DRIVER->game.transitionc=0;
      driver->transition_in_progress=0;
    }
  }
  
  // Transition in progress? Draw world to (transitionto), then combine (transitionfrom,transitionto) into the main.
  // No transition? Draw direct into main.
  if (DRIVER->game.transition) {
    fmn_gl2_framebuffer_use(driver,&DRIVER->game.transitionto);
    fmn_gl2_game_render_world(driver,DRIVER->game.hero_above_transition?0:1);
    fmn_gl2_framebuffer_use(driver,&DRIVER->mainfb);
    fmn_gl2_game_transition_apply(
      driver,
      DRIVER->game.transition,DRIVER->game.transitionp,DRIVER->game.transitionc,
      &DRIVER->game.transitionfrom,&DRIVER->game.transitionto
    );
    if (DRIVER->game.hero_above_transition) {
      fmn_gl2_find_and_render_hero_over_transition(driver);
    }
  } else {
    fmn_gl2_framebuffer_use(driver,&DRIVER->mainfb);
    fmn_gl2_game_render_world(driver,1);
  }
  
  // The violin chart is its own thing, above transitions but below menus.
  if (fmn_global.active_item==FMN_ITEM_VIOLIN) {
    fmn_gl2_render_violin(driver);
  }
  
  // If there's a menu it goes on top.
  struct bigpc_menu *menu=bigpc_get_menu();
  if (menu) fmn_gl2_render_menu(driver,menu);
  
  // On even topper, the idle warning if applicable.
  int idle_time=bigpc_get_idle_warning_time_s();
  if (idle_time) {
    fmn_gl2_render_idle_warning(driver,idle_time);
  }
}

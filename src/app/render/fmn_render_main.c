#include "fmn_render_internal.h"
#include "app/fmn_game.h"

struct fmn_render_global fmn_render_global={0};

/* Special case for the hero, during pan transitions we like 
 * to omit her from the world and then draw on top of the combined output.
 */
 
static void fmn_find_and_render_hero_over_transition() {
  int16_t addx,addy;
  fmn_transition_get_hero_offset(&addx,&addy);
  fmn_render_hero_underlay(addx,addy);
  struct fmn_sprite **p=(struct fmn_sprite**)fmn_global.spritev;
  int i=fmn_global.spritec;
  for (;i-->0;p++) {
    struct fmn_sprite *sprite=*p;
    if (sprite->style!=FMN_SPRITE_STYLE_HERO) continue;
    fmn_render_hero(sprite,addx,addy);
  }
  fmn_render_hero_overlay(addx,addy);
}

/* Render world.
 * Map bits are done already, and we don't need to worry about transitions or overlays.
 * Caller establishes the output image beforehand.
 */
 
void fmn_render_world(uint8_t include_hero) {
  
  // Background.
  {
    struct fmn_draw_decal vtx={
      .dstx=0,.dsty=0,.dstw=fmn_render_global.fbw,.dsth=fmn_render_global.fbh,
      .srcx=0,.srcy=0,.srcw=fmn_render_global.fbw,.srch=fmn_render_global.fbh,
    };
    fmn_draw_decal(&vtx,1,FMN_IMAGEID_MAPBITS);
  }
  
  // Hero underlay: shovel target indicator.
  if (include_hero) {
    fmn_render_hero_underlay(0,0);
  }
  
  // Darkness. It's not with the rest of weather, because it's under the sprites.
  fmn_render_darkness();
  
  // Sprites.
  fmn_render_sprites(include_hero);
  
  // Hero overlay: Compass and show-off-item.
  if (include_hero) {
    fmn_render_hero_overlay(0,0);
  }
  
  // Weather.
  fmn_render_weather();
  
  // Game-over fade out.
  fmn_render_fade_out();
}

/* Render, main entry point.
 */
 
uint8_t fmn_render() {
  fmn_render_global.framec++;
  if (!fmn_render_global.fbw) {
    fmn_video_get_framebuffer_size(&fmn_render_global.fbw,&fmn_render_global.fbh);
    fmn_render_global.tilesize=fmn_render_global.fbw/FMN_COLC;
  }
  
  // It's really dumb, but under soft render, the pixel format is not known until the first render starts.
  if (!fmn_render_global.chalk_color) {
    fmn_render_global.chalk_color=fmn_video_pixel_from_rgba(0xffffffff);
    fmn_render_global.violin_line_color=fmn_video_pixel_from_rgba(0x886644ff);
    fmn_render_global.violin_highlight_line_color=fmn_video_pixel_from_rgba(0xcc0000ff);
    fmn_render_global.rain_color=fmn_video_pixel_from_rgba(0x00008880);
    fmn_render_global.wind_color=fmn_video_pixel_from_rgba(0xccccccc0);
    fmn_render_global.snow_color=fmn_video_pixel_from_rgba(0xe0e8f0ff);
  }
  
  // Get the top menu, and if it is opaque, we can skip everything else.
  struct fmn_menu *menu=fmn_get_top_menu();
  if (!menu||!menu->opaque) {
  
    // Redraw map if needed.
    if (fmn_render_global.map_dirty) {
      fmn_render_global.map_dirty=0;
      fmn_render_freshen_map();
    }
  
    // Advance transition clock.
    if (fmn_render_global.transition) {
      fmn_render_global.transitionp++;
      if (fmn_render_global.transitionp>=fmn_render_global.transitionc) {
        fmn_render_global.transition=0;
        fmn_render_global.transitionc=0;
      }
    }
  
    // Transition in progress? Draw world to (transition_to), then combine (transition_from,transition_to) into the main.
    // No transition? Draw direct into main.
    if (fmn_render_global.transition) {
      fmn_draw_set_output(FMN_IMAGEID_TRANSITION_TO);
      fmn_render_world(fmn_render_global.hero_above_transition?0:1);
      fmn_draw_set_output(0);
      fmn_transition_apply(
        fmn_render_global.transition,
        fmn_render_global.transitionp,fmn_render_global.transitionc,
        FMN_IMAGEID_TRANSITION_FROM,
        FMN_IMAGEID_TRANSITION_TO
      );
      if (fmn_render_global.hero_above_transition) {
        fmn_find_and_render_hero_over_transition();
      }
    } else {
      fmn_draw_set_output(0);
      fmn_render_world(1);
    }
  
    // The violin chart is its own thing, above transitions but below menus.
    // (FWIW It's unlikely for transitions, violin, or menus to ever exist at the same time).
    if (fmn_global.active_item==FMN_ITEM_VIOLIN) {
      fmn_render_violin();
    }
    
    // Menu blotter.
    if (menu) {
      struct fmn_draw_rect vtx={0,0,fmn_render_global.fbw,fmn_render_global.fbh,fmn_video_pixel_from_rgba(0x000000c0)};
      fmn_draw_rect(&vtx,1);
    }
  } else {
    fmn_draw_set_output(0);
  }
  
  // Menu content.
  if (menu) {
    if (menu->render) menu->render(menu);
  }
  
  // On even topper, the idle warning if applicable.
  int idle_time=fmn_get_idle_warning_time_s();
  if (idle_time>=0) {
    fmn_render_idle_warning(idle_time);
  }
  
  return 1;
}

/* Trivial accessors.
 */
 
void fmn_map_dirty() {
  fmn_render_global.map_dirty=1;
}

uint8_t fmn_render_transition_in_progress() {
  return fmn_render_global.transition;
}

/* Init.
 */
 
void fmn_render_init() {
}

#include "fmn_hero_internal.h"
#include "app/sprite/fmn_physics.h"
#include "app/fmn_game.h"

/* Trigger (interact) for all sprites in our focus zone, returning the first nonzero result.
 */
 
struct fmn_hero_interact_locally_context {
  uint8_t itemid;
  uint8_t qualifier;
  float x;
  float y;
};

static int fmn_hero_interact_locally_1(struct fmn_sprite *sprite,void *userdata) {
  struct fmn_hero_interact_locally_context *ctx=userdata;
  if (!sprite->interact) return 0;
  if (sprite->x-sprite->radius>ctx->x) return 0;
  if (sprite->x+sprite->radius<ctx->x) return 0;
  if (sprite->y-sprite->radius>ctx->y) return 0;
  if (sprite->y+sprite->radius<ctx->y) return 0;
  return sprite->interact(sprite,ctx->itemid,ctx->qualifier);
}
 
static int16_t fmn_hero_interact_locally(uint8_t itemid,uint8_t qualifier) {
  struct fmn_hero_interact_locally_context ctx={
    .itemid=itemid,
    .qualifier=qualifier,
    .x=fmn_hero.sprite->x,
    .y=fmn_hero.sprite->y,
  };
  if ((itemid==FMN_ITEM_SEED)||(itemid==FMN_ITEM_PITCHER)||(itemid==FMN_ITEM_SHOVEL)) {
    // Using seed, pitcher, or shovel, there would be a visible guide at fmn_global.shovel[xy].
    // That is likely to agree with our +0.5 method, but discrepancies have been observed.
    // The visible guide trumps all else. Look for sprites at the center of the highlighted cell.
    ctx.x=fmn_global.shovelx+0.5f;
    ctx.y=fmn_global.shovely+0.5f;
  } else {
    // All else, the general case: 0.5 cells in the direction we're facing.
    float dx,dy;
    fmn_vector_from_dir(&dx,&dy,fmn_global.facedir);
    ctx.x+=dx*0.5f;
    ctx.y+=dy*0.5f;
  }
  return fmn_sprites_for_each(fmn_hero_interact_locally_1,&ctx);
}

/* Bell.
 */
 
static int fmn_bell_1(struct fmn_sprite *sprite,void *userdata) {
  if (sprite->interact) {
    sprite->interact(sprite,FMN_ITEM_BELL,0);
  }
  return 0;
}
 
static void fmn_hero_bell_begin() {
  fmn_sound_effect(FMN_SFX_BELL);
  fmn_hero.bell_count=1;
  //TODO non-sprite global hooks for bell?
  fmn_sprites_for_each(fmn_bell_1,0);
}

static void fmn_hero_bell_update(float elapsed) {
}

/* Seed.
 */
 
static void fmn_hero_seed_begin() {
  fmn_global.active_item=0xff;
  if (!fmn_global.itemqv[FMN_ITEM_SEED]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_global.itemqv[FMN_ITEM_SEED]--;
  
  // If somebody eats the corn immediately, we're done.
  int16_t result=fmn_hero_interact_locally(FMN_ITEM_SEED,0);
  if (result) return;
  
  // Check the focus tile, can we plant there?
  // If it's solid, hole, or oob, put it back in your pocket and reject.
  if ((fmn_global.shovelx<0)||(fmn_global.shovely<0)||(fmn_global.shovelx>=FMN_COLC)||(fmn_global.shovely>=FMN_ROWC)) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    fmn_global.itemqv[FMN_ITEM_SEED]++;
    return;
  }
  uint8_t tilep=fmn_global.shovely*FMN_COLC+fmn_global.shovelx;
  uint8_t tileid=fmn_global.map[tilep];
  if (tileid==0x0f) { // plantable dirt
    int8_t result=fmn_add_plant(fmn_global.shovelx,fmn_global.shovely);
    if (result>=0) { // planted!
      fmn_log_event("plant","%d,%d",fmn_global.shovelx,fmn_global.shovely);
      fmn_sound_effect(FMN_SFX_PLANT);
      fmn_map_dirty();
      return;
    }
    // If creating the plant rejected, we might still be ok to create the sprite.
  }
  uint8_t physics=fmn_global.cellphysics[tileid];
  switch (physics) {
    case FMN_CELLPHYSICS_SOLID:
    case FMN_CELLPHYSICS_HOLE:
    case FMN_CELLPHYSICS_UNCHALKABLE:
    case FMN_CELLPHYSICS_SAP:
    case FMN_CELLPHYSICS_SAP_NOCHALK:
    case FMN_CELLPHYSICS_WATER:
    case FMN_CELLPHYSICS_REVELABLE: {
        fmn_sound_effect(FMN_SFX_REJECT_ITEM);
        fmn_global.itemqv[FMN_ITEM_SEED]++;
        return;
      }
  }
  
  // Create a seed sprite. Hero can pick it back up, or wait for it to attract a bird. Or whatever.
  float x,y;
  fmn_vector_from_dir(&x,&y,fmn_global.facedir);
  x+=fmn_hero.sprite->x;
  y+=fmn_hero.sprite->y;
  const uint8_t cmdv[]={0x42,FMN_SPRCTL_seed>>8,FMN_SPRCTL_seed};
  struct fmn_sprite *sprite=fmn_sprite_spawn(x,y,0,cmdv,sizeof(cmdv),0,0);
  if (!sprite) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    fmn_global.itemqv[FMN_ITEM_SEED]++;
    return;
  }
  fmn_sound_effect(FMN_SFX_SEED_DROP);
  fmn_log_event("seed-drop","%d,%d",fmn_global.shovelx,fmn_global.shovely);
  fmn_saved_game_dirty();
}

/* Coin.
 */
 
struct fmn_hero_coin_collision_context {
  struct fmn_sprite *coin;
  struct fmn_sprite *blockage;
};
 
static int fmn_hero_coin_collision_1(struct fmn_sprite *sprite,void *userdata) {
  struct fmn_hero_coin_collision_context *ctx=userdata;
  if (sprite==ctx->coin) return 0;
  if (!(sprite->physics&FMN_PHYSICS_SPRITES)) return 0;
  if (sprite->radius<=0.0f) return 0;
  if (fmn_physics_check_sprites(0,0,ctx->coin,sprite)) {
    ctx->blockage=sprite;
    return 1;
  }
  return 0;
}
 
static struct fmn_sprite *fmn_hero_coin_collision(struct fmn_sprite *coin) {
  struct fmn_hero_coin_collision_context ctx={
    .coin=coin,
    .blockage=0,
  };
  fmn_sprites_for_each(fmn_hero_coin_collision_1,&ctx);
  return ctx.blockage;
}
 
static void fmn_hero_coin_begin() {
  struct fmn_sprite *hero=fmn_hero.sprite;
  fmn_global.active_item=0xff;
  if (!fmn_global.itemqv[FMN_ITEM_COIN]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_global.itemqv[FMN_ITEM_COIN]--;
  
  // Anything that collects coins might do so via collision with the missile, but we'll also fire it as an interaction.
  // This is important because we're going to reject coin toss if it collides initially.
  // And we'd like to encourage the witch to politely hand coins to merchants instead of throwing them.
  if (fmn_hero_interact_locally(FMN_ITEM_COIN,0)) {
    fmn_saved_game_dirty();
    return;
  }
  
  float dx,dy;
  fmn_vector_from_dir(&dx,&dy,fmn_global.facedir);
  uint8_t cmdv[]={
    0x42,FMN_SPRCTL_coin>>8,FMN_SPRCTL_coin,
  };
  struct fmn_sprite *coin=fmn_sprite_spawn(hero->x+dx,hero->y+dy,0,cmdv,sizeof(cmdv),0,0);
  if (!coin) return;
  coin->velx*=dx;
  coin->vely*=dy;
  
  
  // Before we commit to it, confirm that there is no collision.
  // If there is one, kill this sprite and put the coin back in your pocket.
  // Plus one last chance to hand it over politely.
  struct fmn_sprite *blockage;
  if (
    (blockage=fmn_hero_coin_collision(coin))||
    fmn_physics_check_grid(0,0,coin,coin->physics)
  ) {
    if (blockage&&blockage->interact&&blockage->interact(blockage,FMN_ITEM_COIN,0)) {
      fmn_sprite_kill(coin);
      return;
    }
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    fmn_sprite_kill(coin);
    fmn_global.itemqv[FMN_ITEM_COIN]++;
    return;
  }
  fmn_sound_effect(FMN_SFX_COIN_TOSS);
  fmn_saved_game_dirty();
}

/* Cheese.
 */
 
static void fmn_hero_cheese_begin() {
  fmn_global.active_item=0xff;
  if (!fmn_global.itemqv[FMN_ITEM_CHEESE]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_sound_effect(FMN_SFX_CHEESE);
  fmn_global.itemqv[FMN_ITEM_CHEESE]--;
  fmn_hero.cheesetime+=FMN_HERO_CHEESE_TIME;
  fmn_global.cheesing=1;
  fmn_saved_game_dirty();
}

/* Pitcher.
 */
 
static uint8_t fmn_hero_pitcher_pickup_from_environment() {
  int16_t result=fmn_hero_interact_locally(FMN_ITEM_PITCHER,0);
  if (result) return result;
  
  float x,y;
  fmn_vector_from_dir(&x,&y,fmn_global.facedir);
  x*=0.5f;
  y*=0.5f;
  x+=fmn_hero.sprite->x;
  y+=fmn_hero.sprite->y;
  if ((x<0.0f)||(y<0.0f)||(x>=FMN_COLC)||(y>=FMN_ROWC)) return 0;
  uint8_t cellp=((uint8_t)y)*FMN_COLC+(uint8_t)x;
  uint8_t tileid=fmn_global.map[cellp];
  uint8_t physics=fmn_global.cellphysics[tileid];
  
  switch (physics) {
    case FMN_CELLPHYSICS_SAP: return FMN_PITCHER_CONTENT_SAP;
    case FMN_CELLPHYSICS_SAP_NOCHALK: return FMN_PITCHER_CONTENT_SAP;
    case FMN_CELLPHYSICS_WATER: return FMN_PITCHER_CONTENT_WATER;
  }
  
  return 0;
}

static void fmn_hero_water_plants(uint8_t liquid) {
  if (fmn_global.shovelx<0) return;
  if (fmn_global.shovelx>=FMN_COLC) return;
  if (fmn_global.shovely<0) return;
  if (fmn_global.shovely>=FMN_ROWC) return;
  struct fmn_plant *plant=fmn_global.plantv;
  uint8_t i=fmn_global.plantc;
  for (;i-->0;plant++) {
    if (plant->x!=fmn_global.shovelx) continue;
    if (plant->y!=fmn_global.shovely) continue;
    if (plant->state!=FMN_PLANT_STATE_SEED) continue;
    plant->fruit=liquid;
    plant->state=FMN_PLANT_STATE_GROW;
    plant->flower_time=fmn_game_get_platform_time_ms()+FMN_FLOWER_TIME_MS;
    fmn_sound_effect(FMN_SFX_SPROUT);
    fmn_map_dirty();
    return;
  }
}
 
static void fmn_hero_pitcher_begin() {
  // Item stays "active", for visual purposes only.
  fmn_global.wand_dir=0;
  if (fmn_global.itemqv[FMN_ITEM_PITCHER]==FMN_PITCHER_CONTENT_EMPTY) {
    if (fmn_global.itemqv[FMN_ITEM_PITCHER]=fmn_hero_pitcher_pickup_from_environment()) {
      fmn_sound_effect(FMN_SFX_PITCHER_PICKUP);
      fmn_global.show_off_item=FMN_ITEM_PITCHER|(fmn_global.itemqv[FMN_ITEM_PITCHER]<<4);
      fmn_global.show_off_item_time=0xff;
      fmn_saved_game_dirty();
    } else {
      fmn_sound_effect(FMN_SFX_PITCHER_NO_PICKUP);
    }
  } else {
    fmn_sound_effect(FMN_SFX_PITCHER_POUR);
    uint8_t content=fmn_global.itemqv[FMN_ITEM_PITCHER];
    fmn_global.itemqv[FMN_ITEM_PITCHER]=FMN_PITCHER_CONTENT_EMPTY;
    fmn_hero_interact_locally(FMN_ITEM_PITCHER,content);
    fmn_global.wand_dir=content;
    fmn_hero_water_plants(content);
    fmn_saved_game_dirty();
  }
}

/* Match.
 */
 
static void fmn_hero_match_begin() {
  if (!fmn_global.itemqv[FMN_ITEM_MATCH]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_global.match_illumination_time+=FMN_HERO_MATCH_ILLUMINATION_TIME;
  fmn_global.itemqv[FMN_ITEM_MATCH]--;
  fmn_sound_effect(FMN_SFX_MATCH);
  fmn_saved_game_dirty();
}

/* Shovel.
 */
 
static void fmn_hero_shovel_begin() {
  if ((fmn_global.shovelx>=0)&&(fmn_global.shovely>=0)&&(fmn_global.shovelx<FMN_COLC)&&(fmn_global.shovely<FMN_ROWC)) {
    uint8_t tilep=fmn_global.shovely*FMN_COLC+fmn_global.shovelx;
    uint8_t pvtile=fmn_global.map[tilep];
    uint8_t pvphysics=fmn_global.cellphysics[pvtile];
    if (pvphysics==FMN_CELLPHYSICS_VACANT) {
    
      // If there's a plant here, destroy it.
      struct fmn_plant *plant=fmn_global.plantv;
      uint8_t i=fmn_global.plantc;
      for (;i-->0;plant++) {
        if (plant->state==FMN_PLANT_STATE_NONE) continue;
        if (plant->x!=fmn_global.shovelx) continue;
        if (plant->y!=fmn_global.shovely) continue;
        if (plant->state!=FMN_PLANT_STATE_DEAD) { // dead plants have no soul
          struct fmn_sprite *goast=fmn_sprite_generate_toast(plant->x+0.5f,plant->y,3,0xf9,0);
          if (goast) {
            goast->style=FMN_SPRITE_STYLE_TWOFRAME;
          }
        }
        plant->state=FMN_PLANT_STATE_NONE;
        fmn_map_dirty();
        fmn_saved_game_dirty();
        break;
      }
      
      // Hard-stop motion. It's disconcerting if she starts digging while walking, then slides to the next tile.
      fmn_hero.walkdx=0;
      fmn_hero.walkdy=0;
      fmn_hero.walkspeed=0.0f;
      fmn_hero.sprite->velx=0.0f;
      fmn_hero.sprite->vely=0.0f;
      
      // Look for buried treasure and doors.
      const struct fmn_door *door=fmn_global.doorv;
      for (i=fmn_global.doorc;i-->0;door++) {
        if (door->x!=fmn_global.shovelx) continue;
        if (door->y!=fmn_global.shovely) continue;
        if (door->mapid) {
          if (!door->extra) continue; // not buried
          if (door->extra!=68) { // transient_hole is a special gsbit; don't actually set it
            fmn_gs_set_bit(door->extra,1);
          }
          fmn_global.map[tilep]=0x3f;
          fmn_map_dirty();
          fmn_sound_effect(FMN_SFX_UNBURY_DOOR);
          fmn_secrets_refresh_for_map();
          fmn_saved_game_dirty();
          return;
        } else {
          if (door->dstx==0x30) { // buried treasure
            fmn_global.map[tilep]=0x0f;
            fmn_map_dirty();
            if (door->extra&&fmn_gs_get_bit(door->extra)) { // already got it
              fmn_sound_effect(FMN_SFX_DIG);
            } else {
              if (door->extra) fmn_gs_set_bit(door->extra,1);
              fmn_sound_effect(FMN_SFX_UNBURY_TREASURE);
              fmn_collect_item(door->dsty,0);
            }
            return;
          }
        }
      }
    
      // Sound effect and swap the tile.
      fmn_global.map[tilep]=0x0f;
      fmn_map_dirty();
      fmn_sound_effect(FMN_SFX_DIG);
      return;
    }
  }
  fmn_hero.item_active_time+=0.300f; // cheat the active time forward to shorten the animation.
  fmn_sound_effect(FMN_SFX_REJECT_DIG);
}

static void fmn_hero_shovel_end() {
  // Prevent end of action; we do it on a fixed timer.
}

static void fmn_hero_shovel_update(float elapsed) {
  if (fmn_hero.item_active_time>=FMN_HERO_SHOVEL_TIME) {
    fmn_global.active_item=0xff;
  }
}

/* Broom.
 */
 
static void fmn_hero_broom_begin() {
  fmn_hero.sprite->physics&=~FMN_PHYSICS_HOLE;
  fmn_hero.landing_pending=0;
}

static uint8_t fmn_hero_broom_ok_to_end() {
  // Offscreen is NOT ok; we don't know whether it's a hole or not, so must assume the worst.
  if (fmn_hero.cellx<0) return 0;
  if (fmn_hero.celly<0) return 0;
  if (fmn_hero.cellx>=FMN_COLC) return 0;
  if (fmn_hero.celly>=FMN_ROWC) return 0;
  // Hole tiles are NOT ok.
  uint8_t tilep=fmn_hero.celly*FMN_COLC+fmn_hero.cellx;
  uint8_t tile=fmn_global.map[tilep];
  uint8_t physics=fmn_global.cellphysics[tile];
  if (physics==FMN_CELLPHYSICS_HOLE) return 0;
  if (physics==FMN_CELLPHYSICS_WATER) return 0;
  // Everything else is fine.
  return 1;
}

static void fmn_hero_broom_end() {
  if (fmn_hero_broom_ok_to_end()) {
    fmn_hero.sprite->physics|=FMN_PHYSICS_HOLE;
    fmn_global.active_item=0xff;
    if ((fmn_hero.cellx>=0)&&(fmn_hero.celly>=0)&&(fmn_hero.cellx<FMN_COLC)&&(fmn_hero.celly<FMN_ROWC)) {
      fmn_game_check_static_hazards(fmn_hero.cellx,fmn_hero.celly);
    }
  } else {
    fmn_hero.landing_pending=1;
  }
}

static void fmn_hero_broom_update(float elapsed) {
  if (fmn_hero.landing_pending) {
    if ((fmn_hero.cellx>=0)&&(fmn_hero.celly>=0)&&(fmn_hero.cellx<FMN_COLC)&&(fmn_hero.celly<FMN_ROWC)) {
      uint8_t tilep=fmn_hero.celly*FMN_COLC+fmn_hero.cellx;
      uint8_t tile=fmn_global.map[tilep];
      uint8_t physics=fmn_global.cellphysics[tile];
      if ((physics!=FMN_CELLPHYSICS_HOLE)&&(physics!=FMN_CELLPHYSICS_WATER)) {
        fmn_hero.sprite->physics|=FMN_PHYSICS_HOLE;
        fmn_global.active_item=0xff;
        return;
      }
    }
  }
}

// Broom is unusual in that you can end it, it doesn't actually end, and then restart it.
// No feedback in this case, because to the user it seems that nothing changes.
static void fmn_hero_broom_restart() {
  fmn_hero.landing_pending=0;
}

/* Snowglobe. Basically the same thing as Wand, but strokes take effect immediately.
 */
 
static int fmn_snowglobe_cb(struct fmn_sprite *sprite,void *userdata) {
  if (sprite->interact) {
    sprite->interact(sprite,FMN_ITEM_SNOWGLOBE,0);
  }
  return 0;
}
 
static void fmn_hero_snowglobe_begin() {
  fmn_global.wand_dir=0;
}

static void fmn_hero_snowglobe_end() {
}
 
static void fmn_hero_snowglobe_motion(uint8_t bit,uint8_t value) {
  uint8_t ndir=fmn_global.wand_dir;
  if (value) {
    switch (bit) {
      case FMN_INPUT_LEFT: ndir=FMN_DIR_W; break;
      case FMN_INPUT_RIGHT: ndir=FMN_DIR_E; break;
      case FMN_INPUT_UP: ndir=FMN_DIR_N; break;
      case FMN_INPUT_DOWN: ndir=FMN_DIR_S; break;
    }
  } else {
    switch (bit) {
      case FMN_INPUT_LEFT: if (fmn_global.wand_dir==FMN_DIR_W) ndir=0; break;
      case FMN_INPUT_RIGHT: if (fmn_global.wand_dir==FMN_DIR_E) ndir=0; break;
      case FMN_INPUT_UP: if (fmn_global.wand_dir==FMN_DIR_N) ndir=0; break;
      case FMN_INPUT_DOWN: if (fmn_global.wand_dir==FMN_DIR_S) ndir=0; break;
    }
  }
  if (ndir==fmn_global.wand_dir) return;
  if (fmn_global.wand_dir=ndir) {
    fmn_sprites_for_each(fmn_snowglobe_cb,0);
    fmn_game_begin_earthquake(fmn_dir_reverse(ndir));
  }
}

/* Wand.
 */
 
static void fmn_hero_wand_begin() {
  fmn_global.wand_dir=0;
  fmn_hero.spellc=0;
}

static void fmn_hero_wand_end() {
  // Drop the wand without having encoded anything, just let it go, no worries.
  if (!fmn_hero.spellc) return;
  // If the length is out of range, it's automatically invalid. Otherwise query the spell service.
  uint8_t spellid=0;
  if (fmn_hero.spellc<=FMN_HERO_SPELL_LIMIT) {
    spellid=fmn_spell_eval(fmn_hero.spellv,fmn_hero.spellc);
  }
  if (spellid) {
    fmn_spell_cast(spellid);
  } else {
    fmn_global.spell_repudiation=0xff;
  }
}
 
static void fmn_hero_wand_motion(uint8_t bit,uint8_t value) {
  uint8_t ndir=fmn_global.wand_dir;
  if (value) {
    switch (bit) {
      case FMN_INPUT_LEFT: ndir=FMN_DIR_W; break;
      case FMN_INPUT_RIGHT: ndir=FMN_DIR_E; break;
      case FMN_INPUT_UP: ndir=FMN_DIR_N; break;
      case FMN_INPUT_DOWN: ndir=FMN_DIR_S; break;
    }
  } else {
    switch (bit) {
      case FMN_INPUT_LEFT: if (fmn_global.wand_dir==FMN_DIR_W) ndir=0; break;
      case FMN_INPUT_RIGHT: if (fmn_global.wand_dir==FMN_DIR_E) ndir=0; break;
      case FMN_INPUT_UP: if (fmn_global.wand_dir==FMN_DIR_N) ndir=0; break;
      case FMN_INPUT_DOWN: if (fmn_global.wand_dir==FMN_DIR_S) ndir=0; break;
    }
  }
  if (ndir==fmn_global.wand_dir) return;
  fmn_global.wand_dir=ndir;
  if (ndir) {
    if (fmn_hero.spellc<FMN_HERO_SPELL_LIMIT) {
      fmn_hero.spellv[fmn_hero.spellc]=ndir;
    } else { // rotate to keep the end of the spell in buffer, for display purposes
      memmove(fmn_hero.spellv,fmn_hero.spellv+1,sizeof(fmn_hero.spellv)-1);
      fmn_hero.spellv[FMN_HERO_SPELL_LIMIT-1]=ndir;
    }
    if (fmn_hero.spellc<0xff) fmn_hero.spellc++;
  }
}

/* Violin.
 */
 
uint8_t fmn_violin_note_from_dir(uint8_t dir) { // NOT STATIC. fmn_sprite_musicteacher.c borrows this too
  // Consider using a different note depending on the last one played.
  // Will be a little complicated, but it would be nice to expand the tones available for songs.
  // That will be auditory only; a canonical song will always be composed of The Four Notes.
  switch (dir) {
    case FMN_DIR_N: return 0x47;
    case FMN_DIR_E: return 0x45;
    case FMN_DIR_W: return 0x43;
    case FMN_DIR_S: return 0x40;
  }
  return 0;
}

static void fmn_violin_check_song() {
  // Copy out of the ring buffer and trim space.
  uint8_t song[FMN_VIOLIN_SONG_LENGTH];
  uint8_t dstc=0,srcp=fmn_global.violin_songp;
  uint8_t i=FMN_VIOLIN_SONG_LENGTH;
  for (;i-->0;srcp++) {
    if (srcp>=FMN_VIOLIN_SONG_LENGTH) srcp=0;
    if (dstc||fmn_global.violin_song[srcp]) {
      song[dstc++]=fmn_global.violin_song[srcp];
    }
  }
  // Trim trailing space and require at least one. (don't commit-and-abort instantly upon the last note)
  if (!dstc) return;
  if (song[--dstc]) return;
  while (dstc&&!song[dstc-1]) dstc--;
  if (dstc) {
    uint8_t spellid=fmn_song_eval(song,dstc);
    if (spellid) {
      fmn_hero.violin_spellid=spellid;
      fmn_game_event_broadcast(FMN_GAME_EVENT_SONG_OK,&spellid);
    }
  }
}
 
static void fmn_hero_violin_begin() {
  fmn_global.wand_dir=0;
  memset(fmn_global.violin_song,0,FMN_VIOLIN_SONG_LENGTH);
  memset(fmn_global.violin_shadow,0,FMN_VIOLIN_SONG_LENGTH);
  fmn_global.violin_clock=0.0f;
  fmn_global.violin_songp=0;
  fmn_hero.next_metronome_songp=0;
  fmn_hero.violin_spellid=0;
  fmn_synth_event(0x0e,0xc0,1,0); // load violin program in channel 14
  fmn_synth_event(0x0e,0xb0,0x07,0x7f);
}

static void fmn_hero_violin_end() {
  if (fmn_global.wand_dir) {
    fmn_synth_event(0x0e,0x80,fmn_violin_note_from_dir(fmn_global.wand_dir),0x40);
  }
  if (fmn_hero.violin_spellid) {
    fmn_spell_cast(fmn_hero.violin_spellid);
    fmn_hero.violin_spellid=0;
    if (!fmn_hero_facedir_agrees()) {
      fmn_hero_reset_facedir();
    }
    fmn_global.walking=0; // force-stop walking
    fmn_hero.walkdx=0;
    fmn_hero.walkdy=0;
  }
}

static void fmn_hero_violin_update(float elapsed) {
  if (fmn_hero.violin_spellid) return;
  fmn_global.violin_clock+=elapsed*FMN_VIOLIN_BEATS_PER_SEC;
  
  // Metronome sounds halfway thru each beat, not at the transitions.
  // Because when we record a note, we floor time rather than rounding.
  if ((fmn_global.violin_songp==fmn_hero.next_metronome_songp)&&(fmn_global.violin_clock>=0.5f)) {
    fmn_sound_effect(FMN_SFX_COWBELL);
    fmn_hero.next_metronome_songp++;
    if (fmn_hero.next_metronome_songp>=FMN_VIOLIN_SONG_LENGTH) {
      fmn_hero.next_metronome_songp-=FMN_VIOLIN_SONG_LENGTH;
    }
  }
  
  while (fmn_global.violin_clock>=1.0f) {
    fmn_global.violin_clock-=1.0f;
    fmn_global.violin_songp++;
    if (fmn_global.violin_songp>=FMN_VIOLIN_SONG_LENGTH) {
      fmn_global.violin_songp=0;
    }
    fmn_global.violin_song[fmn_global.violin_songp]=0;
    fmn_global.violin_shadow[fmn_global.violin_songp]=0;
    fmn_violin_check_song();
  }
}

static void fmn_hero_violin_motion(uint8_t bit,uint8_t value) {
  uint8_t ndir=fmn_global.wand_dir;
  if (value) {
    switch (bit) {
      case FMN_INPUT_LEFT: ndir=FMN_DIR_W; break;
      case FMN_INPUT_RIGHT: ndir=FMN_DIR_E; break;
      case FMN_INPUT_UP: ndir=FMN_DIR_N; break;
      case FMN_INPUT_DOWN: ndir=FMN_DIR_S; break;
    }
  } else {
    switch (bit) {
      case FMN_INPUT_LEFT: if (fmn_global.wand_dir==FMN_DIR_W) ndir=0; break;
      case FMN_INPUT_RIGHT: if (fmn_global.wand_dir==FMN_DIR_E) ndir=0; break;
      case FMN_INPUT_UP: if (fmn_global.wand_dir==FMN_DIR_N) ndir=0; break;
      case FMN_INPUT_DOWN: if (fmn_global.wand_dir==FMN_DIR_S) ndir=0; break;
    }
  }
  if (ndir==fmn_global.wand_dir) return;
  if (fmn_global.wand_dir) {
    fmn_synth_event(0x0e,0x80,fmn_violin_note_from_dir(fmn_global.wand_dir),0x40);
  }
  fmn_global.wand_dir=ndir;
  if (ndir) {
    if (!fmn_hero.violin_spellid) {
      int8_t p=fmn_global.violin_songp-1;
      if (p<0) p=FMN_VIOLIN_SONG_LENGTH-1;
      else if (p>=FMN_VIOLIN_SONG_LENGTH) p=0;
      fmn_global.violin_song[p]=ndir;
    }
    fmn_synth_event(0x0e,0x90,fmn_violin_note_from_dir(fmn_global.wand_dir),0x40);
  }
}

/* Chalk.
 */
 
static int8_t fmn_hero_sketchx,fmn_hero_sketchy;
 
static void fmn_hero_chalk_cb(struct fmn_menu *menu,uint8_t message) {
  switch (message) {
    case FMN_MENU_MESSAGE_CANCEL:
    case FMN_MENU_MESSAGE_SUBMIT: {
        fmn_dismiss_menu(menu);
      } break;
    case FMN_MENU_MESSAGE_CHANGED: {
        struct fmn_sketch *sketch=fmn_global.sketchv;
        int i=fmn_global.sketchc;
        for (;i-->0;sketch++) {
          if (sketch->x!=fmn_hero_sketchx) continue;
          if (sketch->y!=fmn_hero_sketchy) continue;
          sketch->bits=menu->argv[0];
          fmn_map_dirty();
          fmn_saved_game_dirty();
          break;
        }
      } break;
  }
}
 
static void fmn_hero_chalk_begin() {
  int8_t x=fmn_hero.sprite->x;
  int8_t y=fmn_hero.sprite->y-1.0f;
  if ((x<0)||(y<0)||(x>=FMN_COLC)||(y>=FMN_ROWC)||(fmn_global.facedir!=FMN_DIR_N)) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  uint8_t tilep=y*FMN_COLC+x;
  uint8_t pvtile=fmn_global.map[tilep];
  uint8_t pvphysics=fmn_global.cellphysics[pvtile];
  if ((pvphysics!=FMN_CELLPHYSICS_SOLID)&&(pvphysics!=FMN_CELLPHYSICS_SAP)&&(pvphysics!=FMN_CELLPHYSICS_REVELABLE)) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  uint32_t bits=fmn_begin_sketch(x,y);
  if (bits==0xffffffff) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    fmn_log_event("sketch-reject","%d,%d",x,y);
    return;
  }
  fmn_log_event("sketch","%d,%d,0x%x",x,y,bits);
  fmn_hero.chalking=2; // there will be one update between here and the modal; ignore it // TODO render-redesign: is this still true?
  struct fmn_menu *menu=fmn_begin_menu(FMN_MENU_CHALK,bits);
  if (!menu) return;
  menu->cb=fmn_hero_chalk_cb;
  fmn_hero_sketchx=x;
  fmn_hero_sketchy=y;
}

/* Feather.
 */
 
static void fmn_hero_feather_begin() {
  fmn_hero.feather_target=0;
  // Don't bother looking for a target; we'll get him at the next update.
}

struct fmn_hero_feather_find_target_context {
  float x,y;
  struct fmn_sprite *target;
};

static int fmn_hero_feather_find_target_1(struct fmn_sprite *sprite,void *userdata) {
  struct fmn_hero_feather_find_target_context *ctx=userdata;
  if (!sprite->interact) return 0;
  if (sprite->radius>0.0f) {
    if (sprite->x-sprite->radius>=ctx->x) return 0;
    if (sprite->x+sprite->radius<=ctx->x) return 0;
    if (sprite->y-sprite->radius>=ctx->y) return 0;
    if (sprite->y+sprite->radius<=ctx->y) return 0;
  } else {
    if (sprite->x-sprite->hbw>=ctx->x) return 0;
    if (sprite->x+sprite->hbe<=ctx->x) return 0;
    if (sprite->y-sprite->hbn>=ctx->y) return 0;
    if (sprite->y+sprite->hbs<=ctx->y) return 0;
  }
  ctx->target=sprite;
  return 1;
}

static struct fmn_sprite *fmn_hero_feather_find_target() {
  struct fmn_hero_feather_find_target_context ctx={0};
  fmn_vector_from_dir(&ctx.x,&ctx.y,fmn_global.facedir);
  ctx.x*=0.6f;
  ctx.y*=0.6f;
  ctx.x+=fmn_hero.sprite->x;
  ctx.y+=fmn_hero.sprite->y;
  fmn_sprites_for_each(fmn_hero_feather_find_target_1,&ctx);
  return ctx.target;
}
 
static void fmn_hero_feather_update() {
  // Re-tickle the target sprite when it comes in range, but don't tickle at each update.
  // So don't use fmn_hero_interact_locally.
  struct fmn_sprite *target=fmn_hero_feather_find_target();
  if (target==fmn_hero.feather_target) return;
  fmn_hero.feather_target=target;
  if (target&&target->interact) target->interact(target,FMN_ITEM_FEATHER,0);
}

/* Compass: Everything useful is handled by render.
 */
 
static void fmn_hero_compass_begin() {
}

/* Dispatch on item type.
 */
 
void fmn_hero_item_begin() {

  // No actions while injured, that's a firm rule, it's part of the injury penalty.
  // Same deal for curses.
  if ((fmn_global.injury_time>0.0f)||(fmn_global.curse_time>0.0f)) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  
  // No actions while pumpkinned.
  if (fmn_global.transmogrification==1) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  
  // We do allow unpossessed items to be selected. Must verify first that we actually have it.
  if ((fmn_global.selected_item>=FMN_ITEM_COUNT)||!fmn_global.itemv[fmn_global.selected_item]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  
  // If some item is already active (selected or otherwise), reject.
  // This is super important: We might have the Broom 'active' after the key released, because she's in a broom-only position.
  if (fmn_global.active_item<FMN_ITEM_COUNT) {
    if (fmn_global.active_item==fmn_global.selected_item) switch (fmn_global.active_item) {
      case FMN_ITEM_BROOM: fmn_hero_broom_restart(); return;
    }
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  
  fmn_global.active_item=fmn_global.selected_item;
  fmn_hero.item_active_time=0;
  switch (fmn_global.active_item) {
    case FMN_ITEM_SNOWGLOBE: fmn_hero_snowglobe_begin(); break;
    case FMN_ITEM_BELL: fmn_hero_bell_begin(); break;
    case FMN_ITEM_SEED: fmn_hero_seed_begin(); break;
    case FMN_ITEM_COIN: fmn_hero_coin_begin(); break;
    case FMN_ITEM_CHEESE: fmn_hero_cheese_begin(); break;
    case FMN_ITEM_PITCHER: fmn_hero_pitcher_begin(); break;
    case FMN_ITEM_MATCH: fmn_hero_match_begin(); break;
    case FMN_ITEM_SHOVEL: fmn_hero_shovel_begin(); break;
    case FMN_ITEM_BROOM: fmn_hero_broom_begin(); break;
    case FMN_ITEM_WAND: fmn_hero_wand_begin(); break;
    case FMN_ITEM_VIOLIN: fmn_hero_violin_begin(); break;
    case FMN_ITEM_CHALK: fmn_hero_chalk_begin(); break;
    case FMN_ITEM_FEATHER: fmn_hero_feather_begin(); break;
    case FMN_ITEM_COMPASS: fmn_hero_compass_begin(); break;
  }
}

void fmn_hero_item_end() {
  switch (fmn_global.active_item) {
    case FMN_ITEM_SNOWGLOBE: fmn_hero_snowglobe_end(); break;
    case FMN_ITEM_SHOVEL: fmn_hero_shovel_end(); return;
    case FMN_ITEM_BROOM: fmn_hero_broom_end(); return;
    case FMN_ITEM_WAND: fmn_hero_wand_end(); break;
    case FMN_ITEM_VIOLIN: fmn_hero_violin_end(); break;
  }
  fmn_global.active_item=0xff;
  // The active item might have suppressed facedir changes. Bump it:
  if (!fmn_hero_facedir_agrees()) {
    fmn_hero_reset_facedir();
  }
}

void fmn_hero_cancel_item() {
  fmn_hero.spellc=0; // poison the spell; you can't cast it in cancel cases.
  memset(fmn_global.violin_song,0,sizeof(fmn_global.violin_song)); // ditto song
  fmn_hero.violin_spellid=0;
  fmn_hero_item_end();
}

static int fmn_hero_notify_chalk(struct fmn_sprite *sprite,void *dummy) {
  if (sprite->interact) sprite->interact(sprite,FMN_ITEM_CHALK,0);
  return 0;
}

void fmn_hero_item_update(float elapsed) {
  if (fmn_hero.chalking==2) {
    fmn_hero.chalking=1;
  } else if (fmn_hero.chalking) {
    fmn_hero.chalking=0;
    fmn_sprites_for_each(fmn_hero_notify_chalk,0);
  }
  fmn_hero.item_active_time+=elapsed;
  switch (fmn_global.active_item) {
    case FMN_ITEM_BELL: fmn_hero_bell_update(elapsed); break;
    case FMN_ITEM_SHOVEL: fmn_hero_shovel_update(elapsed); break;
    case FMN_ITEM_BROOM: fmn_hero_broom_update(elapsed); break;
    case FMN_ITEM_VIOLIN: fmn_hero_violin_update(elapsed); break;
    case FMN_ITEM_FEATHER: fmn_hero_feather_update(); break;
  }
}

uint8_t fmn_hero_item_motion(uint8_t bit,uint8_t value) {
  switch (fmn_global.active_item) {
    case FMN_ITEM_SNOWGLOBE: fmn_hero_snowglobe_motion(bit,value); return 1;
    case FMN_ITEM_WAND: fmn_hero_wand_motion(bit,value); return 1;
    case FMN_ITEM_VIOLIN: fmn_hero_violin_motion(bit,value); return 1;
  }
  return 0;
}

/* Button state changed.
 */
 
void fmn_hero_item_event(uint8_t buttonvalue) {
  if (buttonvalue) fmn_hero_item_begin();
  else fmn_hero_item_end();
}

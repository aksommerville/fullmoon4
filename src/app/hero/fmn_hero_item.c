#include "fmn_hero_internal.h"
#include "app/fmn_game.h"

/* Bell.
 */
 
static void fmn_hero_bell_begin() {
  fmn_sound_effect(FMN_SFX_BELL);
  fmn_hero.bell_count=1;
  //TODO whatever bells do
}

static void fmn_hero_bell_update(float elapsed) {
  // try to match animation; assume 60 Hz video.
  uint8_t current=(uint8_t)((fmn_hero.item_active_time*60.0f)/32.0f);
  if (current!=fmn_hero.bell_count) {
    fmn_sound_effect(FMN_SFX_BELL);
    fmn_hero.bell_count=current;
  }
}

/* Corn.
 */
 
static void fmn_hero_corn_begin() {
  fmn_global.active_item=0;
  if (!fmn_global.itemqv[FMN_ITEM_CORN]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_log("ok corn it");
  fmn_global.itemqv[FMN_ITEM_CORN]--;
  //TODO corn sprite
  //TODO attract a bird
}

/* Seed. TODO I'm pretty sure Corn and Seed are the same thing... play it out and consider combining to one item
 */
 
static void fmn_hero_seed_begin() {
  fmn_global.active_item=0;
  if (!fmn_global.itemqv[FMN_ITEM_SEED]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_log("ok plant seed");
  fmn_global.itemqv[FMN_ITEM_SEED]--;
  //TODO plant seed
}

/* Coin.
 */
 
static void fmn_hero_coin_begin() {
  fmn_global.active_item=0;
  if (!fmn_global.itemqv[FMN_ITEM_COIN]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_log("ok throw coin");
  fmn_global.itemqv[FMN_ITEM_COIN]--;
  //TODO throw coin
}

/* Cheese.
 */
 
static void fmn_hero_cheese_begin() {
  fmn_global.active_item=0;
  if (!fmn_global.itemqv[FMN_ITEM_CHEESE]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_sound_effect(FMN_SFX_CHEESE);
  fmn_global.itemqv[FMN_ITEM_CHEESE]--;
  fmn_hero.cheesetime+=FMN_HERO_CHEESE_TIME;
  fmn_global.cheesing=1;
}

/* Pitcher.
 */
 
static uint8_t XXX_pitcher_next=FMN_PITCHER_CONTENT_WATER;
 
static uint8_t fmn_hero_pitcher_pickup_from_environment() {
  uint8_t result=XXX_pitcher_next;//XXX very temporary
  switch (XXX_pitcher_next) {
    case FMN_PITCHER_CONTENT_WATER: XXX_pitcher_next=FMN_PITCHER_CONTENT_MILK; break;
    case FMN_PITCHER_CONTENT_MILK: XXX_pitcher_next=FMN_PITCHER_CONTENT_HONEY; break;
    case FMN_PITCHER_CONTENT_HONEY: XXX_pitcher_next=FMN_PITCHER_CONTENT_SAP; break;
    case FMN_PITCHER_CONTENT_SAP: XXX_pitcher_next=FMN_PITCHER_CONTENT_BLOOD; break;
    default: XXX_pitcher_next=FMN_PITCHER_CONTENT_WATER; break;
  }
  return result;
}
 
static void fmn_hero_pitcher_begin() {
  // Item stays "active", for visual purposes only.
  if (fmn_global.itemqv[FMN_ITEM_PITCHER]==FMN_PITCHER_CONTENT_EMPTY) {
    if (fmn_global.itemqv[FMN_ITEM_PITCHER]=fmn_hero_pitcher_pickup_from_environment()) {
      fmn_sound_effect(FMN_SFX_PITCHER_PICKUP);
      fmn_log("pitcher pickup %d",fmn_global.itemqv[FMN_ITEM_PITCHER]);
      //TODO visual feedback. show what we picked up
    } else {
      fmn_sound_effect(FMN_SFX_PITCHER_NO_PICKUP);
    }
  } else {
    fmn_sound_effect(FMN_SFX_PITCHER_POUR);
    fmn_log("pitcher pour %d",fmn_global.itemqv[FMN_ITEM_PITCHER]);
    //TODO locate target, do the thing....
    fmn_global.itemqv[FMN_ITEM_PITCHER]=FMN_PITCHER_CONTENT_EMPTY;
  }
}

/* Match.
 */
 
static void fmn_hero_match_begin() {
  if (!fmn_global.itemqv[FMN_ITEM_MATCH]) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  fmn_global.illumination_time+=FMN_HERO_MATCH_ILLUMINATION_TIME;
  fmn_global.itemqv[FMN_ITEM_MATCH]--;
  fmn_sound_effect(FMN_SFX_MATCH);
}

/* Shovel.
 */
 
static void fmn_hero_shovel_begin() {
  if ((fmn_global.shovelx>=0)&&(fmn_global.shovely>=0)&&(fmn_global.shovelx<FMN_COLC)&&(fmn_global.shovely<FMN_ROWC)) {
    uint8_t tilep=fmn_global.shovely*FMN_COLC+fmn_global.shovelx;
    uint8_t pvtile=fmn_global.map[tilep];
    uint8_t pvphysics=fmn_global.cellphysics[pvtile];
    if (pvphysics==FMN_CELLPHYSICS_VACANT) {
    
      fmn_global.map[tilep]=0x0f;
      fmn_map_dirty();
      fmn_sound_effect(FMN_SFX_DIG);
      //TODO find buried treasure?
      
      // Hard-stop motion. It's disconcerting if she starts digging while walking, then slides to the next tile.
      fmn_hero.walkdx=0;
      fmn_hero.walkdy=0;
      fmn_hero.walkspeed=0.0f;
      fmn_hero.sprite->velx=0.0f;
      fmn_hero.sprite->vely=0.0f;
    
      return;
    }
  }
  fmn_sound_effect(FMN_SFX_REJECT_DIG);
}

static void fmn_hero_shovel_end() {
  // Prevent end of action; we do it on a fixed timer.
}

static void fmn_hero_shovel_update(float elapsed) {
  if (fmn_hero.item_active_time>=FMN_HERO_SHOVEL_TIME) {
    fmn_global.active_item=0;
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
  // Everything else is fine.
  return 1;
}

static void fmn_hero_broom_end() {
  if (fmn_hero_broom_ok_to_end()) {
    fmn_hero.sprite->physics|=FMN_PHYSICS_HOLE;
    fmn_global.active_item=0;
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
      if (physics!=FMN_CELLPHYSICS_HOLE) {
        fmn_hero.sprite->physics|=FMN_PHYSICS_HOLE;
        fmn_global.active_item=0;
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
    //TODO spell repudiation
    //this is for debug only:
    if (fmn_hero.spellc>FMN_HERO_SPELL_LIMIT) fmn_hero.spellc=FMN_HERO_SPELL_LIMIT;
    int i=fmn_hero.spellc; while (i-->0) switch (fmn_hero.spellv[i]) {
      case FMN_DIR_W: fmn_hero.spellv[i]='W'; break;
      case FMN_DIR_E: fmn_hero.spellv[i]='E'; break;
      case FMN_DIR_N: fmn_hero.spellv[i]='N'; break;
      case FMN_DIR_S: fmn_hero.spellv[i]='S'; break;
      default: fmn_hero.spellv[i]='?'; break;
    }
    fmn_log("Not a valid spell: %.*s",fmn_hero.spellc,fmn_hero.spellv);
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
    if (fmn_hero.spellc<FMN_HERO_SPELL_LIMIT) fmn_hero.spellv[fmn_hero.spellc]=ndir;
    if (fmn_hero.spellc<0xff) fmn_hero.spellc++;
  }
}

/* Violin.
 */
 
static uint8_t fmn_violin_note_from_dir(uint8_t dir) {
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
      fmn_spell_cast(spellid);
      if (fmn_global.wand_dir) {
        fmn_synth_event(0x0e,0x80,fmn_violin_note_from_dir(fmn_global.wand_dir),0x40);
      }
      fmn_global.active_item=0;
    }
  }
}
 
static void fmn_hero_violin_begin() {
  fmn_global.wand_dir=0;
  memset(fmn_global.violin_song,0,FMN_VIOLIN_SONG_LENGTH);
  fmn_global.violin_clock=0.0f;
  fmn_global.violin_songp=0;
  fmn_hero.next_metronome_songp=0;
  fmn_synth_event(0x0e,0x0c,1,0); // load violin program in channel 14
}

static void fmn_hero_violin_end() {
  if (fmn_global.wand_dir) {
    fmn_synth_event(0x0e,0x80,fmn_violin_note_from_dir(fmn_global.wand_dir),0x40);
  }
}

static void fmn_hero_violin_update(float elapsed) {
  fmn_global.violin_clock+=elapsed*FMN_VIOLIN_BEATS_PER_SEC;
  
  // Metronome sounds halfway thru each beat, not at the transitions.
  // Because when we record a note, we floor time rather than rounding.
  if ((fmn_global.violin_songp==fmn_hero.next_metronome_songp)&&(fmn_global.violin_clock>=0.5f)) {
    fmn_sound_effect(FMN_SFX_COWBELL);
    fmn_hero.next_metronome_songp+=2;
    if (fmn_hero.next_metronome_songp>=FMN_VIOLIN_SONG_LENGTH) {
      fmn_hero.next_metronome_songp-=FMN_VIOLIN_SONG_LENGTH;
    }
  }
  
  while (fmn_global.violin_clock>=1.0f) {
    fmn_global.violin_clock-=1.0f;
    fmn_global.violin_song[fmn_global.violin_songp]=0;
    fmn_global.violin_songp++;
    if (fmn_global.violin_songp>=FMN_VIOLIN_SONG_LENGTH) {
      fmn_global.violin_songp=0;
    }
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
    int8_t p=fmn_global.violin_songp-1;
    if (p<0) p=FMN_VIOLIN_SONG_LENGTH-1;
    fmn_global.violin_song[p]=ndir;
    fmn_synth_event(0x0e,0x90,fmn_violin_note_from_dir(fmn_global.wand_dir),0x40);
  }
}

/* Chalk.
 */
 
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
  if (pvphysics!=FMN_CELLPHYSICS_SOLID) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  if (fmn_begin_sketch(x, y)<0) {
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
}

/* Dispatch on item type.
 */
 
void fmn_hero_item_begin() {

  // No actions while injured, that's a firm rule, it's part of the injury penalty.
  if (fmn_global.injury_time>0.0f) {
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
  if (fmn_global.active_item) {
    if (fmn_global.active_item==fmn_global.selected_item) switch (fmn_global.active_item) {
      case FMN_ITEM_BROOM: fmn_hero_broom_restart(); return;
    }
    fmn_sound_effect(FMN_SFX_REJECT_ITEM);
    return;
  }
  
  fmn_global.active_item=fmn_global.selected_item;
  fmn_hero.item_active_time=0;
  switch (fmn_global.active_item) {
    case FMN_ITEM_BELL: fmn_hero_bell_begin(); break;
    case FMN_ITEM_CORN: fmn_hero_corn_begin(); break;
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
  }
}

void fmn_hero_item_end() {
  switch (fmn_global.active_item) {
    case FMN_ITEM_SHOVEL: fmn_hero_shovel_end(); return;
    case FMN_ITEM_BROOM: fmn_hero_broom_end(); return;
    case FMN_ITEM_WAND: fmn_hero_wand_end(); break;
    case FMN_ITEM_VIOLIN: fmn_hero_violin_end(); break;
  }
  fmn_global.active_item=0;
  // The active item might have suppressed facedir changes. Bump it:
  if (!fmn_hero_facedir_agrees()) {
    fmn_hero_reset_facedir();
  }
}

void fmn_hero_item_update(float elapsed) {
  fmn_hero.item_active_time+=elapsed;
  switch (fmn_global.active_item) {
    case FMN_ITEM_BELL: fmn_hero_bell_update(elapsed); break;
    case FMN_ITEM_SHOVEL: fmn_hero_shovel_update(elapsed); break;
    case FMN_ITEM_BROOM: fmn_hero_broom_update(elapsed); break;
    case FMN_ITEM_VIOLIN: fmn_hero_violin_update(elapsed); break;
  }
}

uint8_t fmn_hero_item_motion(uint8_t bit,uint8_t value) {
  switch (fmn_global.active_item) {
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

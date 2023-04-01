#include "fmn_game.h"
#include "sprite/fmn_sprite.h"
#include "sprite/fmn_physics.h"
#include "hero/fmn_hero.h"

/* Init.
 */
 
int fmn_game_init() {
  fmn_global.itemv[FMN_ITEM_NONE]=1; // let it show an icon in the inventory, so it doesn't look like an item not found yet
  if (fmn_game_load_map(1)<1) return -1;
  fmn_map_callbacks(FMN_MAP_EVID_LOADED,fmn_game_map_callback,0);
  return 0;
}

/* Spawn sprite in new map.
 */
 
static void cb_spawn(
  int8_t x,int8_t y,
  uint16_t spriteid,uint8_t arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3,
  const uint8_t *cmdv,uint16_t cmdc
) {
  uint8_t argv[]={arg0,arg1,arg2,arg3};
  struct fmn_sprite *sprite=fmn_sprite_spawn(x+0.5f,y+0.5f,spriteid,cmdv,cmdc,argv,sizeof(argv));
  if (!sprite) {
    // This is actually normal; a sprite controller can decide it's not needed, eg treasure chest.
    return;
  }
}

/* Map callback events.
 */
 
void fmn_game_map_callback(uint16_t cbid,uint8_t param,void *userdata) {
  switch (cbid) {
    #define _(tag) case FMN_MAP_CALLBACK_##tag: fmn_map_callback_##tag(param,userdata); break;
    FMN_FOR_EACH_MAP_CALLBACK
    #undef _
  }
}

/* Load map.
 * This DOES NOT trigger FMN_MAP_EVID_LOADED, but the caller should. Update hero position first.
 */
 
int fmn_game_load_map(int mapid) {
  fmn_map_callbacks(FMN_MAP_EVID_UNLOAD,fmn_game_map_callback,0);
  fmn_sprites_clear();
  fmn_gs_drop_listeners();
  fmn_game_event_unlisten_all();
  fmn_game_drop_map_singletons();
  int err=fmn_load_map(mapid,cb_spawn);
  if (err<=0) return err;
  if (fmn_hero_reset()<0) return -1;
  fmn_secrets_refresh_for_map();
  return 1;
}

/* Input event.
 */
 
void fmn_game_input(uint8_t bit,uint8_t value,uint8_t state) {
  fmn_hero_input(bit,value,state);
}

/* Trigger navigation.
 */
 
static void fmn_game_navigate(int8_t dx,int8_t dy) {
  uint16_t mapid=0;
  uint8_t transition=0;
       if (dx<0) { mapid=fmn_global.neighborw; transition=FMN_TRANSITION_PAN_LEFT; }
  else if (dx>0) { mapid=fmn_global.neighbore; transition=FMN_TRANSITION_PAN_RIGHT; }
  else if (dy<0) { mapid=fmn_global.neighborn; transition=FMN_TRANSITION_PAN_UP; }
  else if (dy>0) { mapid=fmn_global.neighbors; transition=FMN_TRANSITION_PAN_DOWN; }
  if (!mapid) return;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  fmn_prepare_transition(transition);
  if (fmn_game_load_map(mapid)<1) {
    fmn_cancel_transition();
    return;
  }
  fmn_hero_set_position(herox-dx*FMN_COLC,heroy-dy*FMN_ROWC);
  // Preserve velocity on these neighbor transitions.
  fmn_map_callbacks(FMN_MAP_EVID_LOADED,fmn_game_map_callback,0);
  fmn_commit_transition();
}

static void fmn_transmogrify(uint8_t mode,uint8_t state) {
  if (fmn_global.transmogrification==state) {
    if (!(mode&0x40)) return;
    fmn_global.transmogrification=0;
    fmn_sound_effect(FMN_SFX_UNPUMPKIN);
  } else {
    if (!(mode&0x80)) return;
    fmn_global.transmogrification=state;
    fmn_sound_effect(FMN_SFX_PUMPKIN);
  }
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  fmn_sprite_generate_soulballs(herox,heroy,7);
  fmn_secrets_refresh_for_map();
}

static uint8_t fmn_game_check_doors(uint8_t x,uint8_t y) {
  struct fmn_door *door=fmn_global.doorv;
  uint32_t i=fmn_global.doorc;
  for (;i-->0;door++) {
    if (door->x!=x) continue;
    if (door->y!=y) continue;
    
    // Some things masquerade as doors, but are in fact something very else.
    if (!door->mapid) {
      if ((door->dsty>0)&&(door->dsty<0x40)&&(door->dstx&0xc0)&&!(door->dstx&0x3f)) {
        fmn_transmogrify(door->dstx,door->dsty);
        continue;
      }
      continue;
    }
    
    // If (extra) nonzero, it's a gsbit telling whether the door is revealed.
    if (door->extra) {
      if (!fmn_gs_get_bit(door->extra)) continue;
    }
    
    float dstx=door->dstx+0.5f;
    float dsty=door->dsty+0.5f;
    fmn_prepare_transition(FMN_TRANSITION_DOOR);
    if (fmn_game_load_map(door->mapid)<1) {
      fmn_cancel_transition();
      continue;
    }
    
    // After placing the hero, pump its quantized position once to ignore whatever it landed on.
    // (typically, that's the other end of this door).
    // Also kill velocity.
    fmn_hero_set_position(dstx,dsty);
    int8_t dumx,dumy;
    fmn_hero_get_quantized_position(&dumx,&dumy);
    fmn_hero_kill_velocity();
    fmn_global.shovelx=fmn_global.shovely=-1;
    
    fmn_map_callbacks(FMN_MAP_EVID_LOADED,fmn_game_map_callback,0);
    
    fmn_commit_transition();
    return 1;
  }
  return 0;
}

/* Collect item.
 */
 
const uint8_t fmn_item_default_quantities[FMN_ITEM_COUNT]={
  [FMN_ITEM_SEED]=5,
  [FMN_ITEM_COIN]=5,
  [FMN_ITEM_MATCH]=5,
  [FMN_ITEM_CHEESE]=5,
};

static void cb_modal_dummy() {
}
 
uint8_t fmn_collect_item(uint8_t itemid,uint8_t quantity) {

  // Get out if invalid, or we can't pick up more of it. Establish quantity.
  if (itemid>=FMN_ITEM_COUNT) return 0;
  if (fmn_item_default_quantities[itemid]) {
    if (!quantity) quantity=fmn_item_default_quantities[itemid];
  } else {
    quantity=0;
  }
  if (fmn_global.itemv[itemid]&&!quantity) return 0;
  if (quantity) {
    if (fmn_global.itemqv[itemid]==0xff) return 0;
    if (fmn_global.itemqv[itemid]>0xff-quantity) fmn_global.itemqv[itemid]=0xff;
    else fmn_global.itemqv[itemid]+=quantity;
  }
  
  // First time picking something up, do the bells and whistles.
  if (!fmn_global.itemv[itemid]) {
    fmn_global.itemv[itemid]=1;
    fmn_global.selected_item=itemid;
    fmn_sound_effect(FMN_SFX_ITEM_MAJOR);
    fmn_begin_menu(FMN_MENU_TREASURE,itemid,cb_modal_dummy);
    fmn_hero_kill_velocity();
    
  // Subsequent pick-ups, do a more passive celebration.
  } else {
    fmn_global.show_off_item=itemid;
    fmn_global.show_off_item_time=0xff;
    fmn_sound_effect(FMN_SFX_ITEM_MINOR);
  }
  
  fmn_secrets_refresh_for_map();
  return 1;
}

/* If we've stepped on a flowered plant, collect it.
 */
 
static void fmn_game_check_plant_collection(uint8_t x,uint8_t y) {
  struct fmn_plant *plant=fmn_global.plantv;
  uint8_t i=fmn_global.plantc;
  for (;i-->0;plant++) {
    if (plant->x!=x) continue;
    if (plant->y!=y) continue;
    if (plant->state!=FMN_PLANT_STATE_FLOWER) continue;
    uint8_t itemid=0,quantity=5;
    switch (plant->fruit) {
      case FMN_PLANT_FRUIT_SEED: itemid=FMN_ITEM_SEED; break;
      case FMN_PLANT_FRUIT_MATCH: itemid=FMN_ITEM_MATCH; break;
      case FMN_PLANT_FRUIT_CHEESE: itemid=FMN_ITEM_CHEESE; break;
      case FMN_PLANT_FRUIT_COIN: itemid=FMN_ITEM_COIN; break;
    }
    if (itemid) {
      if (!fmn_collect_item(itemid,quantity)) {
        continue;
      }
      plant->state=FMN_PLANT_STATE_DEAD;
      fmn_map_dirty();
    }
    return;
  }
}

/* Update weather.
 * Returns the new elapsed time to report to sprites, in case there's a slowstorm in progress.
 */
 
struct fmn_wind_context {
  float dx;
  float dy;
  float others_time;
  float hero_time;
};

static int fmn_wind_blow_1(struct fmn_sprite *sprite,void *userdata) {
  struct fmn_wind_context *ctx=userdata;
  if (sprite->style==FMN_SPRITE_STYLE_HERO) {
    sprite->x+=ctx->dx*ctx->hero_time;
    sprite->y+=ctx->dy*ctx->hero_time;
  } else if (sprite->physics&FMN_PHYSICS_BLOWABLE) {
    sprite->x+=ctx->dx*ctx->others_time;
    sprite->y+=ctx->dy*ctx->others_time;
    if (sprite->wind) sprite->wind(sprite,ctx->dx*ctx->others_time,ctx->dy*ctx->others_time);
  }
  return 0;
}
 
static void fmn_wind_blow(uint8_t dir,float others_time,float hero_time) {
  struct fmn_wind_context ctx={
    .others_time=others_time,
    .hero_time=hero_time,
  };
  fmn_vector_from_dir(&ctx.dx,&ctx.dy,dir);
  ctx.dx*=FMN_WIND_RATE;
  ctx.dy*=FMN_WIND_RATE;
  fmn_sprites_for_each(fmn_wind_blow_1,&ctx);
}

static void fmn_wind_check_blowback(float others_time,float hero_time) {
  // Blowback begins when you cross the midpoint horizontally or vertically, if there is no neighbor in that direction.
  // Intensity increases as you approach the edge.
  const float BLOWBACK_RATE_MAX=20.0f;
  const float HALFW=FMN_COLC*0.5f;
  const float HALFH=FMN_ROWC*0.5f;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  struct fmn_wind_context ctx={
    .others_time=others_time,
    .hero_time=hero_time,
  };
  fmn_global.blowbackx=0.0f;
  fmn_global.blowbacky=0.0f;
  if (herox<HALFW) {
    if (!fmn_global.neighborw) fmn_global.blowbackx=(HALFW-herox)/HALFW;
  } else {
    if (!fmn_global.neighbore) fmn_global.blowbackx=(herox-HALFW)/-HALFW;
  }
  if (heroy<HALFH) {
    if (!fmn_global.neighborn) fmn_global.blowbacky=(HALFH-heroy)/HALFH;
  } else {
    if (!fmn_global.neighbors) fmn_global.blowbacky=(heroy-HALFH)/-HALFH;
  }
  if (
    (fmn_global.blowbackx<-0.001f)||
    (fmn_global.blowbackx> 0.001f)||
    (fmn_global.blowbacky<-0.001f)||
    (fmn_global.blowbacky> 0.001f)
  ) {
    ctx.dx=fmn_global.blowbackx*BLOWBACK_RATE_MAX;
    ctx.dy=fmn_global.blowbacky*BLOWBACK_RATE_MAX;
    fmn_sprites_for_each(fmn_wind_blow_1,&ctx);
  }
}
 
static float fmn_weather_update(float elapsed) {
  float adjusted=elapsed;
  if (fmn_global.slowmo_time>0.0f) {
    if ((fmn_global.slowmo_time-=elapsed)<=0.0f) {
      fmn_sound_effect(FMN_SFX_SLOWMO_END);
      fmn_global.slowmo_time=0.0f;
    } else {
      adjusted*=FMN_SLOWMO_RATE;
    }
  }
  if (fmn_global.rain_time>0.0f) {
    if ((fmn_global.rain_time-=elapsed)<=0.0f) {
      fmn_global.rain_time=0.0f;
    }
  }
  if (fmn_global.wind_time>0.0f) {
    if ((fmn_global.wind_time-=elapsed)<=0.0f) {
      fmn_global.wind_time=0.0f;
    } else {
      fmn_wind_blow(fmn_global.wind_dir,adjusted,elapsed);
    }
  }
  if (fmn_global.invisibility_time>0.0f) {
    if ((fmn_global.invisibility_time-=adjusted)<=0.0f) {
      fmn_sound_effect(FMN_SFX_INVISIBILITY_END);
      fmn_global.invisibility_time=0.0f;
    }
  }
  if (fmn_global.blowback) {
    fmn_wind_check_blowback(adjusted,elapsed);
  } else {
    fmn_global.blowbackx=0.0f;
    fmn_global.blowbacky=0.0f;
  }
  return adjusted;
}

/* Callback from Game Over menu.
 */
 
static void cb_game_over_ok() {
  fmn_global.hero_dead=0;
  fmn_global.werewolf_dead=0;
  fmn_hero_kill_velocity();
  fmn_game_load_map(1);
  fmn_map_callbacks(FMN_MAP_EVID_LOADED,fmn_game_map_callback,0);
}

/* Update.
 */
 
void fmn_game_update(float elapsed) {

  if ((fmn_global.illumination_time-=elapsed)<=0.0f) {
    fmn_global.illumination_time=0.0f;
  }
  if ((fmn_global.match_illumination_time-=elapsed)<=0.0f) {
    fmn_global.match_illumination_time=0.0f;
  }
  
  if (fmn_global.werewolf_dead||fmn_global.hero_dead) {
    if ((fmn_global.terminate_time-=elapsed)<=0.0f) {
      fmn_global.terminate_time=0.0f;
      if (fmn_global.hero_dead) fmn_begin_menu(FMN_MENU_GAMEOVER,1,cb_game_over_ok);
      else fmn_begin_menu(FMN_MENU_VICTORY);
      return;
    }
  }
  
  if (fmn_global.show_off_item_time) {
    uint8_t drop_time=elapsed * 128; // about 2 seconds total
    if (drop_time<1) drop_time=1;
    if (drop_time>=fmn_global.show_off_item_time) fmn_global.show_off_item_time=0;
    else fmn_global.show_off_item_time-=drop_time;
  }

  float weather_adjusted_time=fmn_weather_update(elapsed);
  fmn_sprites_update(weather_adjusted_time,elapsed);
  fmn_hero_update(elapsed);
  fmn_sprites_sort_partial();
  fmn_game_update_map_singletons(elapsed);

  int8_t x,y;
  if (fmn_hero_get_quantized_position(&x,&y)) {
    if (x<0) fmn_game_navigate(-1,0);
    else if (x>=FMN_COLC) fmn_game_navigate(1,0);
    else if (y<0) fmn_game_navigate(0,-1);
    else if (y>=FMN_ROWC) fmn_game_navigate(0,1);
    else {
      if (fmn_game_check_doors(x,y)) return;
      fmn_game_check_plant_collection(x,y);
    }
  }
}

/* Force all plants to bloom.
 */
 
static void fmn_bloom_plants() {
  uint8_t changed=0;
  struct fmn_plant *plant=fmn_global.plantv;
  uint8_t i=fmn_global.plantc;
  for (;i-->0;plant++) {
    if (plant->state==FMN_PLANT_STATE_GROW) {
      plant->state=FMN_PLANT_STATE_FLOWER;
      changed=1;
    }
  }
  if (changed) {
    fmn_sound_effect(FMN_SFX_BLOOM);
    fmn_map_dirty();
  }
}

/* Teleport to another map, or the same one.
 */
 
static void fmn_teleport(uint16_t mapid) {
  fmn_prepare_transition(FMN_TRANSITION_WARP);
  if (fmn_game_load_map(mapid)<1) {
    fmn_cancel_transition();
    return;
  }
  fmn_sound_effect(FMN_SFX_TELEPORT);
  int8_t dumx,dumy;
  fmn_hero_get_quantized_position(&dumx,&dumy);
  fmn_hero_kill_velocity();
  fmn_map_callbacks(FMN_MAP_EVID_LOADED,fmn_game_map_callback,0);
  fmn_commit_transition();
}

/* Weather control spells.
 * Yes, slow-motion is a weather phenomenon, you just haven't been listening close enough to the weather reports.
 */
 
static int fmn_rain_begin_1(struct fmn_sprite *sprite,void *dummy) {
  if (!sprite->interact) return 0;
  sprite->interact(sprite,FMN_ITEM_PITCHER,FMN_PITCHER_CONTENT_WATER);
  return 0;
}
 
static void fmn_rain_begin() {
  if (fmn_global.indoors) {
    //TODO repudiation?
    return;
  }
  fmn_sound_effect(FMN_SFX_RAIN);
  fmn_global.rain_time=FMN_RAIN_TIME;
  fmn_sprites_for_each(fmn_rain_begin_1,0);
  struct fmn_plant *plant=fmn_global.plantv;
  uint8_t i=fmn_global.plantc;
  uint8_t dirty=0;
  for (;i-->0;plant++) {
    if (plant->state!=FMN_PLANT_STATE_SEED) continue;
    fmn_sound_effect(FMN_SFX_SPROUT);
    plant->fruit=FMN_PITCHER_CONTENT_WATER;
    plant->state=FMN_PLANT_STATE_GROW;
    dirty=1;
  }
  if (dirty) fmn_map_dirty();
}
 
static void fmn_wind_begin(uint8_t dir) {
  if (fmn_global.indoors) {
    //TODO repudiation?
    return;
  }
  
  // If wind is already blowing, and she selected the opposite direction, cancel it.
  // This will be important for maps with initial wind, if we ever do that.
  if (fmn_global.wind_time>0.0f) {
    if (dir==fmn_dir_reverse(fmn_global.wind_dir)) {
      fmn_global.wind_time=0.0f;
      return;
    }
  }
  
  fmn_sound_effect(FMN_SFX_WIND);
  fmn_global.wind_dir=dir;
  fmn_global.wind_time=FMN_WIND_TIME;
}
 
static void fmn_slowmo_begin() {
  fmn_sound_effect(FMN_SFX_SLOWMO_BEGIN);
  fmn_global.slowmo_time=FMN_SLOWMO_TIME;
}

static void fmn_invisibility_begin() {
  fmn_sound_effect(FMN_SFX_INVISIBILITY_BEGIN);
  fmn_global.invisibility_time=FMN_INVISIBILITY_TIME;
}

/* Cast the Song of Revelations.
 */
 
static void fmn_cast_revelations() {
  uint8_t *v=fmn_global.map;
  uint8_t i=FMN_COLC*FMN_ROWC;
  uint8_t dirty=0;
  for (;i-->0;v++) {
    if (fmn_global.cellphysics[*v]==FMN_CELLPHYSICS_REVELABLE) {
      //TODO should there be some fireworks?
      (*v)+=0x10;
      dirty=1;
    }
  }
  if (dirty) fmn_map_dirty();
}

/* Cast spell or song.
 */
 
static int fmn_spell_cast_1(struct fmn_sprite *sprite,void *userdata) {
  uint8_t spellid=(uint8_t)(uintptr_t)userdata;
  if (!sprite->interact) return 0;
  sprite->interact(sprite,FMN_ITEM_WAND,spellid);
  return 0;
}
 
void fmn_spell_cast(uint8_t spellid) {
  //fmn_log("%s %d",__func__,spellid);
  fmn_sprites_for_each(fmn_spell_cast_1,(void*)(uintptr_t)spellid);
  switch (spellid) {
    case FMN_SPELLID_BLOOM: fmn_bloom_plants(); break;
    case FMN_SPELLID_RAIN: fmn_rain_begin(); break;
    case FMN_SPELLID_WIND_W: fmn_wind_begin(FMN_DIR_W); break;
    case FMN_SPELLID_WIND_E: fmn_wind_begin(FMN_DIR_E); break;
    case FMN_SPELLID_WIND_N: fmn_wind_begin(FMN_DIR_N); break;
    case FMN_SPELLID_WIND_S: fmn_wind_begin(FMN_DIR_S); break;
    case FMN_SPELLID_SLOWMO: fmn_slowmo_begin(); break;
    case FMN_SPELLID_INVISIBLE: fmn_invisibility_begin(); break;
    case FMN_SPELLID_REVELATIONS: fmn_cast_revelations(); break;
    case FMN_SPELLID_HOME: fmn_teleport(1); break;
    //TODO mapid for TELE(n) should be stored in the archive somehow.
    /*TODO TELE 1..4 disabled for demo
    case FMN_SPELLID_TELE1: fmn_teleport(6); break;
    case FMN_SPELLID_TELE2: fmn_teleport(13); break;
    case FMN_SPELLID_TELE3: fmn_teleport(11); break;
    case FMN_SPELLID_TELE4: fmn_teleport(21); break;
    /**/
  }
}

/* GS listeners.
 */
 
#define FMN_GS_LISTENER_LIMIT 32
 
static struct fmn_gs_listener {
  uint16_t id;
  uint16_t p;
  void (*cb)(void *userdata,uint16_t p,uint8_t v);
  void *userdata;
} fmn_gs_listenerv[FMN_GS_LISTENER_LIMIT];
static uint16_t fmn_gs_listenerc=0;
static uint16_t fmn_gs_listener_id_next=1; // if we create 64k listeners in one map, there may be collisions. unlikely.
 
uint16_t fmn_gs_listen_bit(uint16_t p,void (*cb)(void *userdata,uint16_t p,uint8_t v),void *userdata) {
  if (fmn_gs_listenerc>=FMN_GS_LISTENER_LIMIT) return 0;
  struct fmn_gs_listener *listener=fmn_gs_listenerv+fmn_gs_listenerc++;
  listener->id=fmn_gs_listener_id_next++;
  if (!fmn_gs_listener_id_next) fmn_gs_listener_id_next=1;
  listener->p=p;
  listener->cb=cb;
  listener->userdata=userdata;
  return listener->id;
}

void fmn_gs_unlisten(uint16_t id) {
  uint16_t i=fmn_gs_listenerc;
  while (i-->0) {
    if (fmn_gs_listenerv[i].id==id) {
      fmn_gs_listenerc--;
      memmove(fmn_gs_listenerv+i,fmn_gs_listenerv+i+1,sizeof(struct fmn_gs_listener)*(fmn_gs_listenerc-i));
      return;
    }
  }
}

void fmn_gs_drop_listeners() {
  fmn_gs_listenerc=0;
}

/* GS changes.
 */
 
void fmn_gs_notify(uint16_t p,uint16_t c) {
  if (c==1) { //TODO listening on multiple bits, i haven't thought it through yet
    uint8_t v=(fmn_global.gs[p>>3]&(0x80>>(p&7)))?1:0;
    uint16_t i=fmn_gs_listenerc;
    while (i-->0) {
      const struct fmn_gs_listener *listener=fmn_gs_listenerv+i;
      if (listener->p!=p) continue;
      listener->cb(listener->userdata,p,v);
    }
  }
}

uint8_t fmn_gs_get_bit(uint16_t p) {
  uint16_t bytep=p>>3;
  if (bytep>=FMN_GS_SIZE) return 0;
  uint8_t mask=0x80>>(p&7);
  return (fmn_global.gs[bytep]&mask)?1:0;
}

void fmn_gs_set_bit(uint16_t p,uint8_t v) {
  uint16_t bytep=p>>3;
  if (bytep>=FMN_GS_SIZE) return;
  uint8_t mask=0x80>>(p&7);
  if (v) {
    if (fmn_global.gs[bytep]&mask) return;
    fmn_global.gs[bytep]|=mask;
  } else {
    if (!(fmn_global.gs[bytep]&mask)) return;
    fmn_global.gs[bytep]&=~mask;
  }
  uint16_t i=fmn_gs_listenerc;
  while (i-->0) {
    const struct fmn_gs_listener *listener=fmn_gs_listenerv+i;
    if (listener->p!=p) continue;
    listener->cb(listener->userdata,p,v);
  }
}

/* Game events.
 */
 
#define FMN_GAME_EVENT_LISTENER_LIMIT 32
static struct fmn_game_event_listener {
  uint16_t id;
  uint16_t eventid;
  void (*cb)(void *userdata,uint16_t eventid,void *payload);
  void *userdata;
} fmn_game_event_listenerv[FMN_GAME_EVENT_LISTENER_LIMIT];
static uint8_t fmn_game_event_listenerc=0;
static uint16_t fmn_game_event_listener_next_id=1;

uint16_t fmn_game_event_listen(uint16_t eventid,void (*cb)(void *userdata,uint16_t eventid,void *payload),void *userdata) {
  if (!cb||!eventid) return 0;
  if (fmn_game_event_listenerc>=FMN_GAME_EVENT_LISTENER_LIMIT) return 0;
  struct fmn_game_event_listener *listener=fmn_game_event_listenerv+fmn_game_event_listenerc++;
  listener->id=fmn_game_event_listener_next_id++;
  if (!fmn_game_event_listener_next_id) fmn_game_event_listener_next_id=1;
  listener->eventid=eventid;
  listener->cb=cb;
  listener->userdata=userdata;
  return listener->id;
}

void fmn_game_event_unlisten(uint16_t id) {
  uint8_t i=fmn_game_event_listenerc;
  while (i-->0) {
    struct fmn_game_event_listener *listener=fmn_game_event_listenerv+i;
    if (listener->id==id) {
      fmn_game_event_listenerc--;
      memmove(listener,listener+1,sizeof(struct fmn_game_event_listener)*(fmn_game_event_listenerc-i));
      return;
    }
  }
}

void fmn_game_event_broadcast(uint16_t eventid,void *payload) {
  struct fmn_game_event_listener *v=fmn_game_event_listenerv;
  uint8_t i=fmn_game_event_listenerc;
  for (;i-->0;v++) {
    if (v->eventid!=eventid) continue;
    v->cb(v->userdata,eventid,payload);
  }
}

void fmn_game_event_unlisten_all() {
  fmn_game_event_listenerc=0;
}

/* Map singletons.
 */
 
#define FMN_MAP_SINGLETON_LIMIT 16
static struct fmn_map_singleton {
  uint16_t id;
  void *discriminator;
  void *userdata;
  void (*cb_update)(void *userdata,float elapsed);
  void (*cb_cleanup)(void *userdata);
} fmn_map_singletonv[FMN_MAP_SINGLETON_LIMIT];
static uint8_t fmn_map_singletonc=0;
static uint16_t fmn_map_singleton_next_id=1;

uint16_t fmn_game_register_map_singleton(
  void *discriminator,
  void (*cb_update)(void *userdata,float elapsed),
  void (*cb_cleanup)(void *userdata),
  void *userdata
) {
  if (discriminator) {
    const struct fmn_map_singleton *v=fmn_map_singletonv;
    uint8_t i=fmn_map_singletonc;
    for (;i-->0;v++) {
      if (v->discriminator==discriminator) return 0;
    }
  }
  if (fmn_map_singletonc>=FMN_MAP_SINGLETON_LIMIT) return 0;
  struct fmn_map_singleton *singleton=fmn_map_singletonv+fmn_map_singletonc++;
  singleton->id=fmn_map_singleton_next_id++;
  if (!fmn_map_singleton_next_id) fmn_map_singleton_next_id=1;
  singleton->discriminator=discriminator;
  singleton->userdata=userdata;
  singleton->cb_update=cb_update;
  singleton->cb_cleanup=cb_cleanup;
  return singleton->id;
}

void fmn_game_unregister_map_singleton(uint16_t id) {
  if (!id) return;
  uint8_t i=fmn_map_singletonc;
  while (i-->0) {
    if (fmn_map_singletonv[i].id==id) {
      fmn_map_singletonc--;
      memmove(fmn_map_singletonv+i,fmn_map_singletonv+i+1,sizeof(struct fmn_map_singleton)*(fmn_map_singletonc-i));
      return;
    }
  }
}

void fmn_game_drop_map_singletons() {
  struct fmn_map_singleton *v=fmn_map_singletonv;
  uint8_t i=fmn_map_singletonc;
  for (;i-->0;v++) {
    if (v->cb_cleanup) v->cb_cleanup(v->userdata);
  }
  fmn_map_singletonc=0;
}

void fmn_game_update_map_singletons(float elapsed) {
  struct fmn_map_singleton *v=fmn_map_singletonv;
  uint8_t i=fmn_map_singletonc;
  for (;i-->0;v++) {
    if (v->cb_update) v->cb_update(v->userdata,elapsed);
  }
}

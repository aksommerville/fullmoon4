#include "fmn_eatwq_internal.h"

struct eatwq eatwq={0};

static void eatwq_cb_dummy(uint8_t v,void *userdata) {}

/* Begin.
 */
 
void fmn_eatwq_init(
  uint8_t creditc,uint8_t hiscore,
  void (*cb_creditc)(uint8_t creditc,void *userdata),
  void (*cb_hiscore)(uint8_t hiscore,void *userdata),
  void *userdata
) {
  eatwq.creditc=creditc;
  eatwq.hiscore=hiscore;
  if (cb_creditc) eatwq.cb_creditc=cb_creditc;
  else eatwq.cb_creditc=eatwq_cb_dummy;
  if (cb_hiscore) eatwq.cb_hiscore=cb_hiscore;
  else eatwq.cb_hiscore=eatwq_cb_dummy;
  eatwq.userdata=userdata;
  eatwq.running=0;
}

/* Switch from hello to running state.
 */
 
static void eatwq_begin_play() {
  if (!eatwq.creditc) return;
  eatwq.creditc--;
  eatwq.cb_creditc(eatwq.creditc,eatwq.userdata);
  eatwq.running=1;
  eatwq.umbrella=0;
  eatwq.herox=EATWQ_FB_W>>1;
  eatwq.heroy=EATWQ_FB_H>>1;
  eatwq.dead=0;
  eatwq.playtime=60*30;
  eatwq.dropc=0;
  eatwq.plantc=0;
  eatwq.boomc=0;
  eatwq.score=0;
  eatwq.summaryttl=120; // delay after the last flower blasts off in summary, before ending for real
}

/* Time up: Switch from running to hello state.
 */
 
static void eatwq_end_play() {
  eatwq.running=0;
}

/* Kill hero.
 */
 
static void eatwq_die() {
  eatwq.dead=1;
}

/* Create an explosion.
 */
 
static void eatwq_explode(int16_t x,int16_t y) {
  if (eatwq.boomc>=EATWQ_BOOM_LIMIT) return;
  struct eatwq_boom *boom=eatwq.boomv+eatwq.boomc++;
  boom->x=x;
  boom->y=y;
  boom->ttl=60;
  fmn_sound_effect(FMN_SFX_BREAK_BONES);
  
  // Kill plants.
  int poisoned=0;
  const int16_t radius=6;
  int i=eatwq.plantc;
  struct eatwq_plant *plant=eatwq.plantv+i-1;
  for (;i-->0;plant--) {
    if (plant->tileid==0xe2) { // poison; not explodable
      poisoned=1;
      continue;
    }
    if (plant->x<x-radius) continue;
    if (plant->x>x+radius) continue;
    if (plant->y<y-radius) continue;
    if (plant->y>y+radius) continue;
    eatwq.plantc--;
    memmove(plant,plant+1,sizeof(struct eatwq_plant)*(eatwq.plantc-i));
  }
  
  // Kill hero?
  if (
    (eatwq.herox>=x-radius)&&
    (eatwq.herox<=x+radius)&&
    (eatwq.heroy>=y-radius)&&
    (eatwq.heroy<=y+radius)
  ) {
    eatwq_die();
  }
  
  // Poison earth?
  if ((y>=EATWQ_FB_H-12)&&(x>=0)&&(x<EATWQ_FB_W)&&!poisoned) {
    if (eatwq.plantc<EATWQ_PLANT_LIMIT) {
      plant=eatwq.plantv+eatwq.plantc++;
      plant->x=x;
      plant->y=EATWQ_FB_H-4;
      plant->tileid=0xe2;
    }
  }
}

/* Seed landed on ground. Create a sprout if possible here.
 */
 
static void eatwq_drop_seed(int16_t x) {
  if (eatwq.plantc>=EATWQ_PLANT_LIMIT) return;
  
  // Require at least 4 pixels clearance from existing plants.
  struct eatwq_plant *plant=eatwq.plantv;
  int i=eatwq.plantc;
  for (;i-->0;plant++) {
    if (plant->x<=x-4) continue;
    if (plant->x>=x+4) continue;
    return;
  }
  
  fmn_sound_effect(FMN_SFX_PLANT);
  plant=eatwq.plantv+eatwq.plantc++;
  plant->x=x;
  plant->y=EATWQ_FB_H-12;
  plant->tileid=0xd0;
}

/* Rain landed on ground. Bloom plants if possible here.
 */
 
static void eatwq_drop_rain(int16_t x) {
  struct eatwq_plant *plant=eatwq.plantv;
  int i=eatwq.plantc;
  for (;i-->0;plant++) {
    if (plant->tileid!=0xd0) continue;
    if (plant->x<x-4) continue;
    if (plant->x>x+4) continue;
    plant->tileid=0xd1+(rand()&3);
    fmn_sound_effect(FMN_SFX_BLOOM);
  }
}

/* Move the hero.
 */
 
static void eatwq_update_hero_motion() {
  if (eatwq.dx) {
    eatwq.herox+=eatwq.dx;
    if (eatwq.herox<0) eatwq.herox=0;
    else if (eatwq.herox>=EATWQ_FB_W) eatwq.herox=EATWQ_FB_W-1;
  }
  if (eatwq.dy) {
    eatwq.heroy+=eatwq.dy;
    if (eatwq.heroy<0) eatwq.heroy=0;
    else if (eatwq.heroy>=EATWQ_FB_H-10) eatwq.heroy=EATWQ_FB_H-11;
  }
}

/* Hero is dead. If she's not at the bottom yet, fall a little.
 */
 
static void eatwq_update_hero_fall() {
  if (eatwq.playtime&1) return;
  if (eatwq.heroy<EATWQ_FB_H-12) eatwq.heroy++;
  else eatwq.heroy=EATWQ_FB_H-12;
}

/* If this drop collides with the umbrella, bonk it a little.
 */
 
static void eatwq_check_bounce(struct eatwq_drop *drop) {
  if (drop->y<eatwq.heroy-6) return;
  if (drop->y>eatwq.heroy+2) return;
  if (eatwq.flop) {
    if (drop->x<eatwq.herox) return;
    if (drop->x>eatwq.herox+8) return;
    drop->x+=8;
  } else {
    if (drop->x<eatwq.herox-8) return;
    if (drop->x>eatwq.herox) return;
    drop->x-=8;
  }
  drop->y-=6;
  fmn_sound_effect(FMN_SFX_INJURY_DEFLECTED);
}

/* If this drop collides with the hero, create an explosion and remove the drop.
 * (this will implicitly kill the hero).
 */
 
static void eatwq_check_hero_bomb(struct eatwq_drop *drop) {
  if (drop->x<eatwq.herox-4) return;
  if (drop->x>eatwq.herox+4) return;
  if (drop->y<eatwq.heroy-4) return;
  if (drop->y>eatwq.heroy+4) return;
  eatwq_explode(drop->x,drop->y);
  int p=drop-eatwq.dropv;
  if ((p>=0)&&(p<eatwq.dropc)) {
    eatwq.dropc--;
    memmove(drop,drop+1,sizeof(struct eatwq_drop)*(eatwq.dropc-p));
  }
}

/* Create new raindrop or bombs, and advance motion of existing ones.
 */
 
static void eatwq_update_drops() {
  
  // New thing?
  if (!eatwq.dead&&(eatwq.dropc<EATWQ_DROP_LIMIT)&&!(eatwq.playtime%20)) {
    struct eatwq_drop *drop=eatwq.dropv+eatwq.dropc++;
    drop->x=((rand()%(EATWQ_FB_W-8))&~7)+4;
    drop->y=-4;
    switch (rand()%10) {
      case 0: drop->tileid=0xc3; break; // bomb
      case 1: drop->tileid=0xe4; break; // seed
      case 2: drop->tileid=0xe4; break; // seed
      case 3: drop->tileid=0x0e4; break; // seed
      default: drop->tileid=0xc2; // rain
    }
  }
  
  // Advance all existing drops.
  if (eatwq.playtime&1) return; // skip every other frame
  int animate_bombs=!(eatwq.playtime&0x03);
  int i=eatwq.dropc;
  struct eatwq_drop *drop=eatwq.dropv+i-1;
  for (;i-->0;drop--) {
    drop->y++;
    if (animate_bombs) switch (drop->tileid) {
      case 0xc3: drop->tileid=0xc4; break;
      case 0xc4: drop->tileid=0xc3; break;
    }
    if (drop->y>EATWQ_FB_H-10) {
      switch (drop->tileid) {
        case 0xc3: case 0xc4: eatwq_explode(drop->x,drop->y); break;
        case 0xe4: eatwq_drop_seed(drop->x); break;
        case 0xc2: eatwq_drop_rain(drop->x); break;
      }
      eatwq.dropc--;
      memmove(drop,drop+1,sizeof(struct eatwq_drop)*(eatwq.dropc-i));
    } else if (eatwq.umbrella) {
      eatwq_check_bounce(drop);
    } else if ((drop->tileid==0xc3)||(drop->tileid==0xc4)) {
      eatwq_check_hero_bomb(drop);
    }
  }
}

/* Check booms.
 */
 
static void eatwq_update_booms() {
  int i=eatwq.boomc;
  struct eatwq_boom *boom=eatwq.boomv+i-1;
  for (;i-->0;boom--) {
    if (boom->ttl) boom->ttl--;
    else {
      eatwq.boomc--;
      memmove(boom,boom+1,sizeof(struct eatwq_boom)*(eatwq.boomc-i));
    }
  }
}

/* Update post-game summary.
 */
 
static void eatwq_summary_update() {
  
  struct eatwq_plant *plant=eatwq.plantv;
  int i=eatwq.plantc;
  for (;i-->0;plant++) {
  
    // Only interested in flowers: 0xd1..0xd4
    if (plant->tileid<0xd1) continue;
    if (plant->tileid>0xd4) continue;
    
    // If it's above the screen, ignore it.
    if (plant->y<=-4) continue;
    
    // If it's off the ground, continue launch.
    if (plant->y<EATWQ_FB_H-12) {
      plant->y-=2;
      return;
    }
    
    // Add a point and initiate liftoff.
    fmn_sound_effect(FMN_SFX_TOSS);
    eatwq.score++;
    if (eatwq.score>eatwq.hiscore) {
      eatwq.hiscore=eatwq.score;
      eatwq.cb_hiscore(eatwq.hiscore,eatwq.userdata);
    }
    plant->y=EATWQ_FB_H-14;
    return;
  }
  
  if (eatwq.summaryttl) eatwq.summaryttl--;
  else eatwq_end_play();
}

/* Update.
 */
 
void fmn_eatwq_update(uint8_t input,uint8_t pvinput) {
  if (input!=pvinput) {
  
    if ((input&FMN_INPUT_USE)&&!(pvinput&FMN_INPUT_USE)) {
      if (eatwq.running) eatwq.umbrella=1;
      else eatwq_begin_play();
    } if (!(input&FMN_INPUT_USE)&&(pvinput&FMN_INPUT_USE)) {
      eatwq.umbrella=0;
    }
    
    switch (input&(FMN_INPUT_LEFT|FMN_INPUT_RIGHT)) {
      case FMN_INPUT_LEFT: eatwq.dx=-1; eatwq.flop=0; break;
      case FMN_INPUT_RIGHT: eatwq.dx=1; eatwq.flop=1; break;
      default: eatwq.dx=0; break;
    }
    switch (input&(FMN_INPUT_UP|FMN_INPUT_DOWN)) {
      case FMN_INPUT_UP: eatwq.dy=-1; break;
      case FMN_INPUT_DOWN: eatwq.dy=1; break;
      default: eatwq.dy=0; break;
    }
  }
  if (eatwq.running) {
    if (eatwq.dead) {
      eatwq_update_hero_fall();
      if (!eatwq.dropc) eatwq_summary_update();
    } else {
      if (!eatwq.playtime) eatwq_die();
      else eatwq.playtime--;
      eatwq_update_hero_motion();
    }
    eatwq_update_drops();
    eatwq_update_booms();
  }
}

/* Trivial accessors.
 */
 
int fmn_eatwq_is_running() {
  return eatwq.running;
}

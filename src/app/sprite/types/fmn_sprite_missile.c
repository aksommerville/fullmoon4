/* fmn_sprite_missile.c
 * A general-purpose sprite that locates the hero once, and travels in that direction until collision or offscreen.
 * You can set pv[0] to another sprite, and while set we will position ourselves just above that sprite and not track the hero.
 */
 
#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define MISSILE_HIT_DISTANCE 0.6f

#define holder ((struct fmn_sprite*)sprite->pv[0])
#define speed sprite->fv[0]
#define dx sprite->fv[1]
#define dy sprite->fv[2]
#define ready sprite->bv[0]
#define reflected sprite->bv[1]

/* Init.
 */
 
static void _missile_init(struct fmn_sprite *sprite) {
  speed=6.0f;
  sprite->layer=140;
  sprite->radius=0.5f;
}

/* Target the hero.
 */
 
static void missile_calculate_trajectory(struct fmn_sprite *sprite) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float hdx=herox-sprite->x;
  float hdy=heroy-sprite->y;
  float distance=sqrtf(hdx*hdx+hdy*hdy);
  if (distance<0.1f) { hdy=1.0f; distance=1.0f; }
  dx=(hdx*speed)/distance;
  dy=(hdy*speed)/distance;
  ready=1;
}

/* Check collisions.
 */
 
static void missile_check_collision(struct fmn_sprite *sprite) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float hdx=herox-sprite->x;
  if ((hdx>MISSILE_HIT_DISTANCE)||(hdx<-MISSILE_HIT_DISTANCE)) return;
  float hdy=heroy-sprite->y;
  if ((hdy>MISSILE_HIT_DISTANCE)||(hdy<-MISSILE_HIT_DISTANCE)) return;
  float distance=sqrtf(hdx*hdx+hdy*hdy);
  if (distance>MISSILE_HIT_DISTANCE) return;
  if (fmn_hero_injure(sprite->x,sprite->y,sprite)) {
    fmn_sprite_kill(sprite);
  }
}

/* Update.
 */
 
static void _missile_update(struct fmn_sprite *sprite,float elapsed) {
  if (holder) {
    sprite->x=holder->x;
    sprite->y=holder->y-0.5f;
  } else {
    if (!ready) missile_calculate_trajectory(sprite);
    sprite->x+=dx*elapsed;
    sprite->y+=dy*elapsed;
    if ((sprite->x<-1.0f)||(sprite->y<-1.0f)||(sprite->x>FMN_COLC+1.0f)||(sprite->y>FMN_ROWC+1.0f)) {
      fmn_game_event_broadcast(FMN_GAME_EVENT_MISSILE_OOB,sprite);
      fmn_sprite_kill(sprite);
      return;
    } else {
      missile_check_collision(sprite);
    }
  }
}

/* Reflect off the umbrella.
 * (adj>0) to adjust according to position.
 * (adj<0) to blindly reflect.
 * (!adj) to leave that axis untouched.
 */
 
static void missile_reflect_umbrella(struct fmn_sprite *sprite,int8_t adjx,int8_t adjy) {
  if (adjx<0) dx=-dx;
  if (adjy<0) dy=-dy;
  float xrrange=0.0f,yrrange=0.0f;
  float reft;
  if (adjx>0) {
    if (dy<0.0f) {
      xrrange=1.0f;
      reft=0.0f;
    } else {
      xrrange=-1.0f;
      reft=M_PI;
    }
  } else if (adjy>0) {
    if (dx<0.0f) {
      yrrange=-1.0f;
      reft=M_PI*-0.5f;
    } else {
      yrrange=1.0f;
      reft=M_PI*0.5f;
    }
  } else {
    return; // simple adjustment only; we're done
  }
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  heroy-=0.2f; // cheat her up a little; her center is near her feet
  const float radius=0.5f;
  float xn=(sprite->x-herox)/radius; if (xn<-1.0f) xn=-1.0f; else if (xn>1.0f) xn=1.0f;
  float yn=(sprite->y-heroy)/radius; if (yn<-1.0f) yn=-1.0f; else if (yn>1.0f) yn=1.0f;
  const float arc_range=0.3f;
  float relt=(xn*xrrange+yn*yrrange)*M_PI*arc_range;
  float t=reft+relt;
  dx=sinf(t)*speed;
  dy=-cosf(t)*speed;
  reflected=1;
}

/* Interact.
 */
 
static int16_t _missile_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_UMBRELLA: switch (qualifier) {
        case FMN_DIR_N: case FMN_DIR_S: missile_reflect_umbrella(sprite,1,-1); break;
        case FMN_DIR_W: case FMN_DIR_E: missile_reflect_umbrella(sprite,-1,1); break;
        default: fmn_sprite_kill(sprite);
      } break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_missile={
  .init=_missile_init,
  .update=_missile_update,
  .interact=_missile_interact,
};

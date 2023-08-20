#include "app/sprite/fmn_sprite.h"
#include "app/sprite/fmn_physics.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

// Our behavior is known from (tileid). It's cool, our tile never changes.
#define AB_MODE_ALPHA 0x01
#define AB_MODE_GAMMA 0x02
#define AB_MODE_LAMBDA 0x03
#define AB_MODE_MU 0x04

#define AB_SPEED 1.0f
#define AB_LAMBDA_SLIDE_TIME 0.150f
#define AB_LAMBDA_STOP_DISTANCE 1.0f

// The first 5 of bv are a list of recent contact directions, [0] is oldest.
#define AB_CONTACT_SIZE 5
#define last_contact sprite->bv[5]
#define lambda_attract sprite->bv[6]

#define dx sprite->fv[0]
#define dy sprite->fv[1]
#define motion_clock sprite->fv[2]

/* Init.
 */
 
static void _alphablock_init(struct fmn_sprite *sprite) {
}

/* Shadow for lambda block.
 */
 
struct fmn_alphablock_find_shadow_context {
  struct fmn_sprite *sprite;
  struct fmn_sprite *shadow;
};
 
static int fmn_alphablock_find_shadow_cb(struct fmn_sprite *q,void *userdata) {
  struct fmn_alphablock_find_shadow_context *ctx=userdata;
  if (q->tileid!=0xf0) return 0;
  if (q->layer!=1) return 0;
  if (q->imageid!=ctx->sprite->imageid) return 0;
  float x=q->x-ctx->sprite->x;
  if ((x<-0.5f)||(x>0.5f)) return 0;
  float y=q->y-ctx->sprite->y;
  if ((y<-0.5f)||(y>0.5f)) return 0;
  ctx->shadow=q;
  return 1;
}
 
static struct fmn_sprite *fmn_alphablock_find_shadow(struct fmn_sprite *sprite) {
  struct fmn_alphablock_find_shadow_context ctx={.sprite=sprite};
  fmn_sprites_for_each(fmn_alphablock_find_shadow_cb,&ctx);
  return ctx.shadow;
}
 
static void fmn_alphablock_create_shadow(struct fmn_sprite *sprite) {
  float offset=1.0f/16.0f;
  sprite->y-=offset;
  sprite->hbn=sprite->radius-offset;
  sprite->hbs=sprite->radius+offset;
  sprite->hbw=sprite->radius;
  sprite->hbe=sprite->radius;
  sprite->radius=0.0f;
  struct fmn_sprite *shadow=fmn_sprite_generate_noparam(0,sprite->x,sprite->y+offset);
  if (shadow) {
    shadow->imageid=sprite->imageid;
    shadow->tileid=0xf0;
    shadow->layer=1;
    fmn_sprites_sort_partial(); // the shadow will always be early in the list, but generally gets created near the end of it.
  }
}

static void fmn_alphablock_destroy_shadow(struct fmn_sprite *sprite) {
  sprite->y+=1.0f/16.0f;
  sprite->radius=sprite->hbw;
  sprite->hbn=sprite->hbs=sprite->hbw=sprite->hbe=0.0f;
  struct fmn_sprite *shadow=fmn_alphablock_find_shadow(sprite);
  if (shadow) {
    fmn_sprite_kill(shadow);
  }
}

/* Lambda.
 * Check for summoning.
 */
 
static uint8_t fmn_alphablock_lambda_should_attract(struct fmn_sprite *sprite,float herox,float heroy) {
  const float radius=0.4f;
  dx=dy=0.0f;
  if (fmn_global.active_item!=FMN_ITEM_FEATHER) return 0;
  switch (fmn_global.facedir) {
    case FMN_DIR_N: if ((herox<sprite->x-radius)||(herox>sprite->x+radius)||(heroy<sprite->y)) return 0; dy=1.0f; break;
    case FMN_DIR_S: if ((herox<sprite->x-radius)||(herox>sprite->x+radius)||(heroy>sprite->y)) return 0; dy=-1.0f; break;
    case FMN_DIR_W: if ((heroy<sprite->y-radius)||(heroy>sprite->y+radius)||(herox<sprite->x)) return 0; dx=1.0f; break;
    case FMN_DIR_E: if ((heroy<sprite->y-radius)||(heroy>sprite->y+radius)||(herox>sprite->x)) return 0; dx=-1.0f; break;
    default: return 0;
  }
  return 1;
}
 
static void fmn_alphablock_check_lambda(struct fmn_sprite *sprite,float elapsed) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (fmn_alphablock_lambda_should_attract(sprite,herox,heroy)) {
    if (!lambda_attract) {
      fmn_sprite_generate_enchantment(sprite,1);
      lambda_attract=1;
      fmn_alphablock_create_shadow(sprite);
    }
    struct fmn_sprite *shadow=fmn_alphablock_find_shadow(sprite);
    if (shadow) {
      shadow->x=sprite->x;
      shadow->y=sprite->y+1.0f/16.0f;
    }
  } else {
    if (lambda_attract) {
      fmn_sprite_kill_enchantment(sprite);
      lambda_attract=0;
      fmn_alphablock_destroy_shadow(sprite);
      return;
    }
  }
  float distance=(herox-sprite->x)*dx+(heroy-sprite->y)*dy;
  if (distance<AB_LAMBDA_STOP_DISTANCE) {
    dx=dy=0.0f;
    motion_clock=0.0f;
    return;
  }
  if (motion_clock<=AB_LAMBDA_SLIDE_TIME) {
    motion_clock=AB_LAMBDA_SLIDE_TIME;
  }
}

/* Something was just added to the contact history.
 * Check whether we should activate.
 */
 
static char _(uint8_t src) {
  switch (src) {
    case FMN_DIR_N: return 'N';
    case FMN_DIR_S: return 'S';
    case FMN_DIR_W: return 'W';
    case FMN_DIR_E: return 'E';
    default: return ' ';
  }
}

static void fmn_alphablock_move(struct fmn_sprite *sprite,float ddx,float ddy) {
  dx=ddx;
  dy=ddy;
  motion_clock=1.0f/AB_SPEED;
  memset(sprite->bv,0,AB_CONTACT_SIZE);
  fmn_sound_effect(FMN_SFX_PUSH);
}
 
static void fmn_alphablock_check_contact_history(struct fmn_sprite *sprite) {
  switch (sprite->tileid) {
    case AB_MODE_ALPHA: {
        const uint8_t *B=sprite->bv+1;
             if ((B[0]==FMN_DIR_N)&&(B[1]==FMN_DIR_E)&&(B[2]==FMN_DIR_S)&&(B[3]==FMN_DIR_W)) fmn_alphablock_move(sprite,1.0f,0.0f);
        else if ((B[0]==FMN_DIR_E)&&(B[1]==FMN_DIR_S)&&(B[2]==FMN_DIR_W)&&(B[3]==FMN_DIR_N)) fmn_alphablock_move(sprite,0.0f,1.0f);
        else if ((B[0]==FMN_DIR_S)&&(B[1]==FMN_DIR_W)&&(B[2]==FMN_DIR_N)&&(B[3]==FMN_DIR_E)) fmn_alphablock_move(sprite,-1.0f,0.0f);
        else if ((B[0]==FMN_DIR_W)&&(B[1]==FMN_DIR_N)&&(B[2]==FMN_DIR_E)&&(B[3]==FMN_DIR_S)) fmn_alphablock_move(sprite,0.0f,-1.0f);
      } break;
    case AB_MODE_GAMMA: {
        if (
          (sprite->bv[2]==FMN_DIR_W)&&
          (sprite->bv[3]==FMN_DIR_E)&&
          (sprite->bv[4]==FMN_DIR_W)
        ) {
          fmn_sound_effect(FMN_SFX_BLOCK_EXPLODE);
          fmn_sprite_generate_soulballs(sprite->x,sprite->y,5,0);
          fmn_sprite_kill(sprite);
          fmn_game_event_broadcast(FMN_GAME_EVENT_BLOCKS_MOVED,0);
        }
      } break;
    case AB_MODE_MU: {
        if (
          (sprite->bv[0]==sprite->bv[1])&&
          (sprite->bv[1]==sprite->bv[2])&&
          (sprite->bv[2]!=sprite->bv[3])&&
          (sprite->bv[2]==sprite->bv[4])
        ) {
          switch (sprite->bv[0]) {
            case FMN_DIR_N: fmn_alphablock_move(sprite,0.0f,1.0f); break;
            case FMN_DIR_S: fmn_alphablock_move(sprite,0.0f,-1.0f); break;
            case FMN_DIR_W: fmn_alphablock_move(sprite,1.0f,0.0f); break;
            case FMN_DIR_E: fmn_alphablock_move(sprite,-1.0f,0.0f); break;
          }
        }
      } break;
  }
}

/* Alpha, gamma, or mu. Feather being used.
 * Check for edge encoding.
 */
 
static uint8_t fmn_alphablock_find_contact(const struct fmn_sprite *sprite) {
  if (fmn_global.active_item!=FMN_ITEM_FEATHER) return 0;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float facedx,facedy;
  fmn_vector_from_dir(&facedx,&facedy,fmn_global.facedir);
  facedx*=0.5f;
  facedy*=0.5f;
  if (sprite->x<herox+facedx-0.5f) return 0;
  if (sprite->x>herox+facedx+0.5f) return 0;
  if (sprite->y<heroy+facedy-0.5f) return 0;
  if (sprite->y>heroy+facedy+0.5f) return 0;
  return fmn_dir_reverse(fmn_global.facedir);
}
 
static void fmn_alphablock_check_edge_encoding(struct fmn_sprite *sprite) {
  uint8_t contact=fmn_alphablock_find_contact(sprite);
  if (contact==last_contact) return;
  last_contact=contact;
  if (contact) {
    memmove(sprite->bv,sprite->bv+1,AB_CONTACT_SIZE-1);
    sprite->bv[AB_CONTACT_SIZE-1]=contact;
    fmn_alphablock_check_contact_history(sprite);
  }
}

/* Update.
 */

static void _alphablock_update(struct fmn_sprite *sprite,float elapsed) {

  if (motion_clock>0.0f) {
    float motion_time=motion_clock;
    if (motion_time>elapsed) motion_time=elapsed;
    motion_clock-=motion_time;
    sprite->x+=AB_SPEED*dx*motion_time;
    sprite->y+=AB_SPEED*dy*motion_time;
    fmn_game_event_broadcast(FMN_GAME_EVENT_BLOCKS_MOVED,sprite);
  }

  if (sprite->tileid==AB_MODE_LAMBDA) {
    fmn_alphablock_check_lambda(sprite,elapsed);
  } else {
    fmn_alphablock_check_edge_encoding(sprite);
  }
}

/* Interact.
 */
 
static int16_t _alphablock_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  if ((sprite->tileid==AB_MODE_MU)&&(itemid==FMN_ITEM_PITCHER)&&(qualifier==FMN_PITCHER_CONTENT_MILK)) {
    fmn_sound_effect(FMN_SFX_MOO);
    return 1;
  }
  return 0;
}

/* Type definition.
 */

const struct fmn_sprite_controller fmn_sprite_controller_alphablock={
  .init=_alphablock_init,
  .update=_alphablock_update,
  .interact=_alphablock_interact,
};

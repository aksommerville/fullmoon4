#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

/* argv[0] is the Game ID, what score are we tracking?
 * Rules and such for scoreboardable games are all implemented right here.
 */
#define SCOREBOARD_GAMEID_NONE 0
#define SCOREBOARD_GAMEID_SEAMONSTER_PONG 1

#define SCOREBOARD_BLINK_TIME        0.300f
#define SCOREBOARD_ILLUMINATION_TIME 0.150f /* How much of BLINK_TIME to spend illuminated. */
#define SCOREBOARD_BLINK_DURATION    1.700f /* Total duration of the blink effect. */

#define tileid0 sprite->bv[0]
#define score sprite->bv[1] /* signed -3..3 */
#define running sprite->bv[2]
#define indicator sprite->bv[3] /* signed -2=solid red, -1=blink red, 0=off, 1=blink green, 2=solid green */
#define indicator_clock sprite->fv[0]
#define blink_duration sprite->fv[1]
#define listenerid sprite->sv[0]
#define gameid sprite->argv[0]

/* Generate my child sprites left and right.
 * These are managed entirely by the parent.
 */
 
static void scoreboard_generate_children(struct fmn_sprite *sprite) {
  struct fmn_sprite *child;
  if (child=fmn_sprite_generate_noparam(0,sprite->x-1.0f,sprite->y)) {
    child->imageid=sprite->imageid;
    child->tileid=tileid0+3;
    child->xform=FMN_XFORM_XREV;
    child->style=FMN_SPRITE_STYLE_TILE;
    child->layer=sprite->layer;
  }
  if (child=fmn_sprite_generate_noparam(0,sprite->x+1.0f,sprite->y)) {
    child->imageid=sprite->imageid;
    child->tileid=tileid0+3;
    child->xform=0;
    child->style=FMN_SPRITE_STYLE_TILE;
    child->layer=sprite->layer;
  }
}

/* Add a point and trigger all necessary events.
 */
 
static int fmn_scoreboard_check_child(struct fmn_sprite *q,void *userdata) {
  struct fmn_sprite *sprite=userdata;
  if (q->controller) return 0;
  if (q->imageid!=sprite->imageid) return 0;
  float dy=q->y-sprite->y;
  if ((dy<-0.5f)||(dy>0.5f)) return 0;
  float dx=q->x-sprite->x;
  if ((dx>=-1.5f)&&(dx<=-0.5f)) {
    // left child
    switch ((int8_t)score) {
      case -3: q->tileid=tileid0+6; break;
      case -2: q->tileid=tileid0+5; break;
      case -1: q->tileid=tileid0+4; break;
      default: q->tileid=tileid0+3; break;
    }
  } else if ((dx>=0.5f)&&(dx<=1.5f)) {
    // right child
    switch ((int8_t)score) {
      case 3: q->tileid=tileid0+6; break;
      case 2: q->tileid=tileid0+5; break;
      case 1: q->tileid=tileid0+4; break;
      default: q->tileid=tileid0+3; break;
    }
  }
  return 0;
}
 
static void fmn_scoreboard_score(struct fmn_sprite *sprite,int8_t d) {
  int8_t nscore=score+d;
  if (nscore<-3) nscore=-3;
  else if (nscore>3) nscore=3;
  if (nscore==(int8_t)score) return;
  score=nscore;
  
  fmn_sprites_for_each(fmn_scoreboard_check_child,sprite);
  
  if (nscore<=-3) {
    running=0;
    indicator=-2;
    fmn_game_event_broadcast(FMN_GAME_EVENT_SCOREBOARD_LOSE,sprite);
  } else if (nscore>=3) {
    running=0;
    indicator=2;
    fmn_game_event_broadcast(FMN_GAME_EVENT_SCOREBOARD_WIN,sprite);
  } else if (d<0) {
    indicator=-1;
    indicator_clock=0.0f;
    blink_duration=SCOREBOARD_BLINK_DURATION;
  } else if (d>0) {
    indicator=1;
    indicator_clock=0.0f;
    blink_duration=SCOREBOARD_BLINK_DURATION;
  }
}

/* Update animation for the central indicator.
 * Should be common to all games, but they do have to opt in.
 */
 
static void scoreboard_update_animation(struct fmn_sprite *sprite,float elapsed) {
  
  if (((int8_t)indicator==-1)||(indicator==1)) {
    // blinking
    if ((blink_duration-=elapsed)<=0.0f) {
      indicator=0;
      indicator_clock=0.0f;
      sprite->tileid=tileid0;
      return;
    }
    indicator_clock+=elapsed;
    if (indicator_clock>=SCOREBOARD_BLINK_TIME) indicator_clock-=SCOREBOARD_BLINK_TIME;
    if (indicator_clock>=SCOREBOARD_ILLUMINATION_TIME) sprite->tileid=tileid0;
    else if ((int8_t)indicator<0) sprite->tileid=tileid0+1;
    else sprite->tileid=tileid0+2;
    
  } else if ((int8_t)indicator==-2) {
    sprite->tileid=tileid0+1;
  } else if (indicator==2) {
    sprite->tileid=tileid0+2;
  } else {
    sprite->tileid=tileid0;
  }
}

/* SEAMONSTER_PONG
 */
 
static void scoreboard_cb_missile_oob(void *userdata,uint16_t eventid,void *payload) {
  struct fmn_sprite *sprite=userdata;
  struct fmn_sprite *missile=payload;
  if (!running) return;
  if (missile->tileid!=0xb5) return; // only fish bones count
  if (missile->x<0.0f) {
    fmn_scoreboard_score(sprite,-1);
  } else if (missile->x>FMN_COLC) {
    fmn_scoreboard_score(sprite,1);
  }
}
 
static void _scoreboard_init_SEAMONSTER_PONG(struct fmn_sprite *sprite) {
  listenerid=fmn_game_event_listen(FMN_GAME_EVENT_MISSILE_OOB,scoreboard_cb_missile_oob,sprite);
}
 
static void _scoreboard_update_SEAMONSTER_PONG(struct fmn_sprite *sprite,float elapsed) {
  scoreboard_update_animation(sprite,elapsed);
  if (!running) {
    sprite->update=0;
    fmn_game_event_unlisten(listenerid);
    listenerid=0;
    return;
  }
}

/* Init.
 */
 
static void _scoreboard_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  scoreboard_generate_children(sprite);
  running=1;
  switch (gameid) {
    #define _(tag) case SCOREBOARD_GAMEID_##tag: _scoreboard_init_##tag(sprite); sprite->update=_scoreboard_update_##tag; break;
    _(SEAMONSTER_PONG)
    #undef _
    default: running=0;
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_scoreboard={
  .init=_scoreboard_init,
  // We do have an (update) hook but it gets chosen dynamically and can be null.
};

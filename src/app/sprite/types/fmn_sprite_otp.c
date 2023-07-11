#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

#define role sprite->argv[0]
#define gsbit sprite->argv[1]
#define gsbit_submit sprite->argv[2] /* and the "reject" gsbit is n+1 */
#define tileid0 sprite->bv[0]
#define reject_time sprite->fv[0]

#define OTP_ROLE_READ 0
#define OTP_ROLE_WRITE 1

/* Update.
 */
 
static void _otp_update(struct fmn_sprite *sprite,float elapsed) {
  if (reject_time>0.0f) {
    if ((reject_time-=elapsed)<=0.0f) {
      reject_time=0.0f;
      fmn_gs_set_bit(gsbit_submit+1,0);
    }
  }
}

/* Interact.
 */
 
static int16_t _otp_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
  }
  return 0;
}

/* Find a partner sprite.
 */
 
struct otp_find_partners_context {
  struct fmn_sprite *sprite;
  struct fmn_sprite *left;
  struct fmn_sprite *right;
};

static int otp_find_partners_1(struct fmn_sprite *q,void *userdata) {
  struct otp_find_partners_context *ctx=userdata;
  if (q==ctx->sprite) return 0;
  if (q->controller!=FMN_SPRCTL_dummy) return 0;
  if (q->imageid!=ctx->sprite->imageid) return 0;
  if (q->y<ctx->sprite->y-0.25f) return 0;
  if (q->y>ctx->sprite->y+0.25f) return 0;
  float dx=q->x-ctx->sprite->x;
  if ((dx>0.25f)&&(dx<1.0f)) ctx->right=q;
  else if ((dx>-1.0f)&&(dx<-0.25f)) ctx->left=q;
  if (ctx->left&&ctx->right) return 1;
  return 0;
}
 
static int otp_find_partners(struct fmn_sprite **dst/*2*/,struct fmn_sprite *sprite) {
  struct otp_find_partners_context ctx={.sprite=sprite};
  if (fmn_sprites_for_each(otp_find_partners_1,&ctx)<1) return 0;
  if (!ctx.left||!ctx.right) return 0;
  dst[0]=ctx.left;
  dst[1]=ctx.right;
  return 1;
}

/* Change me and my partners' tileid to show a given password. 0..999
 */
 
static void otp_display_password(struct fmn_sprite *sprite,int password) {
  sprite->tileid=tileid0+((password/10)%10);
  struct fmn_sprite *partnerv[2];
  if (otp_find_partners(partnerv,sprite)) {
    partnerv[0]->tileid=tileid0+((password/100)%10);
    partnerv[1]->tileid=tileid0+password%10;
  }
}

/* We are the read sprite, and we're just created. Generate a random password, store it, and display it.
 */
 
static void otp_generate_new_password(struct fmn_sprite *sprite) {
  int password=1+rand()%999;
  otp_display_password(sprite,password);
  fmn_gs_set_word(gsbit,10,password);
  fmn_saved_game_dirty();
}

/* We are the write sprite, and we're just created.
 * Read the user's guess out of gs and display it.
 */
 
static void otp_read_guess_from_gs(struct fmn_sprite *sprite) {
  int password=fmn_gs_get_word(gsbit+10,10);
  otp_display_password(sprite,password);
}

/* The true OTP sprite is the middle digit.
 * Spawn two partners, left and right.
 */
 
static int otp_spawn_partners(struct fmn_sprite *sprite) {

  struct fmn_sprite *left=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x-12.0f/16.0f,sprite->y);
  struct fmn_sprite *right=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x+12.0f/16.0f,sprite->y);
  if (!left||!right) {
    if (left) fmn_sprite_kill(left);
    if (right) fmn_sprite_kill(right);
    return -1;
  }
  left->imageid=right->imageid=sprite->imageid;
  left->tileid=right->tileid=sprite->tileid;
  left->style=right->style=sprite->style;
  left->layer=right->layer=sprite->layer;

  return 0;
}

/* Value changed via treadmill.
 */
 
static int otp_add_digit(int v,int d) {
  int o=v%10;
  int t=(v-o)%100;
  int h=v-t-o;
  if ((d<=-100)||(d>=100)) { h+=d; if (h<0) h+=1000; else if (h>=1000) h-=1000; }
  else if ((d<=-10)||(d>=10)) { t+=d; if (t<0) t+=100; else if (t>=100) t-=100; }
  else { o+=d; if (o<0) o+=10; else if (o>=10) o-=10; }
  return h+t+o;
}
 
static void otp_cb_treadmill(void *userdata,uint16_t eventid,void *payload) {
  struct fmn_sprite *sprite=userdata;
  int d=*(int*)payload;
  int password=fmn_gs_get_word(gsbit+10,10);
  password=otp_add_digit(password,d);
  fmn_gs_set_word(gsbit+10,10,password);
  fmn_saved_game_dirty();
  otp_display_password(sprite,password);
}

/* "Submit" button pressed.
 */
 
static void otp_cb_submit(void *userdata,uint16_t p,uint8_t v) {
  struct fmn_sprite *sprite=userdata;
  if (!v) return;
  if (fmn_gs_get_bit(gsbit_submit+2)) {
    fmn_log("%s already passed",__func__);
    return;
  }
  int password=fmn_gs_get_word(gsbit,10);
  int guess=fmn_gs_get_word(gsbit+10,10);
  fmn_log("%s password=%d guess=%d",__func__,password,guess);
  if (password==guess) {
    //TODO sound effect
    fmn_gs_set_bit(gsbit_submit+2,1);
  } else {
    //TODO sound effect
    fmn_gs_set_bit(gsbit_submit+1,1);
    reject_time=2.0f;
  }
}

/* Init.
 */
 
static void _otp_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  sprite->y+=6.0f/16.0f;
  if (otp_spawn_partners(sprite)<0) {
    fmn_sprite_kill(sprite);
  }
  if (role==OTP_ROLE_READ) {
    otp_generate_new_password(sprite);
  } else {
    otp_read_guess_from_gs(sprite);
    fmn_game_event_listen(FMN_GAME_EVENT_TREADMILL,otp_cb_treadmill,sprite);
    fmn_gs_listen_bit(gsbit_submit,otp_cb_submit,sprite);
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_otp={
  .init=_otp_init,
  .update=_otp_update,
  .interact=_otp_interact,
};

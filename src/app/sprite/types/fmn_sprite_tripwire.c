#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define gsbit sprite->argv[0]
#define tileid0 sprite->bv[0]
#define range sprite->fv[0]
#define debounce sprite->fv[1]

#define TW_RADIUS 0.25f
#define TW_DEBOUNCE_TIME 1.0f

/* Beam sprites.
 * For now, a tripwire gun is always facing south.
 * Wouldn't be a big deal to use argv[1] as "direction", and orient any cardinal direction.
 */
 
static void tw_make_beams(struct fmn_sprite *sprite) {
  int8_t x=sprite->x;
  if ((x<0)||(x>=FMN_COLC)) return;
  int8_t y=sprite->y;
  y++;
  range=0.5f;
  while ((y>=0)&&(y<FMN_ROWC)) {
    switch (fmn_global.cellphysics[fmn_global.map[y*FMN_COLC+x]]) {
      case FMN_CELLPHYSICS_SOLID:
      case FMN_CELLPHYSICS_UNCHALKABLE:
      case FMN_CELLPHYSICS_SAP:
      case FMN_CELLPHYSICS_SAP_NOCHALK:
      case FMN_CELLPHYSICS_REVELABLE:
        return;
    }
    struct fmn_sprite *beam=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x,y+0.5f);
    if (!beam) return;
    beam->imageid=sprite->imageid;
    beam->tileid=tileid0+0x10;
    beam->style=FMN_SPRITE_STYLE_TWOFRAME;
    y++;
    range+=1.0f;
  }
}

/* Beams style, so we can flash them when tripped.
 */
 
struct tw_beams_style_context {
  struct fmn_sprite *sprite;
  uint8_t style;
};

static int tw_set_beams_style_1(struct fmn_sprite *q,void *userdata) {
  struct tw_beams_style_context *ctx=userdata;
  struct fmn_sprite *sprite=ctx->sprite;
  if (q->controller!=FMN_SPRCTL_dummy) return 0;
  if (q->imageid!=sprite->imageid) return 0;
  if (q->tileid!=tileid0+0x10) return 0;
  if ((q->x<sprite->x-0.125f)||(q->x>sprite->x+0.125f)) return 0;
  if (q->y<sprite->y) return 0;
  if (q->y>sprite->y+range) return 0;
  q->style=ctx->style;
  return 0;
}
 
static void tw_set_beams_style(struct fmn_sprite *sprite,uint8_t style) {
  struct tw_beams_style_context ctx={
    .sprite=sprite,
    .style=style,
  };
  fmn_sprites_for_each(tw_set_beams_style_1,&ctx);
}

/* Trip.
 */
 
static void tw_trip(struct fmn_sprite *sprite) {
  fmn_gs_set_bit(gsbit,1);
  //TODO sound effect for tripwire?
  debounce=TW_DEBOUNCE_TIME;
  tw_set_beams_style(sprite,FMN_SPRITE_STYLE_HIDDEN);
}

/* Update.
 */
 
static void _tw_update(struct fmn_sprite *sprite,float elapsed) {
  if (debounce>0.0f) {
    if ((debounce-=elapsed)<=0.0f) {
      debounce=0.0f;
      tw_set_beams_style(sprite,FMN_SPRITE_STYLE_TWOFRAME);
      fmn_gs_set_bit(gsbit,0);
    } else return;
  }
  if (fmn_global.invisibility_time<=0.0f) {
    float herox,heroy;
    fmn_hero_get_position(&herox,&heroy);
    if ((heroy>=sprite->y)&&(heroy<=sprite->y+range)&&(herox>=sprite->x-TW_RADIUS)&&(herox<=sprite->x+TW_RADIUS)) {
      tw_trip(sprite);
    }
  }
}

/* Init.
 */
 
static void _tw_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  tw_make_beams(sprite);
  fmn_gs_set_bit(gsbit,0);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_tripwire={
  .init=_tw_init,
  .update=_tw_update,
};

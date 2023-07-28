/* fmn_sprite_ghostgen.c
 * Not a real sprite.
 * Monitors a given region of the map, and if anyone digs there, generate a ghost.
 * (for the graveyard)
 */

#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define col sprite->bv[0]
#define row sprite->bv[1]
#define colc sprite->argv[0]
#define rowc sprite->argv[1]
#define blackout sprite->fv[0]

/* Update.
 */
 
static void _ghostgen_update(struct fmn_sprite *sprite,float elapsed) {
  if (blackout>0.0f) {
    blackout-=elapsed;
    return;
  }
  if (fmn_global.active_item!=FMN_ITEM_SHOVEL) return;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  int8_t herocol=(int8_t)herox;
  if ((herocol<col)||(herocol>=col+colc)) return;
  int8_t herorow=(int8_t)heroy;
  if ((herorow<row)||(herorow>=row+rowc)) return;
  blackout=2.0f;
  struct fmn_sprite *ghost=fmn_sprite_generate_noparam(FMN_SPRCTL_ghost,col+colc*0.5f,row+rowc*0.5f);
  if (ghost) {
    ghost->imageid=3;
    ghost->tileid=0x98;
    ghost->style=FMN_SPRITE_STYLE_TWOFRAME;
    ghost->layer=140;
    ghost->radius=0.5f;
  }
}

/* Init.
 */
 
static void _ghostgen_init(struct fmn_sprite *sprite) {
  col=(uint8_t)sprite->x;
  row=(uint8_t)sprite->y;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_ghostgen={
  .init=_ghostgen_init,
  .update=_ghostgen_update,
};

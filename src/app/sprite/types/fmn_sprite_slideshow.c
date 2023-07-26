#include "app/sprite/fmn_sprite.h"

#define colc sprite->bv[0] /* from sprite resource */
#define rowc sprite->bv[1] /* from sprite resource */
#define tileid0 sprite->bv[2]
#define col sprite->bv[3]
#define row sprite->bv[4]
#define clock sprite->fv[0]

#define SLIDESHOW_FRAME_TIME 2.5f

/* Update.
 */
 
static void _slideshow_update(struct fmn_sprite *sprite,float elapsed) {
  clock+=elapsed;
  if (clock>=SLIDESHOW_FRAME_TIME) {
    clock-=SLIDESHOW_FRAME_TIME;
    col++;
    if (col>=colc) {
      col=0;
      row++;
      if (row>=rowc) {
        row=0;
      }
    }
    sprite->tileid=tileid0+(row<<5)+(col*3);
  }
}

/* Init.
 */
 
static void _slideshow_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  sprite->hbn=0.5f;
  sprite->hbw=1.5f;
  sprite->hbe=1.5f;
  sprite->hbs=1.5f;
  sprite->radius=0.0f;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_slideshow={
  .init=_slideshow_init,
  .update=_slideshow_update,
};

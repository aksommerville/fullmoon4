#include "app/sprite/fmn_sprite.h"

#define TOAST_SPEED 1.0f

#define ttl sprite->fv[0]
#define dx sprite->fv[1]
#define dy sprite->fv[2]

/* Init.
 */
 
static void _toast_init(struct fmn_sprite *sprite) {
  ttl=2.0f;
  sprite->layer=200;
  dx=0.0f;
  dy=-TOAST_SPEED;
}

/* Update.
 */
 
static void _toast_update(struct fmn_sprite *sprite,float elapsed) {
  sprite->x+=dx*elapsed;
  sprite->y+=dy*elapsed;
  if ((ttl-=elapsed)<=0.0f) fmn_sprite_kill(sprite);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_toast={
  .init=_toast_init,
  .update=_toast_update,
};

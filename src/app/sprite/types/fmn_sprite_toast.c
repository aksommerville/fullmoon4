#include "app/sprite/fmn_sprite.h"

#define TOAST_SPEED 1.0f

#define ttl sprite->fv[0]
#define dx sprite->fv[1]
#define dy sprite->fv[2]
#define restorettl sprite->fv[3]
#define xxtra sprite->fv[4]
#define yxtra sprite->fv[5]
#define source sprite->pv[0]
// For a persistent repeating toast, caller should set (source) and (restorettl).

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
  
  if ((ttl-=elapsed)<=0.0f) {
    if (source&&(restorettl>0.0f)) {
      struct fmn_sprite *src=source;
      if (!src->style) {
        // We have a source but it's been removed.
        // Luckily we caught it before the source object got reused...
        fmn_sprite_kill(sprite);
      } else {
        sprite->x=src->x+xxtra;
        sprite->y=src->y-0.5f+yxtra;
        ttl=restorettl;
      }
    } else {
      // Normal single-use toast.
      fmn_sprite_kill(sprite);
    }
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_toast={
  .init=_toast_init,
  .update=_toast_update,
};

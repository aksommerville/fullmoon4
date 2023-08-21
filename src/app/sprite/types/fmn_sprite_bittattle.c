#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

#define leftbit sprite->argv[0]
#define rightbit sprite->argv[1]
#define tileid0 sprite->bv[0]

/* gsbit events.
 */
 
static void _bittattle_change(void *userdata,uint16_t p,uint8_t v) {
  struct fmn_sprite *sprite=userdata;
  if (v) {
    if (p==leftbit) sprite->tileid=tileid0+1;
    else if (p==rightbit) sprite->tileid=tileid0+2;
  }
}

/* Init.
 */
 
static void _bittattle_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  if (fmn_gs_get_bit(leftbit)) sprite->tileid=tileid0+1;
  else if (fmn_gs_get_bit(rightbit)) sprite->tileid=tileid0+2;
  fmn_gs_listen_bit(leftbit,_bittattle_change,sprite);
  fmn_gs_listen_bit(rightbit,_bittattle_change,sprite);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_bittattle={
  .init=_bittattle_init,
};

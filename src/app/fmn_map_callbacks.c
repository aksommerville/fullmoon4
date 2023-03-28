#include "app/fmn_game.h"
#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

static int _find_top_pushblock(struct fmn_sprite *sprite,void *userdata) {
  struct fmn_sprite **found=userdata;
  if (sprite->controller!=FMN_SPRCTL_pushblock) return 0;
  if (*found&&((*found)->y<sprite->y)) return 0;
  *found=sprite;
  return 0;
}

void fmn_map_callback_kill_top_pushblock_if_pumpkin_at_nw(uint8_t param,void *userdata) {
  if (fmn_global.transmogrification!=1) return;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (herox>1.0f) return;
  struct fmn_sprite *pushblock=0;
  fmn_sprites_for_each(_find_top_pushblock,&pushblock);
  if (!pushblock) return;
  if (heroy>=pushblock->y) return;
  fmn_sprite_kill(pushblock);
}

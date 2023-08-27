#include "fmn_eatwq_internal.h"
#include "app/fmn_game.h"

void fmn_arcade_set_context(struct fmn_eatwq_context *ctx);

/* Begin.
 */
 
int fmn_eatwq_begin(struct fmn_eatwq_context *ctx) {
  fmn_arcade_set_context(ctx);
  fmn_begin_menu(FMN_MENU_ARCADE,0);
  return 0;
}

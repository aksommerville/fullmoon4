/* fmn_eatwq.h
 * "Enchanting Adventures: The Witch's Quest", aka "The Witches of Eatwick".
 * Interface to Dot's arcade cabinet.
 */
 
#ifndef FMN_EATWQ_H
#define FMN_EATWQ_H

#include <stdint.h>

struct fmn_draw_mintile;

struct fmn_eatwq_context {
  int creditc;
  int hiscore;
};

/* Treats with app's upper layers to begin playing eatwq modally.
 * (ctx) contains the initial credit and score. (0..7,0..255).
 * We hold on to this pointer and update both fields when they change.
 * Caller should check again when he resumes control.
 * We do not read or write gs -- that's the caller's responsibility.
 */
int fmn_eatwq_begin(struct fmn_eatwq_context *ctx);

/* Detailed API for fmn_menu_arcade to consume.
 **************************************************************/

void fmn_eatwq_init(
  uint8_t creditc,uint8_t hiscore,
  void (*cb_creditc)(uint8_t creditc,void *userdata),
  void (*cb_hiscore)(uint8_t hiscore,void *userdata),
  void *userdata
);

/* Caller should regulate timing at 60 Hz.
 */
void fmn_eatwq_update(uint8_t input,uint8_t pvinput);

/* Returns (vtxc).
 * Caller draws the background: horizon at height-16.
 */
int fmn_eatwq_render(struct fmn_draw_mintile *vtxv,int vtxa);

int fmn_eatwq_is_running();

#endif

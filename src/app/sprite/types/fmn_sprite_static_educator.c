/* fmn_sprite_static_educator.c
 * This is not a real sprite.
 * We're just abusing the sprite controller interface to apply some quirky logic.
 * Used in the swamp, where you push a pushblock so as to connect an alphablock with a static 3x3 region.
 * The tiles of that 3x3 region change depending on which blocks are touching.
 *
 * Place the fake sprite at the center of the 3x3 region.
 * Tiles for the 3x3 region must be aligned vertically (anywhere on the sheet), and their horz position is meaningful:
 *   0..2: disconnected
 *   3..5: gamma
 *   6..8: alpha
 */

#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

/* Globals. There must not be more than one static educator at a time.
 */
 
static struct {
  int8_t x,y; // center of 3x3 region
  uint8_t tilecol;
} sted={1,1};

/* Change the map's tiles, if request is actually different from current state.
 */
 
static void sted_retile(uint8_t col) {
  if (col>0x0e) return;
  if (col==sted.tilecol) return;
  fmn_log("%s %d",__func__,col);
  sted.tilecol=col;
  uint8_t *maprow=fmn_global.map+(sted.y-1)*FMN_COLC+sted.x-1;
  maprow[0]=(maprow[0]&0xf0)|col;
  maprow[1]=(maprow[0]&0xf0)|(col+1);
  maprow[2]=(maprow[0]&0xf0)|(col+2);
  maprow+=FMN_COLC;
  maprow[0]=(maprow[0]&0xf0)|col;
  maprow[1]=(maprow[0]&0xf0)|(col+1);
  maprow[2]=(maprow[0]&0xf0)|(col+2);
  maprow+=FMN_COLC;
  maprow[0]=(maprow[0]&0xf0)|col;
  maprow[1]=(maprow[0]&0xf0)|(col+1);
  maprow[2]=(maprow[0]&0xf0)|(col+2);
  fmn_map_dirty();
}

/* Find the alphablock we're connected to.
 */
 
#define STED_SPACE_FUDGE 0.125f
 
struct sted_find_connection_context {
  struct fmn_sprite *pushblock;
  struct fmn_sprite *alphablock;
  float midx,midy; // (sted.[xy]) but floated at the cell's center
  float targetx,targety; // expected position of alphablock, depends on pushblock
};

static int _sted_find_connection_final(struct fmn_sprite *sprite,void *userdata) {
  struct sted_find_connection_context *ctx=userdata;
  if (sprite->controller!=FMN_SPRCTL_alphablock) return 0;
  
  float dx=sprite->x-ctx->targetx;
  if (dx>STED_SPACE_FUDGE) return 0;
  if (dx<-STED_SPACE_FUDGE) return 0;
  float dy=sprite->y-ctx->targety;
  if (dy>STED_SPACE_FUDGE) return 0;
  if (dy<-STED_SPACE_FUDGE) return 0;
  
  ctx->alphablock=sprite;
  return 1;
}
 
static int _sted_find_connection_1(struct fmn_sprite *sprite,void *userdata) {
  struct sted_find_connection_context *ctx=userdata;
  if (sprite->controller!=FMN_SPRCTL_pushblock) return 0;
  
  // Determine which cardinal edge the block abuts. If none, return zero.
  ctx->targetx=sprite->x;
  ctx->targety=sprite->y;
  float dx=sprite->x-ctx->midx;
  float dy=sprite->y-ctx->midy;
  if ((dx>=-1.5f)&&(dx<=1.5f)) { // top or bottom
    if ((dy>=-2.0f-STED_SPACE_FUDGE)&&(dy<=-2.0f+STED_SPACE_FUDGE)) { // top
      ctx->targety-=1.0f;
    } else if ((dy>=2.0f-STED_SPACE_FUDGE)&&(dy<=2.0f+STED_SPACE_FUDGE)) { // bottom
      ctx->targety+=1.0f;
    } else return 0;
  } else if ((dy>=-1.5f)&&(dy<=1.5f)) { // left or right
    if ((dx>=-2.0f-STED_SPACE_FUDGE)&&(dx<=-2.0f+STED_SPACE_FUDGE)) { // left
      ctx->targetx-=1.0f;
    } else if ((dx>=2.0f-STED_SPACE_FUDGE)&&(dx<=2.0f+STED_SPACE_FUDGE)) { // right
      ctx->targetx+=1.0f;
    } else return 0;
  } else return 0;
  
  ctx->pushblock=sprite;
  return fmn_sprites_for_each(_sted_find_connection_final,ctx);
}
 
static struct fmn_sprite *sted_find_connection() {
  struct sted_find_connection_context ctx={0};
  ctx.midx=sted.x+0.5f;
  ctx.midy=sted.y+0.5f;
  fmn_sprites_for_each(_sted_find_connection_1,&ctx);
  return ctx.alphablock;
}

/* React to change in blocks state.
 */

// tileid of alphablock
#define AB_MODE_ALPHA 0x01
#define AB_MODE_GAMMA 0x02
#define AB_MODE_LAMBDA 0x03
#define AB_MODE_MU 0x04
 
static void _sted_blocks_moved(void *userdata,uint16_t eventid,void *payload) {
  struct fmn_sprite *subject=sted_find_connection();
  if (subject) {
    if (subject->controller==FMN_SPRCTL_alphablock) switch (subject->tileid) {
      case AB_MODE_ALPHA: sted_retile(6); return;
      case AB_MODE_GAMMA: sted_retile(3); return;
    }
  }
  sted_retile(0);
}

/* Init.
 */
 
static void _sted_init(struct fmn_sprite *sprite) {
  
  sted.tilecol=0; // we always begin in the blank state, and don't check initially.
  
  // (sted.[xy]) must have one meter clearance on all sides. force it so.
  sted.x=sprite->x;
  if (sted.x<1) sted.x=1;
  else if (sted.x>=FMN_COLC-1) sted.x=FMN_COLC-2;
  sted.y=sprite->y;
  if (sted.y<1) sted.y=1;
  else if (sted.y>=FMN_ROWC-1) sted.y=FMN_ROWC-2;
  
  uint16_t listener=fmn_game_event_listen(FMN_GAME_EVENT_BLOCKS_MOVED,_sted_blocks_moved,0);
  fmn_sprite_kill(sprite);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_static_educator={
  .init=_sted_init,
};

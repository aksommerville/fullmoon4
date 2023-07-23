#include "app/sprite/fmn_sprite.h"

#define tileid0 sprite->bv[0]
#define opened sprite->bv[1]
#define tileid_open sprite->argv[0]

#define BOOK_HBW_CLOSED  0.375f
#define BOOK_HBE_CLOSED  0.375f
#define BOOK_HBN_CLOSED  0.500f
#define BOOK_HBS_CLOSED  0.500f
#define BOOK_HBW_OPEN  1.000f
#define BOOK_HBE_OPEN  1.000f
#define BOOK_HBN_OPEN  0.500f
#define BOOK_HBS_OPEN  0.500f

/* Open/close.
 */
 
static void book_open(struct fmn_sprite *sprite) {
  if (opened) return;
  opened=1;
  sprite->tileid=tileid_open;
  sprite->style=FMN_SPRITE_STYLE_DOUBLEWIDE;
  sprite->x-=0.5f;
  sprite->hbw=BOOK_HBW_OPEN;
  sprite->hbe=BOOK_HBE_OPEN;
  sprite->hbn=BOOK_HBN_OPEN;
  sprite->hbs=BOOK_HBS_OPEN;
}
 
static void book_close(struct fmn_sprite *sprite) {
  if (!opened) return;
  opened=0;
  sprite->tileid=tileid0;
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->x+=0.5f;
  sprite->hbw=BOOK_HBW_CLOSED;
  sprite->hbe=BOOK_HBE_CLOSED;
  sprite->hbn=BOOK_HBN_CLOSED;
  sprite->hbs=BOOK_HBS_CLOSED;
}

static void book_toggle(struct fmn_sprite *sprite) {
  if (opened) book_close(sprite);
  else book_open(sprite);
}

/* Interact.
 */
 
static int16_t _book_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_BELL: book_toggle(sprite); break;
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_OPEN: book_open(sprite); break;
      } break;
  }
  return 0;
}

/* Init.
 */
 
static void _book_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  sprite->radius=0.0f;
  sprite->hbw=BOOK_HBW_CLOSED;
  sprite->hbe=BOOK_HBE_CLOSED;
  sprite->hbn=BOOK_HBN_CLOSED;
  sprite->hbs=BOOK_HBS_CLOSED;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_book={
  .init=_book_init,
  .interact=_book_interact,
};

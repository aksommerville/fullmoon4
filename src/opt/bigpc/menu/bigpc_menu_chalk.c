#if 0 /*XXX move to client*/
/* bigpc_menu_chalk.c
 * Interactive chalk-drawing menu.
 */
 
#include "../bigpc_internal.h"

/* Instance definition.
 */
 
struct bigpc_menu_chalk {
  struct bigpc_menu hdr;
  uint8_t pvinput; // we track input independently
  uint8_t focus; // an edit is in progress, USE button is down
  int8_t anchorx;
  int8_t anchory;
};

#define MENU ((struct bigpc_menu_chalk*)menu)

/* Cleanup.
 */
 
static void _chalk_del(struct bigpc_menu *menu) {
}

/* Ready.
 */
 
static int _chalk_ready(struct bigpc_menu *menu) {
  const struct bigpc_menu_option *option=menu->optionv;
  int i=menu->optionc;
  for (;i-->0;option++) {
    if (option->stringid&0x80000000) {
      menu->extra[0]=option->stringid&0x000fffff;
      break;
    }
  }
  return 0;
}

/* Set the highlight bit (extra[1]) according to (selx,sely) and (anchorx,anchory).
 * Call only while focussed.
 */

//TODO bigpc is allowed to build without gl2. Maybe this function should move to bigpc?
uint32_t fmn_gl2_chalk_bit_from_points(uint32_t points);
 
static void chalk_rebuild_highlight(struct bigpc_menu *menu) {
  if ((menu->selx==MENU->anchorx)&&(menu->sely==MENU->anchory)) {
    menu->extra[1]=0;
  } else {
    uint32_t points=(menu->selx<<12)|(menu->sely<<8)|(MENU->anchorx<<4)|MENU->anchory;
    menu->extra[1]=fmn_gl2_chalk_bit_from_points(points);
  }
}

/* Move cursor.
 */
 
static void chalk_move(struct bigpc_menu *menu,int8_t dx,int8_t dy) {
  int8_t nx=menu->selx+dx; if (nx<0) nx=2; else if (nx>2) nx=0;
  int8_t ny=menu->sely+dy; if (ny<0) ny=2; else if (ny>2) ny=0;
  if ((nx==menu->selx)&&(ny==menu->sely)) return;
  if (MENU->focus) {
    // While setting a line, the new position must be within 1 of the old on both axes, without wrapping.
    int ex=nx-MENU->anchorx; if (ex<0) ex=-ex;
    int ey=ny-MENU->anchory; if (ey<0) ey=-ey;
    if ((ex>1)||(ey>1)) return;
    menu->selx=nx;
    menu->sely=ny;
    chalk_rebuild_highlight(menu);
  } else {
    menu->selx=nx;
    menu->sely=ny;
  }
}

/* Line begin and end.
 */
 
static void chalk_begin_line(struct bigpc_menu *menu) {
  MENU->focus=1;
  MENU->anchorx=menu->selx;
  MENU->anchory=menu->sely;
  menu->extra[1]=0;
}

static void chalk_end_line(struct bigpc_menu *menu) {
  if (menu->extra[1]) {
    menu->extra[0]^=menu->extra[1];
  }
  MENU->focus=0;
  menu->extra[1]=0;
}

/* Update.
 */
 
static int _chalk_update(struct bigpc_menu *menu,uint8_t new_input) {
  if (bigpc.input_state!=MENU->pvinput) {
    #define ON(tag) ((bigpc.input_state&FMN_INPUT_##tag)&&!(MENU->pvinput&FMN_INPUT_##tag))
    #define OFF(tag) (!(bigpc.input_state&FMN_INPUT_##tag)&&(MENU->pvinput&FMN_INPUT_##tag))
    if (ON(LEFT)) chalk_move(menu,-1,0);
    if (ON(RIGHT)) chalk_move(menu,1,0);
    if (ON(UP)) chalk_move(menu,0,-1);
    if (ON(DOWN)) chalk_move(menu,0,1);
    if (ON(USE)) chalk_begin_line(menu);
    else if (OFF(USE)) chalk_end_line(menu);
    if (ON(MENU)) {
      bigpc_menu_callback_any(menu);
      return 0;
    }
    #undef ON
    #undef OFF
    MENU->pvinput=bigpc.input_state;
  }
  return 1;
}

/* New.
 */
 
struct bigpc_menu *bigpc_menu_new_CHALK() {
  struct bigpc_menu *menu=calloc(1,sizeof(struct bigpc_menu_chalk));
  if (!menu) return 0;
  menu->prompt=FMN_MENU_CHALK;
  menu->pvinput=0xff;
  MENU->pvinput=0xff;
  menu->selx=1;
  menu->sely=1;
  menu->extra[0]=0; // bits
  menu->extra[1]=0; // highlighted bit
  menu->del=_chalk_del;
  menu->ready=_chalk_ready;
  menu->update=_chalk_update;
  return menu;
}
#endif

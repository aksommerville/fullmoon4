#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define tileid0 sprite->bv[0]
#define stage sprite->bv[1]
#define songp sprite->bv[2]
#define songcomplete sprite->bv[3]
#define animclock sprite->fv[0]
#define pvviolinclock sprite->fv[1]
#define gsbit_selection sprite->argv[0]

#define MT_STAGE_IDLE 0
#define MT_STAGE_NO_VIOLIN 1
#define MT_STAGE_READY 2
#define MT_STAGE_TEACH 3

#define MT_TEACH_FRAME_TIME 0.25f

/* Store the current song globally, it won't fit inside the sprite.
 * This means we can only have one music teacher per map, which is fine. There's only one in the whole game.
 * (unless we turn the Rabbit into a Music Teacher? Think that over. But still fine.)
 */
static uint8_t mt_song[FMN_VIOLIN_SONG_LENGTH];
static uint8_t mt_songc=0;

/* Word bubble.
 */
 
struct mt_bubble_search_context {
  struct fmn_sprite *sprite;
  struct fmn_sprite *bubble;
};

static int mt_bubble_search_cb(struct fmn_sprite *q,void *userdata) {
  struct mt_bubble_search_context *ctx=userdata;
  struct fmn_sprite *sprite=ctx->sprite;
  if (q->controller!=FMN_SPRCTL_dummy) return 0;
  if (q->style!=FMN_SPRITE_STYLE_TILE) return 0;
  if (q->imageid!=ctx->sprite->imageid) return 0;
  if (q->tileid!=tileid0+3) return 0;
  ctx->bubble=q;
  return 1;
}
 
static void mt_drop_word_bubble(struct fmn_sprite *sprite) {
  struct mt_bubble_search_context ctx={.sprite=sprite};
  fmn_sprites_for_each(mt_bubble_search_cb,&ctx);
  if (!ctx.bubble) return;
  fmn_sprite_kill(ctx.bubble);
}

static void mt_create_word_bubble(struct fmn_sprite *sprite) {
  struct fmn_sprite *bubble=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x,sprite->y-1.0f);
  if (!bubble) return;
  bubble->style=FMN_SPRITE_STYLE_TILE;
  bubble->imageid=sprite->imageid;
  bubble->tileid=tileid0+3;
  bubble->layer=0x90;
}

/* Enter IDLE stage. No op if already there.
 */
 
static void mt_set_stage_IDLE(struct fmn_sprite *sprite) {
  if (stage==MT_STAGE_IDLE) return;
  if (stage==MT_STAGE_NO_VIOLIN) mt_drop_word_bubble(sprite);
  stage=MT_STAGE_IDLE;
  sprite->tileid=tileid0;
}

/* Enter READY stage. No op if already there.
 */
 
static void mt_set_stage_READY(struct fmn_sprite *sprite) {
  if (stage==MT_STAGE_READY) return;
  if (stage==MT_STAGE_NO_VIOLIN) mt_drop_word_bubble(sprite);
  stage=MT_STAGE_READY;
  sprite->tileid=tileid0;
}

/* Enter NO_VIOLIN stage, if not already there.
 */
 
static void mt_set_stage_NO_VIOLIN(struct fmn_sprite *sprite) {
  if (stage==MT_STAGE_NO_VIOLIN) return;
  mt_create_word_bubble(sprite);
  stage=MT_STAGE_NO_VIOLIN;
  sprite->tileid=tileid0;
}

/* Advance song position.
 * Play the note at current (songp), then increment.
 * "visual" beat should fire when the clock rolls over, not at the midpoint like the regular beat.
 */
 
uint8_t fmn_violin_note_from_dir(uint8_t dir); // fmn_hero_item.c
 
static void mt_advance_beat(struct fmn_sprite *sprite) {
  if (songp<mt_songc) {
    uint8_t dir=mt_song[songp];
    uint8_t noteid=fmn_violin_note_from_dir(dir);
    if (noteid) {
      // Release notes immediately. This plays a little staccato, which is probably for the best.
      // And it eliminates any danger of a note getting stuck on.
      fmn_synth_event(0x0d,0x90,noteid,0x40);
      fmn_synth_event(0x0d,0x80,noteid,0x40);
    }
  }
  if (++songp>=mt_songc) songp=0;
}

static void mt_advance_visual_beat(struct fmn_sprite *sprite) {
  int8_t dstp=(int8_t)fmn_global.violin_songp-1;
  if (dstp<0) dstp=FMN_VIOLIN_SONG_LENGTH-1;
  fmn_global.violin_shadow[dstp]=mt_song[songp];
}

/* Enter TEACH stage, if not already there.
 */
 
static void mt_set_stage_TEACH(struct fmn_sprite *sprite) {
  if (stage==MT_STAGE_TEACH) return;
  if (stage==MT_STAGE_NO_VIOLIN) mt_drop_word_bubble(sprite);
  stage=MT_STAGE_TEACH;
  sprite->tileid=tileid0+1;
  animclock=0.0f;
  
  uint8_t spellid;
  switch (fmn_gs_get_word(gsbit_selection,2)) {
    case 0: spellid=FMN_SPELLID_LULLABYE; break;
    case 1: spellid=FMN_SPELLID_REVEILLE; break;
    case 2: spellid=FMN_SPELLID_REVELATIONS; break;
    case 3: spellid=FMN_SPELLID_BLOOM; break;
  }

  mt_songc=fmn_spell_get(mt_song,sizeof(mt_song),spellid);
  if (!mt_songc||(mt_songc>sizeof(mt_song))) {
    mt_set_stage_IDLE(sprite);
    return;
  }
  // All songs are in 4/4 time. Bump length to the next multiple of 4, to play some silence at the end and align nice.
  while ((mt_songc<sizeof(mt_song))&&(mt_songc&3)) mt_song[mt_songc++]=0;
  fmn_synth_event(0x0d,0xc0,42,0); // Set program 42 on channel 13.
  pvviolinclock=fmn_global.violin_clock;
  songp=0;
  mt_advance_visual_beat(sprite);
}

/* Update, TEACH stage.
 */
 
static void mt_update_TEACH(struct fmn_sprite *sprite,float elapsed) {

  // Animation. (not connected to song tempo, but maybe it should be).
  animclock+=elapsed;
  if (animclock>=MT_TEACH_FRAME_TIME) {
    animclock-=MT_TEACH_FRAME_TIME;
    if (sprite->tileid==tileid0+1) sprite->tileid=tileid0+2;
    else sprite->tileid=tileid0+1;
  }
  
  // If (violin_clock) decreased, it means one beat elapsed.
  // This could fail if the sprite's update interval goes longer than one beat, I think that's not likely.
  if (
    (fmn_global.violin_clock>=0.5f)&&
    (pvviolinclock<0.5f)
  ) {
    mt_advance_beat(sprite);
  }
  if (fmn_global.violin_clock<pvviolinclock) {
    mt_advance_visual_beat(sprite);
  }
  pvviolinclock=fmn_global.violin_clock;
}

/* Update.
 */
 
static void _mt_update(struct fmn_sprite *sprite,float elapsed) {

  // Check for stage changes.
  if (fmn_global.active_item==FMN_ITEM_VIOLIN) {
    if (!songcomplete) {
      mt_set_stage_TEACH(sprite);
    }
  } else if ((fmn_global.selected_item==FMN_ITEM_VIOLIN)&&fmn_global.itemv[FMN_ITEM_VIOLIN]) {
    mt_set_stage_READY(sprite);
    songcomplete=0;
  } else {
    mt_set_stage_NO_VIOLIN(sprite);
    songcomplete=0;
  }
  
  switch (stage) {
    case MT_STAGE_TEACH: mt_update_TEACH(sprite,elapsed); break;
  }
}

/* Interact.
 */
 
static int16_t _mt_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
  }
  return 0;
}

/* Callback when a correct song is played.
 * It's not necessarily the song we were teaching.
 * But the violin mechanics are such that playback must stop when any valid song is played.
 */
 
static void _mt_song_ok(void *userdata,uint16_t eventid,void *payload) {
  if (!payload) return;
  struct fmn_sprite *sprite=userdata;
  uint8_t songid=*(uint8_t*)payload;
  if (stage==MT_STAGE_TEACH) {
    songcomplete=1;
    mt_set_stage_READY(sprite);
  }
}

/* Callback when the treadmill ticks.
 */
 
static void _mt_treadmill(void *userdata,uint16_t eventid,void *payload) {
  if (!payload) return;
  struct fmn_sprite *sprite=userdata;
  int d=*(int*)payload;
  int selection=fmn_gs_get_word(gsbit_selection,2);
  selection+=d;
  selection&=3;
  fmn_gs_set_word(gsbit_selection,2,selection);
}

/* Init.
 */
 
static void _mt_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  fmn_game_event_listen(FMN_GAME_EVENT_SONG_OK,_mt_song_ok,sprite);
  fmn_game_event_listen(FMN_GAME_EVENT_TREADMILL,_mt_treadmill,sprite);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_musicteacher={
  .init=_mt_init,
  .update=_mt_update,
  .interact=_mt_interact,
};

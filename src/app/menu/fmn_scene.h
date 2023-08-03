/* fmn_scene.h
 * 20x8-tile scenes of fake gameplay, for the credits menu.
 */
 
#ifndef FMN_SCENE_H
#define FMN_SCENE_H

#define FMN_SCENE_ROWC 8

#define FMN_SCENE_ACTION_DRAG_RL 1
#define FMN_SCENE_ACTION_DRAG_LR 2
#define FMN_SCENE_ACTION_DRAG_HOME 3
#define FMN_SCENE_ACTION_CLOTHE 4
#define FMN_SCENE_ACTION_GOODNIGHT 5
#define FMN_SCENE_ACTION_BUTCHER 6
#define FMN_SCENE_ACTION_SEW 7

struct fmn_scene_setup {
  uint16_t mapid;
  uint8_t action;
  float duration;
};

/* Client should treat struct fmn_scene as opaque.
 * But it's declared publicly so you can allocate statically.
 */
struct fmn_scene {
  const struct fmn_scene_setup *setup;
  uint8_t blackout; // alpha
  float clock;
  uint16_t maptsid;
  uint8_t map[FMN_COLC*FMN_SCENE_ROWC];
  uint8_t mapdirty;
  float animclock;
  int animframe;
  int stage;
  float stageclock;
  int stageextra;
};

int fmn_scene_init(struct fmn_scene *scene,const struct fmn_scene_setup *setup);

void fmn_scene_blank(struct fmn_scene *scene);

/* Returns nonzero if complete.
 */
uint8_t fmn_scene_update(struct fmn_scene *scene,float elapsed);

void fmn_scene_render(int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,struct fmn_scene *scene);

#endif

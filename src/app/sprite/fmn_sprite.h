/* fmn_sprite.h
 * Registry of live sprites, and features not visible to the platform.
 */
 
#ifndef FMN_SPRITE_H
#define FMN_SPRITE_H

#include "app/fmn_platform.h"

#define FMN_SPRITE_ARGV_SIZE 4

#define FMN_PHYSICS_MOTION   0x01 /* Automatic motion per (velx,vely,veldecay) */
#define FMN_PHYSICS_EDGE     0x02 /* Collide against screen edges. */
#define FMN_PHYSICS_SPRITES  0x04 /* Collide against other sprites, if they also have this flag. */
#define FMN_PHYSICS_SOLID    0x10 /* Collide against SOLID (1) grid cells. */
#define FMN_PHYSICS_HOLE     0x20 /* '' HOLE (2) */
#define FMN_PHYSICS_GRID (FMN_PHYSICS_SOLID|FMN_PHYSICS_HOLE)
#define FMN_PHYSICS_BLOWABLE 0x40 /* Wind can move it. */

struct fmn_sprite {
  FMN_SPRITE_HEADER
  
  //TODO Why are we copying out all the callbacks instead of pointing to a controller?
  // There must have been a reason for that, but now I forget. -aks 2023-02-27
  
  // Called each master update, if set.
  void (*update)(struct fmn_sprite *sprite,float elapsed);
  
  /* Called when another sprite collides against me.
   * (dir) is a cardinal, the direction (presser) is trying to move.
   * This will be called after all 'update' hooks, during physics resolution.
   */
  void (*pressure)(struct fmn_sprite *sprite,struct fmn_sprite *presser,uint8_t dir);
  void (*static_pressure)(struct fmn_sprite *sprite,struct fmn_sprite *null_dummy,uint8_t dir);
  
  void (*hero_collision)(struct fmn_sprite *sprite,struct fmn_sprite *hero);
  
  /* Notify sprite that the hero has done something, possibly scoped to this sprite specifically.
   * FMN_ITEM_WAND for both spells and songs. (qualifier) is the spellid. All sprites onscreen get notified.
   * FMN_ITEM_PITCHER if the pitcher is used with this sprite in focus.
   *   (qualifier) is a nonzero FMN_PITCHER_CONTENT_* if we're pouring.
   *   If zero, we're collecting. Return zero or a content to provide.
   * FMN_ITEM_BELL broadcast, no qualifier or return.
   * FMN_ITEM_SEED local, no qualifier. Return nonzero to consume it.
   * FMN_ITEM_COIN ''. You make a sound effect if warranted.
   */
  int16_t (*interact)(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier);
  
  /* For normal wind interaction, just add FMN_PHYSICS_BLOWABLE to (physics).
   * Implement this if you need to react to wind in some peculiar way.
   */
  void (*wind)(struct fmn_sprite *sprite,float dx,float dy);
  
  // Reference data recorded at spawn point.
  uint16_t spriteid;
  uint8_t argv[FMN_SPRITE_ARGV_SIZE];
  
  // Generic physics.
  uint8_t physics; // Bitfields, FMN_PHYSICS_*
  float velx,vely;
  float veldecay; // Linear velocity decay in m/s**2.
  float radius;
  uint8_t invmass; // 1/mass, 0 if infinite
  float pvx,pvy;
  
  // Lower layers draw first. The hero is at layer 0x80.
  uint8_t layer;
};

// Drop all sprites cold.
void fmn_sprites_clear();

/* Make a new sprite if possible.
 * Note that sprite coordinates and map cell coordinates are the same thing, just sprites are floating-point.
 */
struct fmn_sprite *fmn_sprite_spawn(
  float x,float y,
  uint16_t spriteid,
  const uint8_t *cmdv,uint16_t cmdc, // from sprite definition
  const uint8_t *argv,uint8_t argc // from spawn point
);

int fmn_sprites_for_each(int (*cb)(struct fmn_sprite *sprite,void *userdata),void *userdata);

void fmn_sprites_update(float elapsed,float hero_elapsed);

void fmn_sprite_apply_force(struct fmn_sprite *sprite,float dx,float dy);

void fmn_sprites_sort_partial();

void fmn_sprite_kill(struct fmn_sprite *sprite);

/* Sprite controllers.
 ***************************************************************/

// Be mindful of formatting here; this chunk is read by our mksprites tool.
#define FMN_SPRCTL_dummy          0
#define FMN_SPRCTL_hero           1
#define FMN_SPRCTL_pushblock      2
#define FMN_SPRCTL_alphablock     3
#define FMN_SPRCTL_hazard         4
#define FMN_SPRCTL_treasure       5
#define FMN_SPRCTL_soulball       6
#define FMN_SPRCTL_firenozzle     7
#define FMN_SPRCTL_treadle        8
#define FMN_SPRCTL_gate           9
#define FMN_SPRCTL_coin          10
#define FMN_SPRCTL_cow           11
#define FMN_SPRCTL_beehive       12
#define FMN_SPRCTL_bee           13
#define FMN_SPRCTL_seed          14
#define FMN_SPRCTL_zzz           15
#define FMN_SPRCTL_crow          16

#define FMN_FOR_EACH_SPRCTL \
  _(dummy) \
  _(hero) \
  _(pushblock) \
  _(alphablock) \
  _(hazard) \
  _(treasure) \
  _(soulball) \
  _(firenozzle) \
  _(treadle) \
  _(gate) \
  _(coin) \
  _(cow) \
  _(beehive) \
  _(bee) \
  _(seed) \
  _(zzz) \
  _(crow)
  
struct fmn_sprite_controller {
  void (*init)(struct fmn_sprite *sprite);
  void (*update)(struct fmn_sprite *sprite,float elapsed);
  void (*pressure)(struct fmn_sprite *sprite,struct fmn_sprite *presser,uint8_t dir);
  void (*static_pressure)(struct fmn_sprite *sprite,struct fmn_sprite *null_dummy,uint8_t dir);
  void (*hero_collision)(struct fmn_sprite *sprite,struct fmn_sprite *hero);
  int16_t (*interact)(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier);
  void (*wind)(struct fmn_sprite *sprite,float dx,float dy);
};

#define _(tag) extern const struct fmn_sprite_controller fmn_sprite_controller_##tag;
FMN_FOR_EACH_SPRCTL
#undef _

const struct fmn_sprite_controller *fmn_sprite_controller_by_id(uint16_t id);

void fmn_sprite_generate_soulballs(float x,float y,uint8_t c);
void fmn_sprite_generate_noparam(uint16_t sprctl,float x,float y);

#endif

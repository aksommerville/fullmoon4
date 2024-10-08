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
#define FMN_PHYSICS_GRID     0x30
#define FMN_PHYSICS_BLOWABLE 0x40 /* Wind can move it. */

struct fmn_sprite {
  FMN_SPRITE_HEADER
  
  // Called each master update, if set.
  void (*update)(struct fmn_sprite *sprite,float elapsed);
  
  /* Called when another sprite collides against me.
   * (dir) is a cardinal, the direction (presser) is trying to move.
   * This will be called after all 'update' hooks, during physics resolution.
   */
  void (*pressure)(struct fmn_sprite *sprite,struct fmn_sprite *presser,uint8_t dir);
  void (*static_pressure)(struct fmn_sprite *sprite,struct fmn_sprite *null_dummy,uint8_t dir);
  
  /* Should only be used by non-physical sprites.
   * If you have FMN_PHYSICS_SPRITES, you'll get these calls unpredictably, since regular collision detection might escape them.
   * Use (pressure) and check (presser->style).
   */
  void (*hero_collision)(struct fmn_sprite *sprite,struct fmn_sprite *hero);
  
  /* Notify sprite that the hero has done something, possibly scoped to this sprite specifically.
   * FMN_ITEM_WAND for both spells and songs. (qualifier) is the spellid. All sprites onscreen get notified.
   * FMN_ITEM_PITCHER if the pitcher is used with this sprite in focus.
   *   (qualifier) is a nonzero FMN_PITCHER_CONTENT_* if we're pouring.
   *   If zero, we're collecting. Return zero or a content to provide.
   * FMN_ITEM_BELL broadcast, no qualifier or return.
   * FMN_ITEM_SEED local, no qualifier. Return nonzero to consume it.
   * FMN_ITEM_COIN ''. You make a sound effect if warranted.
   * FMN_ITEM_CHALK broadcast, no qualifier or return, after the chalk modal dismisses.
   * FMN_ITEM_UMBRELLA. qualifier is FMN_DIR_* (always same as facedir). When an injury is deflected.
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
  float hbw,hbe,hbn,hbs; // hitbox edges relative to (x,y). (radius) overrides if >0
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

/* A "defunct" sprite is frozen in memory but will not interact with the world, until refuncted.
 */
void fmn_sprite_defunct(struct fmn_sprite *sprite);
void fmn_sprite_refunct(struct fmn_sprite *sprite);

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
#define FMN_SPRCTL_saw           17
#define FMN_SPRCTL_firewall      18
#define FMN_SPRCTL_chalkguard    19
#define FMN_SPRCTL_raccoon       20
#define FMN_SPRCTL_missile       21
#define FMN_SPRCTL_rat           22
#define FMN_SPRCTL_rabbit        23
#define FMN_SPRCTL_toast         24
#define FMN_SPRCTL_farmer        25
#define FMN_SPRCTL_moonsong      26
#define FMN_SPRCTL_pitchfork     27
#define FMN_SPRCTL_lobster       28
#define FMN_SPRCTL_tolltroll     29
#define FMN_SPRCTL_lizard        30
#define FMN_SPRCTL_wildflower    31
#define FMN_SPRCTL_conveyor      32
#define FMN_SPRCTL_skyleton      33
#define FMN_SPRCTL_ghost         34
#define FMN_SPRCTL_duck          35
#define FMN_SPRCTL_seamonster    36
#define FMN_SPRCTL_throwswitch   37
#define FMN_SPRCTL_decoanimal    38
#define FMN_SPRCTL_scarydoor     39
#define FMN_SPRCTL_werewolf      40
#define FMN_SPRCTL_floorfire     41
#define FMN_SPRCTL_deadwitch     42
#define FMN_SPRCTL_losthat       43
#define FMN_SPRCTL_scoreboard    44
#define FMN_SPRCTL_trickfloor    45
#define FMN_SPRCTL_tree          46
#define FMN_SPRCTL_pamcake       47
#define FMN_SPRCTL_magicdoor     48
#define FMN_SPRCTL_bittattle     49
#define FMN_SPRCTL_panda         50
#define FMN_SPRCTL_watchduck     51
#define FMN_SPRCTL_pumpkin       52
#define FMN_SPRCTL_otp           53
#define FMN_SPRCTL_treadmill     54
#define FMN_SPRCTL_tripwire      55
#define FMN_SPRCTL_hattroll      56
#define FMN_SPRCTL_static_educator 57
#define FMN_SPRCTL_tallbanner    58
#define FMN_SPRCTL_book          59
#define FMN_SPRCTL_musicteacher  60
#define FMN_SPRCTL_musicstand    61
#define FMN_SPRCTL_bucklock      62
#define FMN_SPRCTL_coincollector 63
#define FMN_SPRCTL_orphan        64
#define FMN_SPRCTL_goody         65
#define FMN_SPRCTL_slideshow     66
#define FMN_SPRCTL_dragon        67
#define FMN_SPRCTL_ghostgen      68
#define FMN_SPRCTL_nffish        69
#define FMN_SPRCTL_arcade        70
#define FMN_SPRCTL_anim2         71

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
  _(crow) \
  _(saw) \
  _(firewall) \
  _(chalkguard) \
  _(raccoon) \
  _(missile) \
  _(rat) \
  _(rabbit) \
  _(toast) \
  _(farmer) \
  _(moonsong) \
  _(pitchfork) \
  _(lobster) \
  _(tolltroll) \
  _(lizard) \
  _(wildflower) \
  _(conveyor) \
  _(skyleton) \
  _(ghost) \
  _(duck) \
  _(seamonster) \
  _(throwswitch) \
  _(decoanimal) \
  _(scarydoor) \
  _(werewolf) \
  _(floorfire) \
  _(deadwitch) \
  _(losthat) \
  _(scoreboard) \
  _(trickfloor) \
  _(tree) \
  _(pamcake) \
  _(magicdoor) \
  _(bittattle) \
  _(panda) \
  _(watchduck) \
  _(pumpkin) \
  _(otp) \
  _(treadmill) \
  _(tripwire) \
  _(hattroll) \
  _(static_educator) \
  _(tallbanner) \
  _(book) \
  _(musicteacher) \
  _(musicstand) \
  _(bucklock) \
  _(coincollector) \
  _(orphan) \
  _(goody) \
  _(slideshow) \
  _(dragon) \
  _(ghostgen) \
  _(nffish) \
  _(arcade) \
  _(anim2)
  
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

void fmn_sprite_generate_soulballs(float x,float y,uint8_t c,uint8_t return_to_sender);
void fmn_sprite_generate_soilballs(float x,float y); // like soulballs but for plants
struct fmn_sprite *fmn_sprite_generate_zzz(struct fmn_sprite *source);
void fmn_sprite_kill_zzz(struct fmn_sprite *source);
struct fmn_sprite *fmn_sprite_generate_noparam(uint16_t sprctl,float x,float y);
struct fmn_sprite *fmn_sprite_generate_toast(float x,float y,uint8_t imageid,uint8_t tileid,uint8_t xform);
struct fmn_sprite *fmn_sprite_generate_enchantment(struct fmn_sprite *source,uint8_t persistent);
void fmn_sprite_kill_enchantment(struct fmn_sprite *source);

/* Defuncts (victim) and creates a pumpkin in its place.
 * Returns that pumpkin on success.
 */
struct fmn_sprite *fmn_sprite_pumpkinize(struct fmn_sprite *victim);

void fmn_sprite_force_unpumpkin(struct fmn_sprite *victim);

#endif

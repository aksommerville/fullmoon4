/* datan_internal.h
 * "Data Analysis"
 * Meant to replace the "verify" tool, which is kind of a mess of Javascript.
 * Reads finished archives and validates mostly business-level stuff aggressively.
 *
 * We use fmn_datafile, same as the native-build games.
 * That makes this tool lean, and guarantees we're processing it realistically.
 * But it does eliminate some opportunities for detailed analysis of archive structure faults.
 * (I don't imagine that's a big deal; we're not reading archives from unknown sources here).
 */
 
#ifndef DATAN_INTERNAL_H
#define DATAN_INTERNAL_H

#include "opt/datafile/fmn_datafile.h"
#include "tool/common/serial/serial.h"
#include "app/fmn_platform.h" /* defines only */
#include "app/sprite/fmn_sprite.h" /* defines only */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

extern struct datan {
  const char *exename;
  const char *srcpath;
  const char *arpath;
  const char *tileusage; // If set, path to output HTML file. Don't run normal validation.
  struct fmn_datafile *datafile;
  
  // Chalk definitions. Unsorted (preserving order of input file, not that we care).
  struct datan_chalk {
    int codepoint;
    int bits;
  } *chalkv;
  int chalkc,chalka;
  
  // Gsbit definitions. Sorted by id. Text lives in the big dump.
  struct datan_gsbit {
    int id;
    const char *name;
    int namec;
  } *gsbitv;
  int gsbitc,gsbita;
  char *gsbittext;
  
  // Registry of live resources.
  // This should include map and sprite. Definitely don't need string.
  // Others, not sure.
  // Serial data is available via (datafile), for the same resource set.
  struct datan_res {
    uint16_t type;
    uint16_t qualifier;
    uint32_t id;
    void *obj;
    void (*del)(void *obj); // required
  } *resv;
  int resc,resa;
  
  // --tileusage
  struct datan_tuentry {
    uint8_t imageid; // maps can only refer to the first 256 images
    uint8_t usage[256>>3]; // little-endian bits per tile
  } *tuentryv;
  int tuentryc,tuentrya;
} datan;

const char *fmn_restype_repr(uint16_t type);
uint16_t fmn_restype_eval(const char *src,int srcc);

int datan_chalk_codepoint_for_bits(int bits);
int datan_chalk_bits_for_codepoint(int codepoint,int p);
int datan_chalk_add(int bits,int codepoint); // caller must assert valid

int datan_gsbit_by_name(const char *name,int namec);
int datan_gsbit_search_id(int id);
int datan_gsbit_insert(int p,int id,const char *name_borrow,int namec);

void datan_res_clear();
int datan_res_search(uint16_t type,uint16_t qualifier,uint32_t id);
struct datan_res *datan_res_insert(int p,uint16_t type,uint16_t qualifier,uint32_t id);
int datan_res_add(uint16_t type,uint16_t qualifier,uint32_t id,void *obj,void *del); // fails if already existing
void *datan_res_get(uint16_t type,uint16_t qualifier,uint32_t id);

/* Source validators.
 * (srcpath) is valid when these run.
 * (arpath,datafile) must be ignored.
 */
int datan_acquire_gsbit();
int datan_acquire_chalk();

int datan_tileusage(); // Call once per archive, after datan_validate_individual_resources
int datan_tileusage_finish();

/* Archive validators.
 * (arpath,datafile) are valid when these run.
 */
int datan_validate_individual_resources();
int datan_validate_spawn_points();
int datan_validate_res_id_continuity();
int datan_validate_cross_qualifier();
int datan_validate_save_points();
int datan_validate_song_choice();
int datan_validate_blowback();
int datan_validate_indoor_outdoor_boundaries();
int datan_validate_tileprops_against_image();
int datan_validate_buried_things();
int datan_validate_map_refs();
int datan_validate_reachability();

/* Individual resource validators, single serial only.
 */
int datan_song_validate_serial(uint16_t qualifier,uint32_t id,const void *v,int c);
int datan_tileprops_validate_serial(uint16_t qualifier,uint32_t id,const void *v,int c);
int datan_string_validate_serial(uint16_t qualifier,uint32_t id,const void *v,int c);

/* Complex resources.
 * All of our data is read-only.
 * So we speed things up by pointing directly into the serial input where possible.
 * Any serial data you pass to a ctor here may be borrowed.
 * As a general rule, 'new' should validate only to the extent necessary for structural purposes.
 * Broader single-object validation should be a separate function post init.
 ************************************************************/
 
struct datan_map {
  uint16_t qualifier;
  uint32_t id;
  const uint8_t *v; // FMN_COLC*FMN_ROWC
  const uint8_t *addl; // Encoded commands.
  int addlc;
  
  // Fields extracted at validate:
  uint8_t songid;
  uint8_t imageid;
  uint8_t saveto;
  uint8_t winddir;
  uint8_t flags;
  uint8_t ref;
  uint16_t neighborw;
  uint16_t neighbore;
  uint16_t neighborn;
  uint16_t neighbors;
  uint8_t herox,heroy,spellid;
  struct datan_map_spawn {
    uint8_t x,y;
    uint16_t spriteid;
    uint8_t arg0,arg1,arg2;
    struct datan_sprite *sprite; // borrowed. Null until after the individual stage.
  } *spawnv;
  int spawnc,spawna;
  
  // Available after datan_validate_spawn_points:
  const uint8_t *cellphysics; // 256
};

void datan_map_del(struct datan_map *map);
struct datan_map *datan_map_new(uint16_t qualifier,uint32_t id,const void *src,int srcc);
int datan_map_validate(struct datan_map *map);

/* Opcodes below 0xc0 have intrinsic length and (c) is redundant.
 */
int datan_map_for_each_command(
  struct datan_map *map,
  int (*cb)(uint8_t opcode,const uint8_t *v,int c,void *userdata),
  void *userdata
);

int datan_map_cell_is_solid(const struct datan_map *map,int x,int y);
uint8_t datan_map_get_cellphysics(const struct datan_map *map,int x,int y);
int datan_map_rect_contains_solid(const struct datan_map *map,int x,int y,int w,int h);
int datan_map_rect_entirely_solid(const struct datan_map *map,int x,int y,int w,int h);

/* Trigger callback for each edge and door with a nonzero mapid.
 * Optionally skip cardinal neighbors if our side of the border is fully solid.
 */
int datan_map_for_each_neighbor(
  struct datan_map *map,
  int check_solid_edges,
  int (*cb)(struct datan_map *map,uint16_t neighbor_map_id,void *userdata),
  void *userdata
);

struct datan_sprite {
  uint16_t qualifier;
  uint32_t id;
  const uint8_t *addl; // Encoded commands. (that's the whole resource)
  int addlc;
  // Fields extracted from (addl) at validate:
  uint8_t imageid;
  uint8_t tileid;
  uint8_t xform;
  uint8_t style; // FMN_SPRITE_STYLE_*
  uint8_t physics;
  uint8_t invmass;
  uint8_t layer;
  uint16_t veldecay; // u8.8
  uint16_t radius; // u8.8
  uint16_t controller;
  uint8_t bv[FMN_SPRITE_BV_SIZE];
  // Extracted from source before validation:
  uint16_t arg0type; // if nonzero, spawn point argument is a resource id
  uint16_t arg1type;
  uint16_t arg2type;
};

void datan_sprite_del(struct datan_sprite *sprite);
struct datan_sprite *datan_sprite_new(uint16_t qualifier,uint32_t id,const void *src,int srcc);
int datan_sprite_validate(struct datan_sprite *sprite);

/* Opcodes under 0xa0 (all currently defined ones) have an intrinsic body length, and (c) is redundant.
 */
int datan_sprite_for_each_command(
  struct datan_sprite *sprite,
  int (*cb)(uint8_t opcode,const uint8_t *v,int c,void *userdata),
  void *userdata
);

int datan_sprites_acquire_argtype();

#endif

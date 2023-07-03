/* fmstore.h
 * Storage dump for game state, things that belong only to the platform.
 * At the beginning, I was hoping to keep Full Moon able to run on extremely low-memory platforms,
 * but that doesn't seem to have worked out, so now separating this stuff from the rest of the global state seems silly.
 * But whatever.
 * Basically, any game state that survives beyond a screen of play should live here.
 */
 
#ifndef FMSTORE_H
#define FMSTORE_H

struct fmstore;

void fmstore_del(struct fmstore *fmstore);

struct fmstore *fmstore_new();

// Platform should call when entering a map:
void fmstore_write_plants_to_globals(struct fmstore *fmstore,uint16_t mapid);
void fmstore_write_sketches_to_globals(struct fmstore *fmstore,uint16_t mapid);

// Platform should call when exiting a map:
void fmstore_read_plants_from_globals(struct fmstore *fmstore,uint16_t mapid);
void fmstore_read_sketches_from_globals(struct fmstore *fmstore,uint16_t mapid);

int fmstore_for_each_plant(
  struct fmstore *fmstore,
  int (*cb)(uint16_t mapid,struct fmn_plant *plant,void *userdata),
  void *userdata
);
int fmstore_for_each_sketch(
  struct fmstore *fmstore,
  int (*cb)(uint16_t mapid,struct fmn_sketch *sketch,void *userdata),
  void *userdata
);

/* This interface should only be used at load/save, not during regular play.
 * "write to globals" or "read from globals" during play.
 */
void fmstore_clear_plants(struct fmstore *fmstore);
void fmstore_clear_sketches(struct fmstore *fmstore);
struct fmn_plant *fmstore_add_plant(struct fmstore *fmstore,uint16_t mapid,uint8_t x,uint8_t y);
struct fmn_sketch *fmstore_add_sketch(struct fmstore *fmstore,uint16_t mapid,uint8_t x,uint8_t y);

#endif

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

#endif

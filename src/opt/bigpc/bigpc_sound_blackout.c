#include "bigpc_internal.h"

// Prevent sound effects starting within 12 Hz of each other.
// (based on real time, not audio output time, because it's easier to know).
#define BIGPC_SOUND_BLACKOUT_US 80000

/* Drop all records older than the blackout period.
 * Records are implicitly sorted old-to-new because we add only to the end.
 */
 
static void bigpc_sound_blackout_graduate() {
  int rmc=0;
  while (
    (rmc<bigpc.sound_blackoutc)&&
    (bigpc.sound_blackoutv[rmc].realtime<=bigpc.clock.last_real_time_us)
  ) rmc++;
  if (rmc) {
    bigpc.sound_blackoutc-=rmc;
    memmove(bigpc.sound_blackoutv,bigpc.sound_blackoutv+rmc,sizeof(struct bigpc_sound_blackout)*bigpc.sound_blackoutc);
  }
}

/* Is this sound effect present?
 */
 
static int bigpc_sound_blackout_has(uint16_t sfxid) {
  const struct bigpc_sound_blackout *q=bigpc.sound_blackoutv;
  int i=bigpc.sound_blackoutc;
  for (;i-->0;q++) if (q->sfxid==sfxid) return 1;
  return 0;
}

/* Append to list.
 */
 
static int bigpc_sound_blackout_append(uint16_t sfxid) {
  if (bigpc.sound_blackoutc>=bigpc.sound_blackouta) {
    int na=bigpc.sound_blackouta+16;
    if (na>INT_MAX/sizeof(struct bigpc_sound_blackout)) return -1;
    void *nv=realloc(bigpc.sound_blackoutv,sizeof(struct bigpc_sound_blackout)*na);
    if (!nv) return -1;
    bigpc.sound_blackoutv=nv;
    bigpc.sound_blackouta=na;
  }
  struct bigpc_sound_blackout *blackout=bigpc.sound_blackoutv+bigpc.sound_blackoutc++;
  blackout->sfxid=sfxid;
  blackout->realtime=bigpc.clock.last_real_time_us+BIGPC_SOUND_BLACKOUT_US;
  return 0;
}

/* Check and update blackout.
 */

int bigpc_check_sound_blackout(uint16_t sfxid) {
  bigpc_sound_blackout_graduate();
  if (bigpc_sound_blackout_has(sfxid)) return 0;
  bigpc_sound_blackout_append(sfxid);
  return 1;
}

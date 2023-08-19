#ifndef GENIOC_INTERNAL_H
#define GENIOC_INTERNAL_H

#include "genioc.h"
#include "../bigpc/bigpc.h"
#include <stdio.h>
#include <stdint.h>

#if FMN_USE_mswin
  #include <Windows.h>
#endif

#define GENIOC_FRAME_RATE_MAX 100000
#define GENIOC_DELAY_LIMIT_US 50000 /* Assume the clock is broken and don't delay longer than this. Effective minimum frame rate 20 hz */

extern struct genioc {
  int quit;
  
  // clock
  int frame_rate;
  int64_t us_per_frame;
  int64_t next_update_time;
  int64_t first_update_time;
  int64_t framec;
  int clock_faultc;
  int64_t cpu_time_start;
  
} genioc;

/* Updating clock forces a delay if we seem to be running fast.
 * The plan is to tolerate both vsync-blocking and non-blocking clients.
 * We keep enough data to make a sensible report at quit, but bigpc is also doing that so we stay quiet.
 */
void genioc_clock_init();
void genioc_clock_update();

#endif

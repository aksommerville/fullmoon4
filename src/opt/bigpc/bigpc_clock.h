/* bigpc_clock.h
 */
 
#ifndef BIGPC_CLOCK_H
#define BIGPC_CLOCK_H

#include <stdint.h>

int64_t bigpc_now_us();
double bigpc_now_real_s();
double bigpc_now_cpu_s();

struct bigpc_clock {
  int64_t last_real_time_us;
  uint32_t last_game_time_ms; // Includes modals and transitions. Probably don't use.
  int32_t skew_us;
  
  /* We track faults, that's any moment when real and game time unexpectedly skew.
   * Owner is free to clear these whenever.
   * (underflowc) means you updated too fast and we force game time forward faster than real time,
   *   rather than reporting a tiny interval that could lead to rounding error in the client.
   *   One underflow at startup is normal.
   * (overflowc) means you updated too slow, or forgot to declare a skip, and we report less time elapsed than actually did.
   *   Reporting too much elapsed time in one frame could lead to missed collisions in the client.
   * (faultc) are things like negative or unreasonably long real-time intervals.
   *   If we report a real fault, we also reset the real-time clock and probably miss a beat.
   * (wrapc) is the worst of all, game time has exceeded 32 bits. That takes about 50 days.
   */
  uint32_t underflowc;
  uint32_t overflowc;
  uint32_t faultc;
  uint32_t wrapc;
  
  // For your curiosity only.
  int64_t first_real_time_us;
  int framec,skipc; // Count of "update" and "skip" calls.

  // Tracking CPU consumption, a separate concern but obviously "clocky".
  double starttime_real,starttime_cpu;
};

void bigpc_clock_reset(struct bigpc_clock *clock);

/* At each update cycle -- presumably video frames -- you should call either update or skip.
 * Updating returns the current absolute game time in ms.
 * Skipping lets us know the game is not updating, and we allow real time to slip ahead of game time.
 * We also sanitize reported game time. It's always something safe to report to the client.
 */
uint32_t bigpc_clock_update(struct bigpc_clock *clock);
void bigpc_clock_skip(struct bigpc_clock *clock);

// Start time is for the process (trimming startup and teardown). It never resets.
void bigpc_clock_capture_start_time(struct bigpc_clock *clock);

double bigpc_clock_estimate_cpu_load(const struct bigpc_clock *clock);

#endif

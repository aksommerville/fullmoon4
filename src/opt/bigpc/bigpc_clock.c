#include "bigpc_clock.h"
#include <sys/time.h>
#include <time.h>

// This is more about preventing 64-to-32-bit overflow than about timekeeping.
// Five seconds is an extremely long interval between updates.
#define BIGPC_UNREASONABLE_REAL_TIME_INTERVAL_US 5000000

/* The preferred interval is 16.667 ms.
 * Clients should tolerate double that, but not much further.
 */
#define BIGPC_CLOCK_MIN_GAME_INTERVAL_MS 10
#define BIGPC_CLOCK_MAX_GAME_INTERVAL_MS 50

/* Current real time in microseconds.
 */

int64_t bigpc_now_us() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (int64_t)tv.tv_sec*1000000ll+tv.tv_usec;
}

double bigpc_now_real_s() {
  struct timespec tv={0};
  clock_gettime(CLOCK_REALTIME,&tv);
  return tv.tv_sec+tv.tv_nsec/1000000000.0;
}

double bigpc_now_cpu_s() {
  struct timespec tv={0};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tv);
  return tv.tv_sec+tv.tv_nsec/1000000000.0;
}

/* Reset clock.
 */

void bigpc_clock_reset(struct bigpc_clock *clock) {
  clock->last_real_time_us=bigpc_now_us();
  clock->first_real_time_us=clock->last_real_time_us;
  clock->last_game_time_ms=0;
  clock->skew_us=0;
  clock->underflowc=0;
  clock->overflowc=0;
}

/* Start time. Should only be called once.
 */
 
void bigpc_clock_capture_start_time(struct bigpc_clock *clock) {
  clock->starttime_real=bigpc_now_real_s();
  clock->starttime_cpu=bigpc_now_cpu_s();
}

/* Update real-time clock, account for real-time faults, and
 * return real elapsed time in microseconds.
 */
 
static int64_t bigpc_clock_update_real_time(struct bigpc_clock *clock) {
  int64_t now=bigpc_now_us();
  int64_t elapsed_us=now-clock->last_real_time_us;
  clock->last_real_time_us=now;
  if (elapsed_us<0) {
    clock->faultc++;
    clock->skew_us=0;
    return 0;
  }
  if (elapsed_us>BIGPC_UNREASONABLE_REAL_TIME_INTERVAL_US) {
    clock->faultc++;
    clock->skew_us=0;
    return BIGPC_UNREASONABLE_REAL_TIME_INTERVAL_US;
  }
  elapsed_us+=clock->skew_us;
  clock->skew_us=0;
  return elapsed_us;
}

/* Update, game is running.
 */

uint32_t bigpc_clock_update(struct bigpc_clock *clock) {
  clock->framec++;
  int64_t elapsed_us=bigpc_clock_update_real_time(clock);
  int32_t elapsed_ms=elapsed_us/1000;
  clock->skew_us=elapsed_us%1000;
  if (elapsed_ms<BIGPC_CLOCK_MIN_GAME_INTERVAL_MS) {
    clock->underflowc++;
    elapsed_ms=BIGPC_CLOCK_MIN_GAME_INTERVAL_MS;
  } else if (elapsed_ms>BIGPC_CLOCK_MAX_GAME_INTERVAL_MS) {
    clock->overflowc++;
    elapsed_ms=BIGPC_CLOCK_MAX_GAME_INTERVAL_MS;
  }
  if (clock->last_game_time_ms>0xffffffffu-elapsed_us) {
    clock->wrapc++;
    clock->last_game_time_ms=0;
  }
  clock->last_game_time_ms+=elapsed_ms;
  return clock->last_game_time_ms;
}

/* Update, game is not running.
 */
 
void bigpc_clock_skip(struct bigpc_clock *clock) {
  clock->skipc++;
  int64_t elapsed_us=bigpc_clock_update_real_time(clock);
  clock->skew_us=0;
}

/* Estimate CPU load.
 */
 
double bigpc_clock_estimate_cpu_load(const struct bigpc_clock *clock) {
  double elapsed_real=bigpc_now_real_s()-clock->starttime_real;
  double elapsed_cpu=bigpc_now_cpu_s()-clock->starttime_cpu;
  return elapsed_cpu/elapsed_real;
}

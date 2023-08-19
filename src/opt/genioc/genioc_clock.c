#include "genioc_internal.h"
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

/* Current absolute time in microseconds.
 */
 
static int64_t genioc_now() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (int64_t)tv.tv_sec*1000000ll+tv.tv_usec;
}

/* Current CPU time in microseconds.
 */
 
static int64_t genioc_now_cpu() {
  #if FMN_USE_mswin
    // MinGW doesn't have clock_gettime, or I'm missing a library, or something. Whatever. CPU timing isn't critical.
    struct timeval tv={0};
    gettimeofday(&tv,0);
    return (int64_t)tv.tv_sec*1000000ll+tv.tv_usec;
  #else
    struct timespec tv={0};
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tv);
    return (int64_t)tv.tv_sec*1000000ll+tv.tv_nsec/1000;
  #endif
}

/* Init.
 */
 
void genioc_clock_init() {
  genioc.first_update_time=genioc_now();
  genioc.next_update_time=0;
  genioc.cpu_time_start=genioc_now_cpu();
}

/* Update.
 */
 
void genioc_clock_update() {
  if (!genioc.frame_rate) return;
  genioc.framec++;
  int64_t now=genioc_now();
  if (now<genioc.next_update_time) {
    int64_t delay=genioc.next_update_time-now;
    if (delay>GENIOC_DELAY_LIMIT_US) {
      genioc.clock_faultc++;
      genioc.next_update_time=now+genioc.us_per_frame;
      return;
    }
    #if FMN_USE_mswin
      // mingw on my nuc does have usleep, but on the dell it does not
      Sleep(delay/1000);
    #else
      usleep(delay);
    #endif
  }
  genioc.next_update_time+=genioc.us_per_frame;
  if (genioc.next_update_time<=now) {
    genioc.next_update_time=now+genioc.us_per_frame;
  }
}

/* Set frame rate (public).
 */
 
void genioc_set_frame_rate(int hz) {
  if (hz<=0) hz=0;
  else if (hz>GENIOC_FRAME_RATE_MAX) hz=GENIOC_FRAME_RATE_MAX;
  if (hz==genioc.frame_rate) return;
  genioc.frame_rate=hz;
  if (hz) {
    genioc.us_per_frame=1000000/hz;
  } else {
    genioc.us_per_frame=0;
  }
}

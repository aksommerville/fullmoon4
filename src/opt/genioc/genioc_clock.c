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
  struct timespec tv={0};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tv);
  return (int64_t)tv.tv_sec*1000000ll+tv.tv_nsec/1000;
}

/* Report.
 */
 
void genioc_clock_report() {
  if (genioc.framec<1) return;
  double elapsed=(genioc.next_update_time-genioc.first_update_time)/1000000.0;
  if (elapsed<1.0) return;
  double average=genioc.framec/elapsed;
  int64_t cpu_now=genioc_now_cpu();
  double elapsed_cpu=(cpu_now-genioc.cpu_time_start)/1000000.0;
  double load=elapsed_cpu/elapsed;
  fprintf(stderr,
    "%lld frames in %.03f s, average frame rate %.03f Hz. Average CPU load %.06f.\n",
    (long long)genioc.framec,elapsed,average,load
  );
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
    usleep(delay);
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

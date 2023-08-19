#include "assist_internal.h"
#include <sys/time.h>
#include <time.h>

/* Current time, both real and CPU.
 */
 
void assist_now(struct assist_timestamp *t) {

  struct timeval tv={0};
  gettimeofday(&tv,0);
  t->real=(double)tv.tv_sec+(double)tv.tv_usec/1000000.0;

  #if FMN_USE_mswin
    t->cpu=t->real;
  #else
    struct timespec ts={0};
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&ts);
    t->cpu=(double)ts.tv_sec+(double)ts.tv_nsec/1000000000.0;
  #endif
}

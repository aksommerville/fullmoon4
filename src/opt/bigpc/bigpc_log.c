#include "bigpc_internal.h"
#include "app/fmn_game.h"
#include "app/hero/fmn_hero.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

/* Log.
 */
 
void fmn_log(const char *fmt,...) {
  if (!fmt||!fmt[0]) return;
  char buf[512];
  va_list vargs;
  va_start(vargs,fmt);
  int bufc=vsnprintf(buf,sizeof(buf),fmt,vargs);
  if ((bufc<0)||(bufc>=sizeof(buf))) { // sic >= not > because vsnprintf can negative-terminate
    fprintf(stderr,"fmn_log: Message too long. Format string: %.100s\n",fmt);
  } else {
    fprintf(stderr,"%.*s\n",bufc,buf);
  }
}

/* Business logging.
 */
 
void fmn_log_event(const char *key,const char *fmt,...) {
  if (!bigpc.logfile) return;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  char pfx[256];
  int pfxc=snprintf(pfx,sizeof(pfx),
    "%u:%u+%u:%lld@%d,%d,%d;%s",
    bigpc.clock.last_game_time_ms,
    bigpc.clock.framec,
    bigpc.clock.skipc,
    (long long)(bigpc.clock.last_real_time_us-bigpc.clock.first_real_time_us),
    bigpc.mapid,(int)herox,(int)heroy,
    key
  );
  if ((pfxc<0)||(pfxc>=sizeof(pfx))) return;
  va_list vargs;
  va_start(vargs,fmt);
  char msgs[256];
  const char *msg=msgs;
  int msgc=vsnprintf(msgs,sizeof(msgs),fmt,vargs);
  if ((msgc<0)||(msgc>=sizeof(msgs))) msg=fmt;
  fprintf(bigpc.logfile,"%.*s %.*s\n",pfxc,pfx,msgc,msg);
}

/* Begin logging.
 */
 
int bigpc_log_init() {

  if (bigpc.config.log_path) {
    time_t now=time(0);
    struct tm tm={0};
    struct tm *tmp=localtime(&now); // mswin apparently doesn't have localtime_r
    if (tmp) tm=*tmp;
    char path[1024];
    int pathc=snprintf(path,sizeof(path),
      "%s-%04d%02d%02d%02d%02d%02d.txt",
      bigpc.config.log_path,
      1900+tm.tm_year,
      1+tm.tm_mon,
      tm.tm_mday,
      tm.tm_hour,
      tm.tm_min,
      tm.tm_sec
    );
    if (!(bigpc.logfile=fopen(path,"w"))) {
      fprintf(stderr,"%s: Failed to open log file for writing.\n",path);
      return -2;
    }
    fprintf(stderr,"%s: Opened business log file.\n",path);
  } else {
    fprintf(stderr,"%s: log_path unset, will not generate business logs.\n",__func__);
  }
  return 0;
}

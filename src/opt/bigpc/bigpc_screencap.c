#include "bigpc_internal.h"
#include "opt/png/png.h"
#include "opt/datafile/fmn_datafile.h"
#include <time.h>

/* Take a screencap.
 */
 
void bigpc_cap_screen() {
  if (!bigpc.render->type->read_framebuffer) return;
  void *rgba=0;
  int w=0,h=0;
  if (bigpc.render->type->read_framebuffer(&rgba,&w,&h,bigpc.render)<0) return;
  if (!rgba) return;
  struct png_image image={
    .pixels=rgba,
    .ownpixels=0,
    .w=w,
    .h=h,
    .stride=w<<2,
    .depth=8,
    .colortype=6,
  };
  void *serial=0;
  int serialc=png_encode(&serial,&image);
  free(rgba);
  if (serialc<0) return;
  
  //TODO Generate nice screencap paths per user config.
  char path[1024];
  time_t now=time(0);
  struct tm *tm=localtime(&now);
  if (tm) snprintf(path,sizeof(path),"scratch/fullmoon-%04d%02d%02d-%02d%02d%02d.png",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
  else snprintf(path,sizeof(path),"scratch/fullmoon-%d.png",rand());
  if (fmn_file_write(path,serial,serialc)>=0) {
    fprintf(stderr,"%s: Saved screencap, %d bytes.\n",path,serialc);
  }
  
  free(serial);
}

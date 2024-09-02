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

/* Send framebuffer to gamemon.
 */
#if FMN_USE_gamemon

static void bigpc_fb_downsample(
  uint8_t *dst,int dstw,int dsth, // bgr332
  const uint8_t *src,int srcw,int srch // rgba
) {
  int yscale=srch/dsth; if (yscale<1) yscale=1;
  int xscale=srcw/dstw; if (xscale<1) xscale=1;
  int srcstride=srcw*4*yscale;
  int srcstep=xscale*4;
  int yi=dsth,srcy=0;
  for (;yi-->0;dst+=dstw,src+=srcstride,srcy+=yscale) {
    if (srcy>=srch) {
      memset(dst,0,dstw);
    } else {
      uint8_t *dstp=dst;
      const uint8_t *srcp=src;
      int xi=dstw,srcx=0;
      for (;xi-->0;dstp++,srcp+=srcstep,srcx+=xscale) {
        if (srcx>=srcw) {
          *dstp=0;
        } else {
          uint8_t r=srcp[0],g=srcp[1],b=srcp[2];
          *dstp=(b&0xe0)|((g>>3)&0x1c)|(r>>6);
        }
      }
    }
  }
}

void bigpc_gamemon_send_framebuffer() {
  int pixfmt,w=0,h=0;
  pixfmt=gamemon_get_fb_format(&w,&h,bigpc.gamemon);
  if ((w<1)||(h<1)) return;
  if ((w!=bigpc.gamemon_fbw)||(h!=bigpc.gamemon_fbh)) {
    void *nv=malloc(w*h);
    if (!nv) return;
    if (bigpc.gamemon_fb) free(bigpc.gamemon_fb);
    bigpc.gamemon_fb=nv;
    bigpc.gamemon_fbw=w;
    bigpc.gamemon_fbh=h;
  }
  if (!bigpc.render->type->read_framebuffer) return;
  void *rgba=0;
  int srcw=0,srch=0;
  if (bigpc.render->type->read_framebuffer(&rgba,&srcw,&srch,bigpc.render)<0) return;
  if (!rgba) return;
  bigpc_fb_downsample(bigpc.gamemon_fb,w,h,rgba,srcw,srch);
  free(rgba);
  gamemon_send_framebuffer(bigpc.gamemon,bigpc.gamemon_fbw,bigpc.gamemon_fbh,GAMEMON_PIXFMT_BGR332,bigpc.gamemon_fb,w*h);
}

#endif

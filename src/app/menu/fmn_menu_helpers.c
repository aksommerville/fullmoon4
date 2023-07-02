#include "fmn_menu_internal.h"

/* Generate text image.
 */
 
void fmn_generate_text_image(uint16_t imageid,const char *src,int srcc,int16_t forcew,int16_t forceh) {

  const int16_t glyphw=8,glyphh=8;
  const uint16_t font_imageid=20;

  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  #define vtxa 128
  if (srcc>vtxa) srcc=vtxa;
  int16_t labelw=forcew,labelh=forceh;
  if (!labelw) labelw=glyphw*srcc;
  if (!labelh) labelh=glyphh;
  
  fmn_video_init_image(imageid,labelw,labelh);
  if (fmn_draw_set_output(imageid)<0) return;
  
  fmn_draw_clear();
  
  int16_t fullw=glyphw*srcc;
  int16_t dsty=labelh>>1;
  int16_t dstx=(labelw>>1)-(fullw>>1)+(glyphw>>1);
  struct fmn_draw_mintile vtxv[vtxa];
  int vtxc=0;
  for (;srcc-->0;src++,dstx+=glyphw) {
    if ((unsigned char)(*src)<=0x20) continue;
    vtxv[vtxc++]=(struct fmn_draw_mintile){dstx,dsty,*src,0};
  }
  fmn_draw_mintile(vtxv,vtxc,font_imageid);
  
  fmn_draw_set_output(0);
  #undef vtxa
}

void fmn_generate_string_image(uint16_t imageid,uint16_t stringid,int16_t forcew,int16_t forceh) {
  char tmp[128];
  int tmpc=fmn_get_string(tmp,sizeof(tmp),stringid);
  if ((tmpc<1)||(tmpc>sizeof(tmp))) tmpc=0;
  fmn_generate_text_image(imageid,tmp,tmpc,forcew,forceh);
}

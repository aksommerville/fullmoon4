#include "mkd_internal.h"

/* Compile one tileprops.
 */
 
int mkd_compile_tileprops(struct mkd_respath *respath) {
  struct sr_decoder decoder={.v=mkd.src,.c=mkd.srcc};
  const char *line;
  int linec,lineno=1;
  int reading=0,gotrowc=0;
  int dstp=mkd.dst.c;
  if (sr_encoder_require(&mkd.dst,256)<0) return -1;
  memset(mkd.dst.v+dstp,0,256);
  mkd.dst.c+=256;
  for (;linec=sr_decode_line(&line,&decoder);lineno++) {
    int i=0; for (;i<linec;i++) if (line[i]=='#') linec=i;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    
    if ((linec==7)&&!memcmp(line,"physics",7)) {
      reading=1;
      continue;
    }
    if (!reading) continue;
    if (gotrowc>=16) break;
    
    if (linec!=32) {
      fprintf(stderr,"%s:%d: Expected 32 hexadecimal digits.\n",respath->path,lineno);
      return -2;
    }
    for (i=16;i-->0;line+=2,dstp++) {
      int hi=sr_digit_eval(line[0]);
      int lo=sr_digit_eval(line[1]);
      if ((hi<0)||(hi>0xf)||(lo<0)||(lo>0xf)) {
        fprintf(stderr,"%s:%d: Expected hexadecimal byte, found '%.2s'\n",respath->path,lineno,line);
        return -2;
      }
      mkd.dst.v[dstp]=(hi<<4)|lo;
    }
    gotrowc++;
  }
  // it's ok if it comes up short
  return 0;
}

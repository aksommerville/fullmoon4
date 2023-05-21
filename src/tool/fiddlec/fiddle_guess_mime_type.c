#include <string.h>

/* Guess MIME type.
 */
 
const char *fiddle_guess_mime_type(const char *path,const void *v,int c) {
  
  // Check unambiguous content signatures.
  if (v) {
    if ((c>=8)&&!memcmp(v,"\x89PNG\r\n\x1a\n",8)) return "image/png";
    if ((c>=6)&&!memcmp(v,"GIF87a",6)) return "image/gif";
    if ((c>=6)&&!memcmp(v,"GIF89a",6)) return "image/gif";
  }
  
  // Trust path extensions if present.
  if (path) {
    const char *sfxsrc=0;
    int sfxsrcc=0;
    int pathp=0;
    for (;path[pathp];pathp++) {
      if (path[pathp]=='.') { sfxsrc=path+pathp+1; sfxsrcc=0; }
      else if (path[pathp]=='/') sfxsrcc=0;
      else if (sfxsrc) sfxsrcc++;
    }
    char norm[16];
    if ((sfxsrcc>0)&&(sfxsrcc<sizeof(norm))) {
      int i=sfxsrcc;
      while (i-->0) {
        if ((sfxsrc[i]>='A')&&(sfxsrc[i]<='Z')) norm[i]=sfxsrc[i]+0x20;
        else norm[i]=sfxsrc[i];
      }
      switch (sfxsrcc) {
        case 1: switch (norm[0]) {
          } break;
        case 2: {
            if (!memcmp(norm,"js",2)) return "application/javascript";
            if (!memcmp(norm,"md",3)) return "text/plain";
          } break;
        case 3: {
            if (!memcmp(norm,"png",3)) return "image/png";
            if (!memcmp(norm,"gif",3)) return "image/gif";
            if (!memcmp(norm,"jpg",3)) return "image/jpeg";
            if (!memcmp(norm,"htm",3)) return "text/html";
            if (!memcmp(norm,"css",3)) return "text/css";
            if (!memcmp(norm,"ico",3)) return "image/x-icon";
            if (!memcmp(norm,"txt",3)) return "text/plain";
          } break;
        case 4: {
            if (!memcmp(norm,"html",4)) return "text/html";
            if (!memcmp(norm,"jpeg",4)) return "image/jpeg";
            if (!memcmp(norm,"json",4)) return "application/json";
          } break;
      }
    }
  }
  
  // Check fuzzier content signatures.
  if (v) {
    if ((c>=14)&&!memcmp(v,"<!DOCTYPE html",14)) return "text/html";
    if ((c>=1)&&!memcmp(v,"{",1)) return "application/json";
  }
  
  // Finally, it's "text/plain" if ASCII, otherwise "application/octet-stream".
  if (v) {
    const char *src=v;
    int i=c;
    for (;i-->0;src++) {
      if (*src==0x09) continue;
      if (*src==0x0a) continue;
      if (*src==0x0d) continue;
      if ((*src>=0x20)&&(*src<=0x7e)) continue;
      return "application/octet-stream";
    }
  }
  return "text/plain";
}

#include "http_internal.h"
#include <stdarg.h>

/* Delete.
 */
 
void http_xfer_del(struct http_xfer *xfer) {
  if (!xfer) return;
  if (xfer->refc-->1) return;
  
  if (xfer->line) free(xfer->line);
  http_dict_cleanup(&xfer->headers);
  if (xfer->body) free(xfer->body);
  
  free(xfer);
}

/* Retain.
 */
 
int http_xfer_ref(struct http_xfer *xfer) {
  if (!xfer) return -1;
  if (xfer->refc<1) return -1;
  if (xfer->refc>=INT_MAX) return -1;
  xfer->refc++;
  return 0;
}

/* New.
 */
 
struct http_xfer *http_xfer_new(struct http_context *context) {
  if (!context) return 0;
  struct http_xfer *xfer=calloc(1,sizeof(struct http_xfer));
  if (!xfer) return 0;
  
  xfer->refc=1;
  xfer->context=context;
  
  return xfer;
}

/* Trivial accessors.
 */

int http_xfer_hold(struct http_xfer *xfer) {
  if (!xfer||(xfer->state!=HTTP_XFER_STATE_UNSET)) return -1;
  xfer->state=HTTP_XFER_STATE_DEFERRED;
  return 0;
}

int http_xfer_ready(struct http_xfer *xfer) {
  if (!xfer||(xfer->state!=HTTP_XFER_STATE_DEFERRED)) return -1;
  xfer->state=HTTP_XFER_STATE_DEFERRAL_COMPLETE;
  return 0;
}

/* Access to Request-Line or Status-Line.
 */
 
int http_xfer_get_status(const struct http_xfer *xfer) {
  if (!xfer) return 0;
  int srcp=0;
  while ((srcp<xfer->linec)&&((unsigned char)xfer->line[srcp]>0x20)) srcp++;
  while ((srcp<xfer->linec)&&((unsigned char)xfer->line[srcp]<=0x20)) srcp++;
  const char *src=xfer->line+srcp;
  int srcc=0;
  while ((srcp<xfer->linec)&&((unsigned char)xfer->line[srcp++]>0x20)) srcc++;
  int status=0;
  if (http_int_eval(&status,src,srcc)<0) return 0;
  return status;
}

int http_xfer_get_status_message(void *dstpp,const struct http_xfer *xfer) {
  if (!xfer) return 0;
  int srcp=0;
  while ((srcp<xfer->linec)&&((unsigned char)xfer->line[srcp]>0x20)) srcp++;
  while ((srcp<xfer->linec)&&((unsigned char)xfer->line[srcp]<=0x20)) srcp++;
  while ((srcp<xfer->linec)&&((unsigned char)xfer->line[srcp]>0x20)) srcp++;
  while ((srcp<xfer->linec)&&((unsigned char)xfer->line[srcp]<=0x20)) srcp++;
  *(void**)dstpp=xfer->line+srcp;
  return xfer->linec-srcp;
}

int http_xfer_get_method(const struct http_xfer *xfer) {
  if (!xfer) return 0;
  const char *src=xfer->line;
  int srcc=0;
  while ((srcc<xfer->linec)&&((unsigned char)src[srcc]>0x20)) srcc++;
  return http_method_eval(src,srcc);
}

int http_xfer_get_path(void *dstpp,const struct http_xfer *xfer) {
  if (!xfer) return 0;
  int srcp=0;
  while ((srcp<xfer->linec)&&((unsigned char)xfer->line[srcp]>0x20)) srcp++;
  while ((srcp<xfer->linec)&&((unsigned char)xfer->line[srcp]<=0x20)) srcp++;
  *(void**)dstpp=xfer->line+srcp;
  int pathc=0;
  while ((srcp<xfer->linec)&&((unsigned char)xfer->line[srcp]>0x20)&&(xfer->line[srcp]!='?')) { srcp++; pathc++; }
  return pathc;
}

int http_xfer_set_line(struct http_xfer *xfer,const char *src,int srcc) {
  if (!xfer) return -1;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  while (srcc&&((unsigned char)src[srcc-1]<=0x20)) srcc--;
  while (srcc&&((unsigned char)src[0]<=0x20)) { src++; srcc--; }
  char *nv=malloc(srcc+1);
  if (!nv) return -1;
  memcpy(nv,src,srcc);
  nv[srcc]=0;
  if (xfer->line) free(xfer->line);
  xfer->line=nv;
  xfer->linec=srcc;
  return 0;
}

int http_xfer_set_status(struct http_xfer *xfer,int code,const char *fmt,...) {
  if ((code<100)||(code>999)) code=500;
  char tmp[256];
  int tmpc=snprintf(tmp,sizeof(tmp),"HTTP/1.1 %3d ",code);
  if ((tmpc<1)||(tmpc>=sizeof(tmp))) return -1;
  va_list vargs;
  va_start(vargs,fmt);
  int err=vsnprintf(tmp+tmpc,sizeof(tmp)-tmpc,fmt,vargs);
  if ((err<0)||(tmpc+err>sizeof(tmp))) err=0;
  tmpc+=err;
  return http_xfer_set_line(xfer,tmp,tmpc);
}

/* Access to headers.
 */
      
int http_xfer_get_header(void *dstpp,const struct http_xfer *xfer,const char *k,int kc) {
  return http_dict_get(dstpp,&xfer->headers,k,kc);
}

int http_xfer_get_header_int(int *dst,const struct http_xfer *xfer,const char *k,int kc) {
  const char *src=0;
  int srcc=http_dict_get(&src,&xfer->headers,k,kc);
  if (srcc<=0) return -1;
  return http_int_eval(dst,src,srcc);
}

int http_xfer_set_header(struct http_xfer *xfer,const char *k,int kc,const char *v,int vc) {
  return http_dict_set(&xfer->headers,k,kc,v,vc);
}

/* Access to body.
 */
 
int http_xfer_set_body(struct http_xfer *xfer,const void *src,int srcc) {
  if ((srcc<0)||(srcc&&!src)) return -1;
  void *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc))) return -1;
    memcpy(nv,src,srcc);
  }
  if (xfer->body) free(xfer->body);
  xfer->body=nv;
  xfer->bodyc=srcc;
  xfer->bodya=srcc;
  return 0;
}

 
int http_xfer_append_body(struct http_xfer *xfer,const void *src,int srcc) {
  if ((srcc<0)||(srcc&&!src)) return -1;
  if (xfer->bodyc>INT_MAX-srcc) return -1;
  int na=xfer->bodyc+srcc;
  if (na>xfer->bodya) {
    if (na>INT_MAX-1024) return -1;
    na=(na+1024)&~1023;
    void *nv=realloc(xfer->body,na);
    if (!nv) return -1;
    xfer->body=nv;
    xfer->bodya=na;
  }
  memcpy(xfer->body+xfer->bodyc,src,srcc);
  xfer->bodyc+=srcc;
  return 0;
}

int http_xfer_append_bodyf(struct http_xfer *xfer,const char *fmt,...) {
  if (!xfer) return -1;
  if (!fmt||!fmt[0]) return 0;
  while (1) {
    va_list vargs;
    va_start(vargs,fmt);
    int err=vsnprintf(xfer->body+xfer->bodyc,xfer->bodya-xfer->bodyc,fmt,vargs);
    if ((err<0)||(err>=INT_MAX)) return -1;
    if (xfer->bodyc<xfer->bodya-err) {
      xfer->bodyc+=err;
      return 0;
    }
    int na=xfer->bodyc+err+1;
    if (na<INT_MAX-1024) na=(na+1024)&~1023;
    void *nv=realloc(xfer->body,na);
    if (!nv) return -1;
    xfer->body=nv;
    xfer->bodya=na;
  }
}

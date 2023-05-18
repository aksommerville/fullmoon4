#include "http_internal.h"
#include <stdarg.h>

/* Delete.
 */
 
void http_xfer_del(struct http_xfer *xfer) {
  if (!xfer) return;
  if (xfer->refc-->1) return;
  
  if (xfer->path) free(xfer->path);
  if (xfer->status_text) free(xfer->status_text);
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
 
int http_xfer_get_method(const struct http_xfer *xfer) {
  return xfer->method;
}

int http_xfer_get_path_verbatim(void *dstpp,const struct http_xfer *xfer) {
  *(void**)dstpp=xfer->path;
  return xfer->pathc;
}

int http_xfer_get_body(void *dstpp,const struct http_xfer *xfer) {
  *(void**)dstpp=xfer->body;
  return xfer->bodyc;
}

int http_xfer_set_status(struct http_xfer *xfer,int code,const char *fmt,...) {
  xfer->status_code=code;
  if (fmt&&fmt[0]) {
    va_list vargs;
    va_start(vargs,fmt);
    char tmp[256];
    int tmpc=vsnprintf(tmp,sizeof(tmp),fmt,vargs);
    if ((tmpc>0)&&(tmpc<sizeof(tmp))) {
      char *nv=malloc(tmpc+1);
      if (nv) {
        if (xfer->status_text) free(xfer->status_text);
        xfer->status_text=nv;
        xfer->status_textc=tmpc;
        return 0;
      }
    }
  }
  xfer->status_textc=0;
  return 0;
}

int http_xfer_hold(struct http_xfer *xfer) {
  xfer->hold=1;
  return 0;
}

int http_xfer_ready(struct http_xfer *xfer) {
  xfer->hold=0;
  //TODO do we need to notify the context?
  return 0;
}

int http_xfer_set_method(struct http_xfer *xfer,int method) {
  xfer->method=method;
  return 0;
}

int http_xfer_set_path_verbatim(struct http_xfer *xfer,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (xfer->path) free(xfer->path);
  xfer->path=nv;
  xfer->pathc=srcc;
  return 0;
}

/* Path without query, resolving escapes.
 */

int http_xfer_get_path(char *dst,int dsta,const struct http_xfer *xfer) {
  const char *src=xfer->path;
  int srcc=0;
  while ((srcc<xfer->pathc)&&(xfer->path[srcc]!='?')) srcc++;
  return http_url_decode(dst,dsta,src,srcc);
}

int http_xfer_set_path(struct http_xfer *xfer,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int dstc=0,dsta=(srcc+32)&~15;
  char *dst=malloc(dsta);
  if (!dst) return -1;
  int srcp=0; for (;srcp<srcc;srcp++) {
    if (dstc>dsta-4) {
      dsta+=16;
      void *nv=realloc(dst,dsta);
      if (!nv) { free(dst); return -1; }
      dst=nv;
    }
    if (
      ((src[srcp]>='a')&&(src[srcp]<='z'))||
      ((src[srcp]>='A')&&(src[srcp]<='Z'))||
      ((src[srcp]>='0')&&(src[srcp]<='9'))||
      (src[srcp]=='.')||
      (src[srcp]==',')||
      (src[srcp]=='/')||
      (src[srcp]=='~')||
      (src[srcp]=='-')||
      (src[srcp]=='_')
    ) {
      dst[dstc++]=src[srcp];
    } else {
      dst[dstc++]='%';
      dst[dstc++]="0123456789abcdef"[((uint8_t)*src)>>4];
      dst[dstc++]="0123456789abcdef"[((uint8_t)*src)&15];
    }
  }
  dst[dstc]=0;
  if (xfer->path) free(xfer->path);
  xfer->path=dst;
  xfer->pathc=dstc;
  return 0;
}

/* Access to headers.
 */
      
int http_xfer_get_header(void *dstpp,const struct http_xfer *xfer,const char *k,int kc) {
  return -1;//TODO
}

int http_xfer_set_header(struct http_xfer *xfer,const char *k,int kc,const char *v,int vc) {
  return -1;//TODO
}

/* Access to query parameters.
 */
 
static int http_xfer_for_query_1(const char *src,int srcc,int (*cb)(const char *k,int kc,const char *v,int vc,void *userdata),void *userdata) {
  char ek[64],ev[1024];
  while (srcc>0) {
    if (*src=='&') { src++; srcc--; continue; }
    const char *k=src;
    int kc=0;
    while ((srcc>0)&&(*src!='=')&&(*src!='&')) { src++; srcc--; kc++; }
    const char *v=0;
    int vc=0;
    if (srcc&&(*src=='=')) {
      src++; srcc--;
      v=src;
      while ((srcc>0)&&(*src!='&')) { src++; srcc--; vc++; }
    }
    int ekc=http_url_decode(ek,sizeof(ek),k,kc);
    if ((ekc<0)||(ekc>sizeof(ek))) continue;
    int evc=http_url_decode(ev,sizeof(ev),v,vc);
    if ((evc<0)||(evc>sizeof(ev))) continue;
    int err=cb(ek,ekc,ev,evc,userdata);
    if (err) return err;
  }
  return 0;
}
 
int http_xfer_for_query(const struct http_xfer *xfer,int (*cb)(const char *k,int kc,const char *v,int vc,void *userdata),void *userdata) {
  if (!cb) return -1;
  int err;
  
  const char *src=xfer->path;
  int srcc=xfer->pathc;
  while (srcc&&(*src!='?')) { src++; srcc--; }
  if (srcc) {
    if (err=http_xfer_for_query_1(src+1,srcc-1,cb,userdata)) return err;
  }
  
  const char *content_type=0;
  int content_typec=http_xfer_get_header(&content_type,xfer,"Content-Type",12);
  if ((content_typec==33)&&!memcmp(content_type,"application/x-www-form-urlencoded",33)) {
    if (err=http_xfer_for_query_1(src+1,srcc-1,cb,userdata)) return err;
  }
  
  return 0;
}

struct http_xfer_get_query_string_context {
  const char *k;
  int kc;
  char *dst;
  int dsta;
};

static int http_xfer_get_query_string_cb(const char *k,int kc,const char *v,int vc,void *userdata) {
  struct http_xfer_get_query_string_context *ctx=userdata;
  if (ctx->kc!=kc) return 0;
  if (memcmp(ctx->k,k,kc)) return 0;
  if (!vc) return -1; // 0 would mean "continue iteration"
  if (vc<=ctx->dsta) {
    memcpy(ctx->dst,v,vc);
    if (vc<ctx->dsta) ctx->dst[vc]=0;
  }
  return vc;
}

int http_xfer_get_query_string(char *dst,int dsta,const struct http_xfer *xfer,const char *k,int kc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  struct http_xfer_get_query_string_context ctx={
    .k=k,
    .kc=kc,
    .dst=dst,
    .dsta=dsta,
  };
  int dstc=http_xfer_for_query(xfer,http_xfer_get_query_string_cb,&ctx);
  if (dstc<0) {
    if (dsta>0) dst[0]=0;
    return 0;
  }
  if (!dstc) return -1; // not found
  return dstc;
}

int http_xfer_get_query_int(int *dst,const struct http_xfer *xfer,const char *k,int kc) {
  char src[32];
  int srcc=http_xfer_get_query_string(src,sizeof(src),xfer,k,kc);
  if ((srcc<0)||(srcc>sizeof(src))) return -1;
  return http_int_eval(dst,src,srcc);
}

int http_xfer_append_query(struct http_xfer *xfer,const char *k,int kc,const char *v,int vc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  int ekc=http_url_encode(0,0,k,kc);
  int evc=http_url_encode(0,0,v,vc);
  int nc=xfer->pathc+1+ekc+1+evc;
  char *nv=malloc(nc+1);
  if (!nv) return -1;
  memcpy(nv,xfer->path,xfer->pathc);
  char sep='?';
  int i=xfer->pathc;
  while (i-->0) if (xfer->path[i]=='?') { sep='&'; break; }
  nv[xfer->pathc]=sep;
  if (http_url_encode(nv+xfer->pathc+1,nc-1-xfer->pathc,k,kc)!=ekc) { free(nv); return -1; }
  nv[xfer->pathc+1+ekc]='=';
  if (http_url_encode(nv+xfer->pathc+1+ekc+1,nc-1-ekc-1-xfer->pathc,v,vc)!=evc) { free(nv); return -1; }
  nv[nc]=0;
  if (xfer->path) free(xfer->path);
  xfer->path=nv;
  xfer->pathc=nc;
  return 0;
}

int http_xfer_append_query_int(struct http_xfer *xfer,const char *k,int kc,int src) {
  char v[16];
  int vc=snprintf(v,sizeof(v),"%d",src);
  if ((vc<1)||(vc>=sizeof(v))) return -1;
  return http_xfer_append_query(xfer,k,kc,v,vc);
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

#include "mkd_internal.h"
#include "mkd_ar.h"

/* Context.
 */
 
struct mkd_showtoc {
  int typefilter;
  int qfilter;
  int summary;
  int quiet; // don't show per-resource id and size
  int numeric;
  struct mkd_showtoc_bucket {
    int type,qualifier;
    int resc;
    int size;
  } *bucketv;
  int bucketc,bucketa;
};

static void mkd_showtoc_cleanup(struct mkd_showtoc *ctx) {
  if (ctx->bucketv) free(ctx->bucketv);
}

/* Search buckets.
 */
 
static int mkd_showtoc_find_bucket(const struct mkd_showtoc *ctx,int type,int qualifier) {
  int lo=0,hi=ctx->bucketc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct mkd_showtoc_bucket *q=ctx->bucketv+ck;
         if (type<q->type) hi=ck;
    else if (type>q->type) lo=ck+1;
    else if (qualifier<q->qualifier) hi=ck;
    else if (qualifier>q->qualifier) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Get or create a bucket.
 */
 
static struct mkd_showtoc_bucket *mkd_showtoc_get_bucket(struct mkd_showtoc *ctx,int type,int qualifier) {
  int p=mkd_showtoc_find_bucket(ctx,type,qualifier);
  if (p>=0) return ctx->bucketv+p;
  p=-p-1;
  if (ctx->bucketc>=ctx->bucketa) {
    int na=ctx->bucketa+32;
    if (na>INT_MAX/sizeof(struct mkd_showtoc_bucket)) return 0;
    void *nv=realloc(ctx->bucketv,sizeof(struct mkd_showtoc_bucket)*na);
    if (!nv) return 0;
    ctx->bucketv=nv;
    ctx->bucketa=na;
  }
  struct mkd_showtoc_bucket *bucket=ctx->bucketv+p;
  memmove(bucket+1,bucket,sizeof(struct mkd_showtoc_bucket)*(ctx->bucketc-p));
  memset(bucket,0,sizeof(struct mkd_showtoc_bucket));
  ctx->bucketc++;
  bucket->type=type;
  bucket->qualifier=qualifier;
  return bucket;
}

/* Single resource callback.
 */
 
static int mkd_showtoc_cb(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata) {
  struct mkd_showtoc *ctx=userdata;
  
  if (ctx->typefilter&&(type!=ctx->typefilter)) return 0;
  if (ctx->qfilter&&(qualifier!=ctx->qfilter)) return 0;
  
  if (!ctx->quiet) {
    if (ctx->numeric) {
      fprintf(stdout,"%d:%d:%d %d\n",type,qualifier,id,c);
    } else {
      char desc[256];
      int descc=mkd_resid_repr(desc,sizeof(desc),type,qualifier,id);
      if ((descc<0)||(descc>sizeof(desc))) descc=0;
      fprintf(stdout,"%.*s %d\n",descc,desc,c);
    }
  }
  
  if (ctx->summary) {
    struct mkd_showtoc_bucket *bucket=mkd_showtoc_get_bucket(ctx,type,qualifier);
    if (!bucket) return -1;
    bucket->resc++;
    bucket->size+=c;
  }
  
  return 0;
}

/* Dump TOC, main entry point.
 */
 
int mkd_main_showtoc() {
  int err=mkd_read_single_input();
  if (err<0) return err;
  struct mkd_ar *ar=mkd_ar_new();
  if (!ar) return -1;
  if ((err=mkd_ar_decode(ar,mkd.src,mkd.srcc,mkd.config.srcpathv[0]))<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to decode archive.\n",mkd.config.srcpathv[0]);
    return -2;
  }
  
  /* Our help text says "type" and "qualifier" should take an int, but that's unwieldly for the user.
   * Try int first, then general eval.
   */
  struct mkd_showtoc ctx={0};
  if (!(ctx.typefilter=mkd_config_get_arg_int(&mkd.config,"type",4))) {
    const char *src=0;
    int srcc=mkd_config_get_arg_string(&src,&mkd.config,"type",4);
    if (srcc>0) {
      if ((ctx.typefilter=mkd_restype_eval(src,srcc))<1) {
        fprintf(stderr,"%s: Unknown type name '%.*s'\n",mkd.config.exename,srcc,src);
        return -2;
      }
    }
  }
  if (!(ctx.qfilter=mkd_config_get_arg_int(&mkd.config,"qualifier",9))) {
    const char *src=0;
    int srcc=mkd_config_get_arg_string(&src,&mkd.config,"qualifier",9);
    if (srcc>0) {
      if ((ctx.qfilter=mkd_qualifier_eval(ctx.typefilter,src,srcc))<0) {
        fprintf(stderr,"%s: Failed to parse qualifier '%.*s' for type %d.\n",mkd.config.exename,srcc,src,ctx.typefilter);
        return -2;
      }
    }
  }
  ctx.summary=mkd_config_get_arg_int(&mkd.config,"summary",7);
  ctx.quiet=mkd_config_get_arg_int(&mkd.config,"quiet",5);
  ctx.numeric=mkd_config_get_arg_int(&mkd.config,"numeric",7);
  
  mkd_ar_for_each(ar,mkd_showtoc_cb,&ctx);
  
  if (ctx.summary) {
    int resc_total=0,size_total=0;
    const struct mkd_showtoc_bucket *bucket=ctx.bucketv;
    int i=ctx.bucketc;
    fprintf(stdout,"%s: Summary by qualified type:\n",mkd.config.srcpathv[0]);
    for (;i-->0;bucket++) {
      if (ctx.numeric) {
        fprintf(stdout,"  %d:%d:%d: %d b\n",bucket->type,bucket->qualifier,bucket->resc,bucket->size);
      } else {
        // We abuse resource repr to show the count as ID (it's always numeric).
        char desc[256];
        int descc=mkd_resid_repr(desc,sizeof(desc),bucket->type,bucket->qualifier,bucket->resc);
        if ((descc<0)||(descc>sizeof(desc))) descc=0;
        fprintf(stdout,"  %.*s: %d b\n",descc,desc,bucket->size);
      }
      resc_total+=bucket->resc;
      size_total+=bucket->size;
    }
    fprintf(stdout,
      "%s: %d bytes total, %d resources, %d bytes payload total.\n",
      mkd.config.srcpathv[0],mkd.srcc,resc_total,size_total
    );
  }
  
  mkd_ar_del(ar);
  mkd_showtoc_cleanup(&ctx);
  return 0;
}

#include "mkd_internal.h"
#include "mkd_ar.h"
#include <unistd.h>
#include <fcntl.h>

struct mkd_extract_context {
  struct mkd_ar *ar;
  // Search for:
  int stype,squalifier,sid;
  // Found:
  int ftype,fqualifier,fid,fc;
  const void *fv;
  int matchc;
};

/* Resource callback.
 */
 
static int mkd_extract_cb(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata) {
  struct mkd_extract_context *ctx=userdata;
  
  if (ctx->stype&&(ctx->stype!=type)) return 0;
  if (ctx->squalifier&&(ctx->squalifier!=qualifier)) return 0;
  if (ctx->sid&&(ctx->sid!=id)) return 0;
  
  ctx->matchc++;
  
  if (ctx->fid) return 0; // keep only the first match
  ctx->ftype=type;
  ctx->fqualifier=qualifier;
  ctx->fid=id;
  ctx->fv=v;
  ctx->fc=c;
  return 0;
}

/* Extract one resource, main entry point.
 */
 
int mkd_main_extract() {
  int err=mkd_read_single_input();
  if (err<0) return err;
  
  struct mkd_ar *ar=mkd_ar_new();
  if (!ar) return -1;
  if ((err=mkd_ar_decode(ar,mkd.src,mkd.srcc,mkd.config.srcpathv[0]))<0) {
    mkd_ar_del(ar);
    return err;
  }
  
  /* We expect parameters "type", "qualifier", and "id".
   * If any is zero or absent, it's a wildcard.
   * There must be exactly one match though, we'll reject otherwise.
   * (lot of times, qualifier is not in play and won't be provided).
   */
  int type=mkd_config_get_arg_int(&mkd.config,"type",4);
  if (!type) {
    const char *src=0;
    int srcc=mkd_config_get_arg_string(&src,&mkd.config,"type",4);
    if (srcc>0) {
      if ((type=mkd_restype_eval(src,srcc))<1) {
        fprintf(stderr,"%s: Unknown type name '%.*s'\n",mkd.config.exename,srcc,src);
        return -2;
      }
    }
  }
  int qualifier=mkd_config_get_arg_int(&mkd.config,"qualifier",9);
  if (!qualifier) {
    const char *src=0;
    int srcc=mkd_config_get_arg_string(&src,&mkd.config,"qualifier",9);
    if (srcc>0) {
      if ((qualifier=mkd_qualifier_eval(type,src,srcc))<0) {
        fprintf(stderr,"%s: Failed to parse qualifier '%.*s' for type %d.\n",mkd.config.exename,srcc,src,type);
        return -2;
      }
    }
  }
  int id=mkd_config_get_arg_int(&mkd.config,"id",2);
  
  struct mkd_extract_context ctx={
    .ar=ar,
    .stype=type,
    .squalifier=qualifier,
    .sid=id,
  };
  mkd_ar_for_each(ar,mkd_extract_cb,&ctx);
  
  if (!ctx.matchc) {
    fprintf(stderr,"%s: Resource %d:%d:%d not found.\n",mkd.config.srcpathv[0],type,qualifier,id);
    mkd_ar_del(ar);
    return -2;
  }
  if (ctx.matchc>1) {
    fprintf(stderr,"%s: %d matches for resource %d:%d:%d.\n",mkd.config.srcpathv[0],ctx.matchc,type,qualifier,id);
    mkd_ar_del(ar);
    return -2;
  }
  
  const uint8_t *SRC=ctx.fv;
  int srcp=0;
  while (srcp<ctx.fc) {
    if ((err=write(STDOUT_FILENO,SRC+srcp,ctx.fc-srcp))<=0) {
      fprintf(stderr,"%s: Write to stdout failed (%m)\n",mkd.config.srcpathv[0]);
      mkd_ar_del(ar);
      return -2;
    }
    srcp+=err;
  }
  
  mkd_ar_del(ar);
  return 0;
}

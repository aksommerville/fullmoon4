#include "datan_internal.h"

/* Look for gaps in resource ID and issue warnings.
 * No errors.
 */
 
struct datan_continuity_context {
  uint16_t type;
  uint16_t qualifier;
  uint32_t id;
};

static int datan_continuity_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct datan_continuity_context *ctx=userdata;
  if (type>ctx->type) {
    ctx->type=type;
    ctx->qualifier=0;
    ctx->id=0;
  }
  if (qualifier>ctx->qualifier) {
    ctx->qualifier=qualifier;
    ctx->id=0;
  }
  if (id>ctx->id+1) {
    if (type==FMN_RESTYPE_TILEPROPS) {
      // TILEPROPS are expected to be discontinuous, it's fine.
    } else {
      int missingc=id-ctx->id-1;
      fprintf(stderr,
        "%s:WARNING: Missing %d resources beginning ID %d of type %s, qualifier %d.\n",
        datan.arpath,missingc,ctx->id+1,fmn_restype_repr(type),qualifier
      );
    }
  }
  ctx->id=id;
  return 0;
}
 
int datan_validate_res_id_continuity() {
  struct datan_continuity_context ctx={0};
  fmn_datafile_for_each(datan.datafile,datan_continuity_cb,&ctx);
  return 0;
}

#include "mkd_internal.h"
#include "opt/png/png.h"

/* Encode QOI from decoded (PNG) image.
 */
 
struct qoi_pixel {
  uint8_t r,g,b,a;
};

struct qoi_context {
  struct sr_encoder *dst;
  uint32_t pv;
  uint32_t colorv[64];
  int run;
  // performance metrics. can remove later:
  int oprgbac,oprgbc,opindexc,opdiffc,oplumac,oprunc; // oprunc is the count of pixels, not count of runs. sum here should be (w*h)
};

static inline int chdiff(int a,int b,int shift) {
  return ((a>>shift)&0xff)-((b>>shift)&0xff);
}

static int mkd_encode_qoi_cb_pixel(struct png_image *src,int x,int y,uint32_t rgba,void *userdata) {
  struct qoi_context *ctx=userdata;
  //fprintf(stderr,"INCOMING 0x%08x (%d,%d)\n",rgba,x,y);
  
  if (sr_encode_intbe(ctx->dst,rgba,4)<0) return -1;
  return 0;
  
  // Continue run?
  if (rgba==ctx->pv) {
    if (ctx->run>=62) {
      if (sr_encode_u8(ctx->dst,0xc0|(ctx->run-1))<0) return -1;
      ctx->run=0;
    }
    ctx->run++;
    ctx->oprunc++;
    //fprintf(stderr,"...RUN\n");
    return 0;
  }
  
  // Update colorv. This is done for every pixel, but redundant within runs.
  int colorp=(((rgba>>24)*3)+(((rgba>>16)&0xff)*5)+(((rgba>>8)&0xff)*7)+(rgba&0xff)*11)&0x3f;
  
  // Terminate run?
  if (ctx->run) {
    if (sr_encode_u8(ctx->dst,0xc0|(ctx->run-1))<0) return -1;
    ctx->run=0;
  }
  
  // QOI_OP_DIFF, and commit pv
  int dr=chdiff(rgba,ctx->pv,24);
  int dg=chdiff(rgba,ctx->pv,16);
  int db=chdiff(rgba,ctx->pv,8);
  int da=chdiff(rgba,ctx->pv,0);
  ctx->pv=rgba;
  if ((dr>=-2)&&(dr<=1)&&(dg>=-2)&&(dg<=1)&&(db>=-2)&&(db<=1)&&!da) {
    dr+=2; dg+=2; db+=2;
    if (sr_encode_u8(ctx->dst,0x40|(dr<<4)|(dg<<2)|db)<0) return -1;
    ctx->opdiffc++;
    ctx->colorv[colorp]=rgba;
    //fprintf(stderr,"...DIFF\n");
    return 0;
  }
  
  // QOI_OP_INDEX
  int p=64; while (p-->0) {
    if (ctx->colorv[p]==rgba) {
      if (sr_encode_u8(ctx->dst,p)<0) return -1;
      ctx->opindexc++;
      ctx->colorv[colorp]=rgba;
      //fprintf(stderr,"...INDEX %d\n",p);
      return 0;
    }
  }
  
  // QOI_OP_LUMA
  if ((dg>=-32)&&(dg<=31)&&!da) {
    dr-=dg;
    db-=dg;
    if ((dr>=-8)&&(dr<=7)&&(db>=-8)&&(db<=7)) {
      if (sr_encode_u8(ctx->dst,0x80|(dg+32))<0) return -1;
      if (sr_encode_u8(ctx->dst,((dr+8)<<4)|(db+8))<0) return -1;
      ctx->oplumac++;
      ctx->colorv[colorp]=rgba;
      //fprintf(stderr,"...LUMA\n");
      return 0;
    }
  }
  
  // QOI_OP_RGB
  if (!da) {
    if (sr_encode_u8(ctx->dst,0xfe)<0) return -1;
    if (sr_encode_u8(ctx->dst,rgba>>24)<0) return -1;
    if (sr_encode_u8(ctx->dst,rgba>>16)<0) return -1;
    if (sr_encode_u8(ctx->dst,rgba>>8)<0) return -1;
    ctx->oprgbc++;
    ctx->colorv[colorp]=rgba;
    //fprintf(stderr,"...RGB\n");
    return 0;
  }
  
  // QOI_OP_RGBA
  if (sr_encode_u8(ctx->dst,0xff)<0) return -1;
  if (sr_encode_u8(ctx->dst,rgba>>24)<0) return -1;
  if (sr_encode_u8(ctx->dst,rgba>>16)<0) return -1;
  if (sr_encode_u8(ctx->dst,rgba>>8)<0) return -1;
  if (sr_encode_u8(ctx->dst,rgba)<0) return -1;
  ctx->oprgbac++;
  ctx->colorv[colorp]=rgba;
  //fprintf(stderr,"...RGBA\n");
  
  return 0;
}
 
static int mkd_encode_qoi(struct sr_encoder *dst,struct png_image *src,struct mkd_respath *respath) {

  // 14-byte header.
  /*
  if (sr_encode_raw(dst,"qoif",4)<0) return -1;
  if (sr_encode_intbe(dst,src->w,4)<0) return -1;
  if (sr_encode_intbe(dst,src->h,4)<0) return -1;
  switch (src->colortype) {
    case 0: case 2: if (sr_encode_u8(dst,3)<0) return -1; break;
    case 4: case 6: if (sr_encode_u8(dst,4)<0) return -1; break;
    case 3: if (sr_encode_u8(dst,4)<0) return -1; break; // Assume tRNS present and relevant for indexed. Could be smarter about this.
    default: return -1; // invalid
  }
  if (sr_encode_u8(dst,1)<0) return -1; // linear samples
  */
  
  // Per pixel.
  struct qoi_context ctx={
    .dst=dst,
    .pv=0x000000ff,
  };
  int err=png_image_iterate(src,mkd_encode_qoi_cb_pixel,&ctx);
  if (err<0) return err;
  
  // Finish any run in progress.
  if (ctx.run) {
    if (sr_encode_u8(dst,0xc0|(ctx.run-1))<0) return -1;
  }
  
  // End of stream indicator.
  //if (sr_encode_raw(dst,"\0\0\0\0\0\0\0\1",8)<0) return -1;
  
  fprintf(stderr,
    "%.*s: pixelc=%d rgba=%d rgb=%d index=%d diff=%d luma=%d run=%d\n",
    respath->pathc,respath->path,src->w*src->h,ctx.oprgbac,ctx.oprgbc,ctx.opindexc,ctx.opdiffc,ctx.oplumac,ctx.oprunc
  );

  return 0;
}

/* Compile image.
 */
 
int mkd_compile_image(struct mkd_respath *respath) {

  //TODO Web will use PNG in and PNG out. Might as well pass it verbatim in that case.
  // Or do we want to optimize PNGs? Sounds reasonable.
  // But for now I'm experimenting with QOI, and will convert everything to QOI.
  //if (sr_encode_raw(&mkd.dst,mkd.src,mkd.srcc)<0) return -1;
  //return 0;
  
  struct png_image *input=png_decode(mkd.src,mkd.srcc);
  if (!input) {
    fprintf(stderr,"%.*s: Failed to decode PNG\n",respath->pathc,respath->path);
    return -2;
  }
  
  fprintf(stderr,"%.*s: Decoded PNG. %dx%d depth=%d colortype=%d\n",respath->pathc,respath->path,input->w,input->h,input->depth,input->colortype);
  
  int err=mkd_encode_qoi(&mkd.dst,input,respath);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%.*s: Unspecified error encoding QOI.\n",respath->pathc,respath->path);
    return -2;
  }

  png_image_del(input);
  return 0;
}

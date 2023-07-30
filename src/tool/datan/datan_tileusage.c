#include "datan_internal.h"
#include "opt/png/png.h"

/* Get tileusage entry or add it.
 */
 
static struct datan_tuentry *datan_tileusage_intern(uint8_t imageid) {
  int lo=0,hi=datan.tuentryc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    struct datan_tuentry *tu=datan.tuentryv+ck;
         if (imageid<tu->imageid) hi=ck;
    else if (imageid>tu->imageid) lo=ck+1;
    else return tu;
  }
  if (datan.tuentryc>=datan.tuentrya) {
    int na=datan.tuentrya+16;
    void *nv=realloc(datan.tuentryv,sizeof(struct datan_tuentry)*na);
    if (!nv) return 0;
    datan.tuentryv=nv;
    datan.tuentrya=na;
  }
  struct datan_tuentry *tu=datan.tuentryv+lo;
  memmove(tu+1,tu,sizeof(struct datan_tuentry)*(datan.tuentryc-lo));
  datan.tuentryc++;
  memset(tu,0,sizeof(struct datan_tuentry));
  tu->imageid=imageid;
  return tu;
}

/* Gather tile usage for one map.
 */
 
static int datan_tileusage_1(const struct datan_res *res) {
  struct datan_map *map=res->obj;
  struct datan_tuentry *tu=datan_tileusage_intern(map->imageid);
  if (!tu) return -1;
  const uint8_t *v=map->v;
  int i=FMN_COLC*FMN_ROWC;
  for (;i-->0;v++) {
    tu->usage[(*v)>>3]|=1<<((*v)&7);
  }
  return 0;
}

/* Gather tile usage for one archive.
 */
 
int datan_tileusage() {
  const struct datan_res *res=datan.resv;
  int i=datan.resc;
  for (;i-->0;res++) {
    if (res->type<FMN_RESTYPE_MAP) continue;
    if (res->type>FMN_RESTYPE_MAP) break;
    int err=datan_tileusage_1(res);
    if (err<0) {
      if (err!=-2) fprintf(stderr,
        "%s:map:%d(%d): Unspecified error processing tile usage.\n",
        datan.arpath,res->id,res->qualifier
      );
      return -2;
    }
  }
  return 0;
}

/* Draw 2-pixel border around an RGB image.
 */
 
static void datan_tileusage_draw_border(uint8_t *v,int stride,int w,int h,uint8_t r,uint8_t g,uint8_t b) {
  // Four horizontal lines.
  uint8_t *pv[4]={
    v,
    v+stride,
    v+stride*(h-2),
    v+stride*(h-1),
  };
  int i=w;
  while (i-->0) {
    *(pv[0]++)=r;
    *(pv[0]++)=g;
    *(pv[0]++)=b;
    *(pv[1]++)=r;
    *(pv[1]++)=g;
    *(pv[1]++)=b;
    *(pv[2]++)=r;
    *(pv[2]++)=g;
    *(pv[2]++)=b;
    *(pv[3]++)=r;
    *(pv[3]++)=g;
    *(pv[3]++)=b;
  }
  // Four vertical lines. And we can strip two rows off top and bottom.
  v+=stride*2;
  h-=4;
  pv[0]=v;
  pv[1]=v+3;
  pv[2]=v+(w-2)*3;
  pv[3]=v+(w-1)*3;
  i=h;
  while (i-->0) {
    pv[0][0]=r; pv[0][1]=g; pv[0][2]=b; pv[0]+=stride;
    pv[1][0]=r; pv[1][1]=g; pv[1][2]=b; pv[1]+=stride;
    pv[2][0]=r; pv[2][1]=g; pv[2][2]=b; pv[2]+=stride;
    pv[3][0]=r; pv[3][1]=g; pv[3][2]=b; pv[3]+=stride;
  }
}

/* True if an image is entirely zeroes.
 */
 
static int datan_tileusage_is_vacant(const uint8_t *v,int stride,int w,int h) {
  for (;h-->0;v+=stride) {
    const uint8_t *p=v;
    int xi=w;
    for (;xi-->0;) {
      if (*(p++)) return 0;
      if (*(p++)) return 0;
      if (*(p++)) return 0;
    }
  }
  return 1;
}

/* Output, the main event: Draw a colored border over each tile of this image, according to its usage.
 */
 
static int datan_tileusage_rewrite_image(struct png_image *image,const uint8_t *usage) {

  // Force to 24-bit RGB.
  if (png_image_reformat_in_place(image,8,2)<0) return -1;

  // Our border will be 2 pixels on each edge. Tiles must be at least 4x4.
  int colw=image->w>>4,rowh=image->h>>4;
  if ((colw<4)||(rowh<4)) return -1;

  int colstride=colw*3;
  int ribbonstride=image->stride*rowh;  
  uint8_t *ribbon=image->pixels;
  uint8_t mask=1;
  int yi=16;
  for (;yi-->0;ribbon+=ribbonstride) {
    uint8_t *tile=ribbon;
    int xi=16;
    for (;xi-->0;tile+=colstride) {
      if ((*usage)&mask) datan_tileusage_draw_border(tile,image->stride,colw,rowh,0x00,0x80,0x00);
      else if (!datan_tileusage_is_vacant(tile,image->stride,colw,rowh)) datan_tileusage_draw_border(tile,image->stride,colw,rowh,0xff,0x00,0x00);
      if (mask==0x80) { usage++; mask=1; }
      else mask<<=1;
    }
  }
  
  return 0;
}

/* Compose HTML for one entry.
 */
 
static int datan_tileusage_generate_report_1(struct sr_encoder *dst,uint8_t imageid,const uint8_t *usage) {
  const void *serial=0;
  int serialc=fmn_datafile_get_any(&serial,datan.datafile,FMN_RESTYPE_IMAGE,imageid);
  if (serialc<1) {
    if (sr_encode_fmt(dst,"<div class=\"error\">image:%d not found!</div>\n",imageid)<0) return -1;
    return 0;
  }
  struct png_image *image=png_decode(serial,serialc);
  if (!image) {
    if (sr_encode_fmt(dst,"<div class=\"error\">image:%d failed to decode %d bytes as PNG</div>\n",imageid,serialc)<0) return -1;
    return 0;
  }
  int err=datan_tileusage_rewrite_image(image,usage);
  if (err<0) {
    if (sr_encode_fmt(dst,
      "<div class=\"error\">image:%d error rewriting image. size=%d depth=%d colortype=%d</div>\n",
      imageid,image->w,image->h,image->depth,image->colortype
    )<0) return -1;
    png_image_del(image);
    return 0;
  }
  void *reserial=0;
  int reserialc=png_encode(&reserial,image);
  png_image_del(image);
  if (reserialc<0) {
    if (sr_encode_fmt(dst,"<div class=\"error\">image:%d failed to reencode as PNG</div>\n",imageid)<0) return -1;
    return 0;
  }
  
  if (
    (sr_encode_fmt(dst,"<h2>image:%d</h2><img src=\"data:image/png;base64,",imageid)<0)||
    (sr_encode_base64(dst,reserial,reserialc)<0)||
    (sr_encode_fmt(dst,"\"/>\n")<0)
  ) {
    free(reserial);
    return -1;
  }
  
  free(reserial);
  return 0;
}

/* Compose report.
 */
 
static int datan_tileusage_finish_internal(struct sr_encoder *dst) {
  if (sr_encode_raw(dst,"<!DOCTYPE html>\n<html><head>\n",-1)<0) return -1;
  if (sr_encode_raw(dst,"<title>Tile Usage</title>\n",-1)<0) return -1;
  if (sr_encode_raw(dst,"<style>\n",-1)<0) return -1;
  if (sr_encode_raw(dst,"html { background-color: #000; color: #fff; }\n",-1)<0) return -1;
  if (sr_encode_raw(dst,".error { border: 1px solid #f00; padding: 1em; }\n",-1)<0) return -1;
  if (sr_encode_raw(dst,"img { width: 512px; image-rendering: pixelated; }\n",-1)<0) return -1; // highly questionable. assumes 256x256 images, and that you want them scaled up
  if (sr_encode_raw(dst,"</style>\n",-1)<0) return -1;
  if (sr_encode_raw(dst,"</head><body>\n",-1)<0) return -1;
  if (sr_encode_raw(dst,"<div>Images not referenced by any map are <b>not shown</b>.</div>\n",-1)<0) return -1;
  if (sr_encode_raw(dst,"<div>Mind that tiles may be referenced by sprites or hard-coded logic; this report is not aware of those.</div>\n",-1)<0) return -1;
  
  const struct datan_tuentry *tu=datan.tuentryv;
  int i=datan.tuentryc;
  for (;i-->0;tu++) {
    int err=datan_tileusage_generate_report_1(dst,tu->imageid,tu->usage);
    if (err<0) return err;
  }
  
  if (sr_encode_raw(dst,"</body></html>\n",-1)<0) return -1;
  return 0;
}

/* Compose and output the report.
 */
 
int datan_tileusage_finish() {
  struct sr_encoder dst={0};
  int err=datan_tileusage_finish_internal(&dst);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error generating report.\n",datan.tileusage);
    sr_encoder_cleanup(&dst);
    return -2;
  }
  if (fmn_file_write(datan.tileusage,dst.v,dst.c)<0) {
    fprintf(stderr,"%s: Failed to write file, %d bytes.\n",datan.tileusage,dst.c);
    sr_encoder_cleanup(&dst);
    return -2;
  }
  fprintf(stderr,"%s: Generated tile usage report.\n",datan.tileusage);
  sr_encoder_cleanup(&dst);
  return 0;
}

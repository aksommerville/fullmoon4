#include "fmn_soft_internal.h"
#include "opt/datafile/fmn_datafile.h"
#include "opt/png/png.h"

/* Acquire image from the attached datafile.
 */
 
struct fmn_soft_image_from_datafile_context {
  struct bigpc_render_driver *driver;
  uint16_t id;
};

static struct bigpc_image *fmn_soft_image_from_datafile(void *userdata) {
  struct fmn_soft_image_from_datafile_context *ctx=userdata;
  struct bigpc_render_driver *driver=ctx->driver;
  const void *serial=0;
  int serialc=fmn_datafile_get_any(&serial,driver->datafile,FMN_RESTYPE_IMAGE,ctx->id);
  if (serialc<0) return 0;
  struct png_image *png=png_decode(serial,serialc);
  if (!png) return 0;
  
  int has_alpha=((png->colortype==4)||(png->colortype==6));
  {
    uint8_t depth=png->depth,colortype=png->colortype;
    png_depth_colortype_8bit(&depth,&colortype);
    png_depth_colortype_rgb(&depth,&colortype);
    png_depth_colortype_alpha(&depth,&colortype);
    png_depth_colortype_legal(&depth,&colortype);
    struct png_image *re=png_image_reformat(png,0,0,png->w,png->h,depth,colortype,0);
    png_image_del(png);
    if (!(png=re)) return 0;
  }
  
  int pixfmt;
  uint16_t bocheck=0xff00; // easier than figuring out <endian.h>
  if (((uint8_t*)&bocheck)[0]==0xff) { // big-endian
    if (has_alpha) {
      pixfmt=BIGPC_IMAGE_PIXFMT_RGBA;
    } else {
      pixfmt=BIGPC_IMAGE_PIXFMT_RGBX;
    }
  } else { // little-endian
    if (has_alpha) {
      pixfmt=BIGPC_IMAGE_PIXFMT_ABGR;
    } else {
      pixfmt=BIGPC_IMAGE_PIXFMT_XBGR;
    }
  }
  struct bigpc_image *image=bigpc_image_new_handoff(png->pixels,png->w,png->h,png->stride,BIGPC_IMAGE_STORAGE_32,pixfmt);
  if (!image) {
    png_image_del(png);
    return 0;
  }
  png->pixels=0;
  png_image_del(png);
  
  if (DRIVER->mainpixfmt) bigpc_image_convert_in_place(image,DRIVER->mainpixfmt);
  
  return image;
}

/* Grow list.
 */
 
static int fmn_soft_image_require(struct bigpc_render_driver *driver) {
  if (DRIVER->imagec<DRIVER->imagea) return 0;
  int na=DRIVER->imagea+16;
  if (na>INT_MAX/sizeof(struct soft_image)) return -1;
  void *nv=realloc(DRIVER->imagev,sizeof(struct soft_image)*na);
  if (!nv) return -1;
  DRIVER->imagev=nv;
  DRIVER->imagea=na;
  return 0;
}

/* Search images.
 */
 
int fmn_soft_image_search(const struct bigpc_render_driver *driver,uint16_t id) {
  if (id<DRIVER->imagecontigc) return id;
  int lo=DRIVER->imagecontigc,hi=DRIVER->imagec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct soft_image *q=DRIVER->imagev+ck;
         if (id<q->id) hi=ck;
    else if (id>q->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Get or add image.
 */
  
struct bigpc_image *fmn_soft_image_get(
  struct bigpc_render_driver *driver,
  uint16_t id,
  struct bigpc_image *(*cb_create)(void *userdata),
  void *userdata
) {
  int p=fmn_soft_image_search(driver,id);
  if (p<0) {
    struct fmn_soft_image_from_datafile_context ctx={.driver=driver,.id=id};
    if (!cb_create) {
      cb_create=fmn_soft_image_from_datafile;
      userdata=&ctx;
    }
    if (fmn_soft_image_require(driver)<0) return 0;
    struct bigpc_image *image=cb_create(userdata);
    if (!image) return 0;
    p=-p-1;
    struct soft_image *r=DRIVER->imagev+p;
    memmove(r+1,r,sizeof(struct soft_image)*(DRIVER->imagec-p));
    DRIVER->imagec++;
    while ((DRIVER->imagecontigc<DRIVER->imagec)&&(DRIVER->imagev[DRIVER->imagecontigc].id==DRIVER->imagecontigc)) DRIVER->imagecontigc++;
    r->id=id;
    r->image=image;
    return image;
  }
  struct soft_image *r=DRIVER->imagev+p;
  if (!r->image) {
    struct fmn_soft_image_from_datafile_context ctx={.driver=driver,.id=id};
    if (!cb_create) {
      cb_create=fmn_soft_image_from_datafile;
      userdata=&ctx;
    }
    r->image=cb_create(userdata);
  }
  return r->image;
}

#include "fontosaur_internal.h"

/* Search entries.
 */
 
static int fontosaur_search(int imageid) {
  int lo=0,hi=fontosaur.entryc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct fontosaur_cache_entry *q=fontosaur.entryv+ck;
         if (imageid<q->imageid) hi=ck;
    else if (imageid>q->imageid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Insert entry.
 */
 
static struct fontosaur_cache_entry *fontosaur_insert(int p,int imageid) {
  if ((p<0)||(p>fontosaur.entryc)) return 0;
  if (p&&(imageid<=fontosaur.entryv[p-1].imageid)) return 0;
  if ((p<fontosaur.entryc)&&(imageid>=fontosaur.entryv[p].imageid)) return 0;
  if (fontosaur.entryc>=fontosaur.entrya) {
    int na=fontosaur.entrya+8;
    if (na>INT_MAX/sizeof(struct fontosaur_cache_entry)) return 0;
    void *nv=realloc(fontosaur.entryv,sizeof(struct fontosaur_cache_entry)*na);
    if (!nv) return 0;
    fontosaur.entryv=nv;
    fontosaur.entrya=na;
  }
  struct fontosaur_cache_entry *entry=fontosaur.entryv+p;
  memmove(entry+1,entry,sizeof(struct fontosaur_cache_entry)*(fontosaur.entryc-p));
  fontosaur.entryc++;
  memset(entry,0,sizeof(struct fontosaur_cache_entry));
  entry->imageid=imageid;
  return entry;
}

/* Get font from imageid.
 */

struct fontosaur_image *fontosaur_get_image(struct fmn_datafile *file,int imageid) {
  int p=fontosaur_search(imageid);
  if (p>=0) return &fontosaur.entryv[p].image;
  p=-p-1;
  
  const void *serial=0;
  int serialc=fmn_datafile_get_any(&serial,file,FMN_RESTYPE_IMAGE,imageid);
  if (serialc<=0) {
    fprintf(stderr,"image:%d not found\n",imageid);
    return 0;
  }
  
  struct png_image *image=png_decode(serial,serialc);
  if (!image) {
    fprintf(stderr,"Failed to decode image:%d, %d bytes\n",imageid,serialc);
    return 0;
  }
  
  struct png_image *replace=png_image_reformat(image,0,0,image->w,image->h,8,0,0);
  png_image_del(image);
  if (!(image=replace)) return 0;
  
  struct fontosaur_cache_entry *entry=fontosaur_insert(p,imageid);
  if (!entry) {
    png_image_del(image);
    return 0;
  }
  
  entry->image.v=image->pixels;
  image->pixels=0;
  entry->image.w=image->w;
  entry->image.h=image->h;
  entry->image.pixelsize=8;
  png_image_del(image);
  
  return &entry->image;
}

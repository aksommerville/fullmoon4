#include "fmn_datafile_internal.h"

#define RD32(v) (((v)[0]<<24)|((v)[1]<<16)|((v)[2]<<8)|((v)[3]))

static int fmn_datafile_toc_entry_cmp(const void *A,const void *B) {
  const struct fmn_datafile_toc_entry *a=A,*b=B;
  if (a->type<b->type) return -1;
  if (a->type>b->type) return 1;
  if (a->id<b->id) return -1;
  if (a->id>b->id) return 1;
  if (a->qualifier<b->qualifier) return -1;
  if (a->qualifier>b->qualifier) return 1;
  return 0;
}

/* Decode TOC.
 */

int fmn_datafile_decode_toc(struct fmn_datafile *file) {
  file->entryc=0;
  
  // 12-byte header, and validate lengths.
  if (file->serialc<12) return -1;
  if (memcmp(file->serial,"\xff""AR\x00",4)) return -1;
  uint32_t tocc=RD32(file->serial+4);
  uint32_t addlc=RD32(file->serial+8);
  uint32_t datap=12;
  if (datap>UINT32_MAX-addlc) return -1;
  datap+=addlc;
  uint32_t tocp=datap;
  if (tocc&0xf00000) return -1;
  uint32_t toclen=tocc<<2;
  if (datap>UINT32_MAX-toclen) return -1;
  datap+=toclen;
  if (datap>file->serialc) return -1;
  
  // (tocc) includes state changes, so it is definitely more than the resource count.
  // But it makes a good upper bound, so let's allocate to that and then we don't have to check again.
  if (!tocc) return 0; // oh also, no entries? we're done.
  if (tocc>file->entrya) {
    if (tocc>INT_MAX/sizeof(struct fmn_datafile_toc_entry)) return -1;
    void *nv=realloc(file->entryv,sizeof(struct fmn_datafile_toc_entry)*tocc);
    if (!nv) return -1;
    file->entryv=nv;
    file->entrya=tocc;
  }
  
  // Set up an FSM and read thru. (tocp,datap) are also part of this FSM.
  uint16_t type=0;
  uint16_t qualifier=0;
  uint32_t id=1;
  struct fmn_datafile_toc_entry *prev=file->entryv;
  struct fmn_datafile_toc_entry *entry=file->entryv;
  for (;tocc-->0;tocp+=4) {
    uint32_t src=RD32(file->serial+tocp);
    if (src&0x80000000) { // state change
      if (src&0x7f000000) return -1; // reserved
      type=(src>>16)&0xff;
      qualifier=src;
      id=1;
    } else { // resource or placeholder; (src) is the absolute offset
      if (src&0x7f000000) return -1; // reserved
      if (src<datap) return -1; // offsets must be in order
      prev->c=src-prev->p; // each offset also tells us the length of the previous resource. (first visit here harmlessly computes some garbage)
      entry->type=type;
      entry->qualifier=qualifier;
      entry->id=id;
      entry->p=src;
      entry->c=0; // until we're told otherwise
      prev=entry;
      entry++;
      datap=src;
      file->entryc++;
      id++;
      //TODO Consider filtering on (qualifier), if we know one qualifier is locked in.
    }
  }
  // And the final entry runs to EOF. Since they're sequential, this is the only check we need against file length.
  if (prev->p>file->serialc) return -1;
  prev->c=file->serialc-prev->p;
  
  // We require empty entries, to maintain ID continuity.
  // They aren't needed anymore, so drop them.
  entry=file->entryv+file->entryc-1;
  int i=file->entryc;
  for (;i-->0;entry--) {
    if (!entry->c) {
      file->entryc--;
      memmove(entry,entry+1,sizeof(struct fmn_datafile_toc_entry)*(file->entryc-i));
    }
  }
  
  // Sort by (type,id,qualifier).
  qsort(file->entryv,file->entryc,sizeof(struct fmn_datafile_toc_entry),fmn_datafile_toc_entry_cmp);
  
  return 0;
}

/* TOC Iterators.
 */
 
int fmn_datafile_for_each_qualifier(
  struct fmn_datafile *file,
  uint16_t type,
  int (*cb)(uint16_t qualifier,void *userdata),
  void *userdata
) {
  if (!file||!cb) return -1;
  int16_t qv[16];
  int qc=0;
  const struct fmn_datafile_toc_entry *entry=file->entryv;
  int i=file->entryc,err;
  for (;i-->0;entry++) {
    if (entry->type>type) break;
    if (entry->type<type) continue;
    
    int j=qc,already=0; while (j-->0) {
      if (qv[j]==entry->qualifier) { already=1; break; }
    }
    if (already) continue;
    if (qc<sizeof(qv)/sizeof(qv[0])) qv[qc++]=entry->qualifier;
    if (err=cb(entry->qualifier,userdata)) return err;
  }
  return 0;
}
 
int fmn_datafile_for_each(
  struct fmn_datafile *file,
  int (*cb)(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata),
  void *userdata
) {
  if (!file||!cb) return -1;
  const struct fmn_datafile_toc_entry *entry=file->entryv;
  int i=file->entryc,err;
  for (;i-->0;entry++) {
    if (err=cb(entry->type,entry->qualifier,entry->id,file->serial+entry->p,entry->c,userdata)) return err;
  }
  return 0;
}

int fmn_datafile_for_each_of_type(
  struct fmn_datafile *file,
  uint16_t type,
  int (*cb)(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata),
  void *userdata
) {
  if (!file||!cb) return -1;
  int lo=0,hi=file->entryc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct fmn_datafile_toc_entry *entry=file->entryv+ck;
         if (type<entry->type) hi=ck;
    else if (type>entry->type) lo=ck+1;
    else {
      while ((ck>lo)&&(entry[-1].type==type)) { entry--; ck--; }
      lo=ck;
      break;
    }
  }
  const struct fmn_datafile_toc_entry *entry=file->entryv+lo;
  int i=file->entryc-lo,err;
  for (;i-->0;entry++) {
    if (entry->type!=type) break;
    if (err=cb(entry->type,entry->qualifier,entry->id,file->serial+entry->p,entry->c,userdata)) return err;
  }
  return 0;
}

int fmn_datafile_for_each_of_qualified_type(
  struct fmn_datafile *file,
  uint16_t type,uint16_t qualifier,
  int (*cb)(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata),
  void *userdata
) {
  if (!file||!cb) return -1;
  int lo=0,hi=file->entryc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct fmn_datafile_toc_entry *entry=file->entryv+ck;
         if (type<entry->type) hi=ck;
    else if (type>entry->type) lo=ck+1;
    else {
      while ((ck>lo)&&(entry[-1].type==type)) { entry--; ck--; }
      lo=ck;
      break;
    }
  }
  const struct fmn_datafile_toc_entry *entry=file->entryv+lo;
  int i=file->entryc-lo,err;
  for (;i-->0;entry++) {
    if (entry->type!=type) break;
    if (entry->qualifier!=qualifier) continue;
    if (err=cb(entry->type,entry->qualifier,entry->id,file->serial+entry->p,entry->c,userdata)) return err;
  }
  return 0;
}

int fmn_datafile_for_each_of_id(
  struct fmn_datafile *file,
  uint16_t type,uint32_t id,
  int (*cb)(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata),
  void *userdata
) {
  if (!file||!cb) return -1;
  int lo=0,hi=file->entryc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct fmn_datafile_toc_entry *entry=file->entryv+ck;
         if (type<entry->type) hi=ck;
    else if (type>entry->type) lo=ck+1;
    else if (id<entry->id) hi=ck;
    else if (id>entry->id) lo=ck+1;
    else {
      while ((ck>lo)&&(entry[-1].type==type)&&(entry[-1].id==id)) { entry--; ck--; }
      lo=ck;
      break;
    }
  }
  const struct fmn_datafile_toc_entry *entry=file->entryv+lo;
  int i=file->entryc-lo,err;
  for (;i-->0;entry++) {
    if (entry->id!=id) break;
    if (entry->type!=type) break;
    if (err=cb(entry->type,entry->qualifier,entry->id,file->serial+entry->p,entry->c,userdata)) return err;
  }
  return 0;
}

/* Get any resource of type and id.
 */
 
static int fmn_datafile_get_any_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  *(const void**)userdata=v;
  return c;
}
 
int fmn_datafile_get_any(void *dstpp,struct fmn_datafile *file,uint16_t type,uint32_t id) {
  return fmn_datafile_for_each_of_id(file,type,id,fmn_datafile_get_any_cb,dstpp);
}

struct fmn_datafile_get_qualified_context {
  void *dstpp;
  int dstc;
  uint16_t qualifier;
  void *alt;
  int altc;
};

static int fmn_datafile_get_qualified_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct fmn_datafile_get_qualified_context *ctx=userdata;
  if (qualifier==ctx->qualifier) {
    *(const void**)ctx->dstpp=v;
    ctx->dstc=c;
    return 1;
  }
  if (!qualifier||!ctx->alt) {
    ctx->alt=(void*)v;
    ctx->altc=c;
  }
  return 0;
}

int fmn_datafile_get_qualified(void *dstpp,struct fmn_datafile *file,uint16_t type,uint16_t qualifier,uint32_t id) {
  struct fmn_datafile_get_qualified_context ctx={
    .dstpp=dstpp,
    .qualifier=qualifier,
  };
  if (fmn_datafile_for_each_of_id(file,type,id,fmn_datafile_get_qualified_cb,&ctx)>0) return ctx.dstc;
  *(void**)dstpp=ctx.alt;
  return ctx.altc;
}

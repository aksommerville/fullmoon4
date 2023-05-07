#include "mkd_internal.h"
#include "mkd_ar.h"

/* Instance definition.
 */
 
struct mkd_ar {
  struct mkd_ar_entry {
    int type,qualifier,id,c;
    char *path;
    void *v;
  } *entryv;
  int entryc,entrya;
};

/* Cleanup.
 */
 
static void mkd_ar_entry_cleanup(struct mkd_ar_entry *entry) {
  if (entry->path) free(entry->path);
  if (entry->v) free(entry->v);
}
 
void mkd_ar_del(struct mkd_ar *ar) {
  if (!ar) return;
  if (ar->entryv) {
    while (ar->entryc-->0) mkd_ar_entry_cleanup(ar->entryv+ar->entryc);
    free(ar->entryv);
  }
  free(ar);
}

/* New.
 */
 
struct mkd_ar *mkd_ar_new() {
  struct mkd_ar *ar=calloc(1,sizeof(struct mkd_ar));
  if (!ar) return 0;
  return ar;
}

/* Decode.
 */
 
int mkd_ar_decode(struct mkd_ar *ar,const void *src,int srcc,const char *path) {
  if (!ar||ar->entryc) return -1;
  const uint8_t *SRC=src;
  #define FAIL(fmt,...) { \
    if (path) { \
      fprintf(stderr,"%s: "fmt"\n",path,##__VA_ARGS__); \
      return -2; \
    } \
    return -1; \
  }
  #define RD32(p) ((SRC[p]<<24)|(SRC[(p)+1]<<16)|(SRC[(p)+2]<<8)|SRC[(p)+3])
  
  // Validate header and chunk geometry.
  if (!src||(srcc<12)) FAIL("Short header.")
  if (memcmp(src,"\xff""AR\0",4)) FAIL("Signature mismatch.")
  uint32_t tocc=RD32(4);
  uint32_t addllen=RD32(8);
  if (tocc&0xf0000000) FAIL("Improbable entry count %u.",tocc)
  uint32_t toclen=tocc<<2;
  if (toclen>UINT32_MAX-addllen-12) FAIL("Improbable geometry tocc=%u addlc=%u.",tocc,addllen)
  uint32_t hdrlen=12+toclen+addllen;
  if (hdrlen>srcc) FAIL("Header and TOC %u bytes larger than input %u bytes.",hdrlen,srcc);
  
  // Set up a read position in TOC and Data, and begin iteration.
  uint32_t tocp=12+addllen;
  uint32_t datap=hdrlen;

  struct { int type,qualifier,id,datap; } pending={0};
  #define FLUSH_PENDING { \
    if (pending.datap&&(datap>pending.datap)) { \
      int err=mkd_ar_add_copy(ar,pending.type,pending.qualifier,pending.id,path,SRC+pending.datap,datap-pending.datap); \
      if (err<0) return err; \
    } \
    pending.type=pending.qualifier=pending.id=pending.datap=0; \
  }

  int type=0,qualifier=0,id=1,i=0;
  for (;i<tocc;i++,tocp+=4) {
    uint32_t tocr=RD32(tocp);
    
    // High bit set means State Change.
    if (tocr&0x80000000) {
      if (tocr&0x7f000000) FAIL("TOC %d/%d reserved bits set: 0x%08x.",i,tocc,tocr)
      type=(tocr>>16)&0xff;
      qualifier=tocr&0xffff;
      if (!type) FAIL("TOC %d/%d type=0: 0x%08x.",i,tocc,tocr)
      id=1;
      continue;
    }
    
    // Anything else is the 24-bit absolute offset to the next resource.
    if (tocr&0xff000000) FAIL("TOC %d/%d reserved bits set: 0x%08x.",i,tocc,tocr)
    if (!type) FAIL("Expected State Change before Resource (TOC %d/%d = 0x%08x)",i,tocc,tocr)
    if (tocr<datap) FAIL("TOC %d/%d offset 0x%08x before read head 0x%08x",i,tocc,tocr,datap)
    if (tocr>srcc) FAIL("TOC %d/%d 0x%08x exceeds file length 0x%08x",i,tocc,tocr,srcc)
    
    datap=tocr;
    FLUSH_PENDING
    
    pending.type=type;
    pending.qualifier=qualifier;
    pending.id=id;
    pending.datap=datap;
    id++;
  }
  
  FLUSH_PENDING
      
  #undef FAIL
  #undef RD32
  #undef FLUSH_PENDING
  return 0;
}

/* TOC primitives.
 */
 
static int mkd_ar_search(const struct mkd_ar *ar,int type,int qualifier,int id) {
  int lo=0,hi=ar->entryc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct mkd_ar_entry *entry=ar->entryv+ck;
         if (type<entry->type) hi=ck;
    else if (type>entry->type) lo=ck+1;
    else if (qualifier<entry->qualifier) hi=ck;
    else if (qualifier>entry->qualifier) lo=ck+1;
    else if (id<entry->id) hi=ck;
    else if (id>entry->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct mkd_ar_entry *mkd_ar_insert(struct mkd_ar *ar,int p,int type,int qualifier,int id) {
  if ((p<0)||(p>ar->entryc)) return 0;
  if ((type<0)||(type>0xff)) return 0;
  if ((qualifier<0)||(qualifier>0xffff)) return 0;
  if ((id<1)||(id>0xffff)) return 0; // 0xffff is not actually imposed by the format, but the app generally uses 16-bit ids.
  if (ar->entryc>=ar->entrya) {
    int na=ar->entrya+128;
    if (na>INT_MAX/sizeof(struct mkd_ar_entry)) return 0;
    void *nv=realloc(ar->entryv,sizeof(struct mkd_ar_entry)*na);
    if (!nv) return 0;
    ar->entryv=nv;
    ar->entrya=na;
  }
  struct mkd_ar_entry *entry=ar->entryv+p;
  memmove(entry+1,entry,sizeof(struct mkd_ar_entry)*(ar->entryc-p));
  ar->entryc++;
  memset(entry,0,sizeof(struct mkd_ar_entry));
  entry->type=type;
  entry->qualifier=qualifier;
  entry->id=id;
  return entry;
}

static void mkd_ar_remove_at(struct mkd_ar *ar,int p) {
  if ((p<0)||(p>=ar->entryc)) return;
  struct mkd_ar_entry *entry=ar->entryv+p;
  mkd_ar_entry_cleanup(entry);
  ar->entryc--;
  memmove(entry,entry+1,sizeof(struct mkd_ar_entry)*(ar->entryc-p));
}

/* Add resource.
 */

int mkd_ar_add_handoff(struct mkd_ar *ar,int type,int qualifier,int id,const char *path,void *v,int c) {
  int p=mkd_ar_search(ar,type,qualifier,id);
  if (p<0) {
    p=-p-1;
    if (!(mkd_ar_insert(ar,p,type,qualifier,id))) return -1;
  }
  struct mkd_ar_entry *entry=ar->entryv+p;
  if (entry->path) free(entry->path);
  if (path) entry->path=strdup(path);
  else entry->path=0;
  if (entry->v) free(entry->v);
  entry->v=v;
  entry->c=c;
  return 0;
}

int mkd_ar_add_copy(struct mkd_ar *ar,int type,int qualifier,int id,const char *path,const void *v,int c) {
  if (!v) c=0;
  if (c<0) return -1;
  void *nv=0;
  if (c) {
    if (!(nv=malloc(c))) return -1;
    memcpy(nv,v,c);
  }
  if (mkd_ar_add_handoff(ar,type,qualifier,id,path,nv,c)<0) {
    if (nv) free(nv);
    return -1;
  }
  return 0;
}

/* Modify resource.
 */
 
int mkd_ar_set_path(struct mkd_ar *ar,int type,int qualifier,int id,const char *path) {
  int p=mkd_ar_search(ar,type,qualifier,id);
  if (p<0) return -1;
  struct mkd_ar_entry *entry=ar->entryv+p;
  if (entry->path) free(entry->path);
  if (path) entry->path=strdup(path);
  else entry->path=0;
  return 0;
}

int mkd_ar_set_serial(struct mkd_ar *ar,int type,int qualifier,int id,const void *v,int c) {
  if ((c<0)||(c&&!v)) return -1;
  int p=mkd_ar_search(ar,type,qualifier,id);
  if (p<0) return -1;
  struct mkd_ar_entry *entry=ar->entryv+p;
  void *nv=0;
  if (c) {
    if (!(nv=malloc(c))) return -1;
    memcpy(nv,v,c);
  }
  if (entry->v) free(entry->v);
  entry->v=nv;
  entry->c=c;
  return 0;
}

/* Remove resource.
 */
 
int mkd_ar_remove(struct mkd_ar *ar,int type,int qualifier,int id) {
  int p=mkd_ar_search(ar,type,qualifier,id);
  if (p<0) return -1;
  mkd_ar_remove_at(ar,p);
  return 0;
}

/* Get resource content.
 */
 
int mkd_ar_count(const struct mkd_ar *ar) {
  return ar->entryc;
}
 
int mkd_ar_exists(const struct mkd_ar *ar,int type,int qualifier,int id) {
  int p=mkd_ar_search(ar,type,qualifier,id);
  return (p>=0)?1:0;
}

const char *mkd_ar_get_path(const struct mkd_ar *ar,int type,int qualifier,int id) {
  int p=mkd_ar_search(ar,type,qualifier,id);
  if (p<0) return 0;
  const struct mkd_ar_entry *entry=ar->entryv+p;
  return entry->path;
}

int mkd_ar_get_serial(void *dstpp,const struct mkd_ar *ar,int type,int qualifier,int id) {
  int p=mkd_ar_search(ar,type,qualifier,id);
  if (p<0) return -1;
  const struct mkd_ar_entry *entry=ar->entryv+p;
  if (dstpp) *(void**)dstpp=entry->v;
  return entry->c;
}

/* Iterate resources.
 */

int mkd_ar_for_each(
  struct mkd_ar *ar,
  int (*cb)(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata),
  void *userdata
) {
  const struct mkd_ar_entry *entry=ar->entryv;
  int i=ar->entryc,err;
  for (;i-->0;entry++) {
    if (err=cb(entry->type,entry->qualifier,entry->id,entry->path,entry->v,entry->c,userdata)) return err;
  }
  return 0;
}

int mkd_ar_for_each_of_type(
  struct mkd_ar *ar,int type,
  int (*cb)(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata),
  void *userdata
) {
  int p=mkd_ar_search(ar,type,0,0);
  if (p<0) p=-p-1;
  const struct mkd_ar_entry *entry=ar->entryv+p;
  int i=ar->entryc-p,err;
  for (;i-->0;entry++) {
    if (entry->type!=type) break;
    if (err=cb(entry->type,entry->qualifier,entry->id,entry->path,entry->v,entry->c,userdata)) return err;
  }
  return 0;
}

int mkd_ar_for_each_of_type_qualifier(
  struct mkd_ar *ar,int type,int qualifier,
  int (*cb)(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata),
  void *userdata
) {
  int p=mkd_ar_search(ar,type,qualifier,0);
  if (p<0) p=-p-1;
  const struct mkd_ar_entry *entry=ar->entryv+p;
  int i=ar->entryc-p,err;
  for (;i-->0;entry++) {
    if (entry->type!=type) break;
    if (entry->qualifier!=qualifier) break;
    if (err=cb(entry->type,entry->qualifier,entry->id,entry->path,entry->v,entry->c,userdata)) return err;
  }
  return 0;
}

/* Filter resources.
 */
 
int mkd_ar_filter(
  struct mkd_ar *ar,
  int (*test)(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata),
  void *userdata
) {
  int i=ar->entryc,rmc=0;
  struct mkd_ar_entry *entry=ar->entryv+i-1;
  for (;i-->0;entry--) {
    if (test(entry->type,entry->qualifier,entry->id,entry->path,entry->v,entry->c,userdata)) continue;
    mkd_ar_entry_cleanup(entry);
    ar->entryc--;
    memmove(entry,entry+1,sizeof(struct mkd_ar_entry)*(ar->entryc-i));
    rmc++;
  }
  return rmc;
}

/* Generate the real TOC with state changes and all, but in the native byte order.
 */
 
static int mkd_ar_generate_toc(uint32_t **dstpp,const struct mkd_ar *ar) {
  int tocc=0,toca=256;
  uint32_t *tocv=malloc(toca<<2);
  if (!tocv) return -1;
  int pvtype=-1,pvqualifier=-1,nextid=1;
  int datac=0;
  const struct mkd_ar_entry *entry=ar->entryv;
  int i=ar->entryc;
  for (;i-->0;entry++) {
  
    #define ADDTOC(v) { \
      if (tocc>=toca) { \
        toca+=256; \
        void *nv=realloc(tocv,toca<<2); \
        if (!nv) { free(tocv); return -1; } \
        tocv=nv; \
      } \
      tocv[tocc++]=(v); \
    }
  
    // State Change on new type or qualifier.
    if ((pvtype!=entry->type)||(pvqualifier!=entry->qualifier)) {
      ADDTOC(0x80000000|(entry->type<<16)|entry->qualifier)
      pvtype=entry->type;
      pvqualifier=entry->qualifier;
      nextid=1;
    }
    
    // Emit dummies if IDs are discontiguous.
    while (nextid<entry->id) {
      ADDTOC(datac)
      nextid++;
    }
    if (nextid>entry->id) {
      fprintf(stderr,"Resources not sorted by id.\n");
      free(tocv);
      return -1;
    }
    
    // Emit a record for this resource, and record its payload length.
    ADDTOC(datac)
    nextid++;
    datac+=entry->c;
    
    #undef ADDTOC
  }
  *dstpp=tocv;
  return tocc;
}

/* Encode.
 */

int mkd_ar_encode(struct sr_encoder *dst,const struct mkd_ar *ar) {

  /* It's a little chicken-and-eggy.
   * TOC must contain absolute offsets from the start of the file.
   * We don't know those until we've determined the full TOC length.
   * I'll try to keep it clean by generating the TOC separately first.
   * Start with offsets relative to the data section, then we can add the header, addl, and toc lengths after. Then encode it.
   */
   
  uint32_t *tocv=0;
  int tocc=mkd_ar_generate_toc(&tocv,ar);
  if (tocc<0) return -1;
  
  int hdrlen=12;
  int addllen=0;
  int toclen=tocc<<2;
  int datap=hdrlen+addllen+toclen;
  
  uint32_t *v=tocv;
  int i=tocc;
  for (;i-->0;v++) {
    if ((*v)&0x80000000) continue;
    (*v)+=datap;
    if ((*v)&0xff000000) {
      fprintf(stderr,"Resource data too long, must be within 24 bits.\n");
      free(tocv);
      return -1;
    }
  }
  
  // Emit header.
  if (sr_encode_raw(dst,"\xff""AR\x00",4)<0) { free(tocv); return -1; }
  if (sr_encode_intbe(dst,tocc,4)<0) { free(tocv); return -1; }
  if (sr_encode_intbe(dst,addllen,4)<0) { free(tocv); return -1; }
  // addl header would go here if we had any.
  
  // Emit TOC.
  for (v=tocv,i=tocc;i-->0;v++) {
    if (sr_encode_intbe(dst,*v,4)<0) { free(tocv); return -1; }
  }
  free(tocv);
  
  // Emit resource payloads.
  const struct mkd_ar_entry *entry=ar->entryv;
  for (i=ar->entryc;i-->0;entry++) {
    if (sr_encode_raw(dst,entry->v,entry->c)<0) return -1;
  }
  
  return 0;
}

#include "datan_internal.h"

/* gsbit registry
 */
 
int datan_gsbit_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  const struct datan_gsbit *gsbit=datan.gsbitv;
  int i=datan.gsbitc;
  for (;i-->0;gsbit++) {
    if (gsbit->namec!=namec) continue;
    if (memcmp(gsbit->name,name,namec)) continue;
    return gsbit->id;
  }
  return 0;
}

int datan_gsbit_search_id(int id) {
  // It is likely that every call to this function will be for the first unavailable ID,
  // which would normally be a worst-case scenario for the binary search.
  if (id<1) return -1;
  if (datan.gsbitc<1) return -1;
  if (id>datan.gsbitv[datan.gsbitc-1].id) return -datan.gsbitc-1;
  int lo=0,hi=datan.gsbitc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct datan_gsbit *gsbit=datan.gsbitv+ck;
         if (id<gsbit->id) hi=ck;
    else if (id>gsbit->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int datan_gsbit_insert(int p,int id,const char *name_borrow,int namec) {
  if ((p<0)||(p>datan.gsbitc)) return -1;
  if (id<1) return -1;
  if (!name_borrow||(namec<1)) return -1;
  if (p&&(id<=datan.gsbitv[p-1].id)) return -1;
  if ((p<datan.gsbitc)&&(id>=datan.gsbitv[p].id)) return -1;
  if (datan.gsbitc>=datan.gsbita) {
    int na=datan.gsbita+128;
    if (na>INT_MAX/sizeof(struct datan_gsbit)) return -1;
    void *nv=realloc(datan.gsbitv,sizeof(struct datan_gsbit)*na);
    if (!nv) return -1;
    datan.gsbitv=nv;
    datan.gsbita=na;
  }
  struct datan_gsbit *gsbit=datan.gsbitv+p;
  memmove(gsbit+1,gsbit,sizeof(struct datan_gsbit)*(datan.gsbitc-p));
  datan.gsbitc++;
  gsbit->id=id;
  gsbit->name=name_borrow;
  gsbit->namec=namec;
  return 0;
}

/* gsbit: one line
 * It is safe to borrow (src).
 */
 
static int read_gsbit_line(const char *src,int srcc,const char *path,int lineno) {
  int srcp=0,id=0;
  while ((srcp<srcc)&&(src[srcp]>='0')&&(src[srcp]<='9')) {
    id*=10;
    id+=src[srcp++]-'0';
  }
  if ((id<1)||(srcp>=srcc)||((unsigned char)src[srcp]>0x20)) {
    fprintf(stderr,"%s:%d: Malformed line. Expected positive decimal integer followed by space then name.\n",path,lineno);
    return -2;
  }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp>=srcc) {
    fprintf(stderr,"%s:%d: Empty name is forbidden. Delete this line if you're not using it.\n",path,lineno);
    return -2;
  }
  int already=datan_gsbit_by_name(src+srcp,srcc-srcp);
  if (already) {
    fprintf(stderr,"%s:%d: Redefinition of gsbit name '%.*s': %d and %d\n",path,lineno,srcc-srcp,src+srcp,already,id);
    return -2;
  }
  int p=datan_gsbit_search_id(id);
  if (p>=0) {
    fprintf(stderr,"%s:%d: Redefinition of bit %d\n",path,lineno,id);
    return -2;
  }
  p=-p-1;
  if (datan_gsbit_insert(p,id,src+srcp,srcc-srcp)<0) return -1;
  return 0;
}

/* Acquire and validate gsbit definitions.
 */
 
int datan_acquire_gsbit() {
  if (datan.gsbittext) {
    free(datan.gsbittext);
    datan.gsbittext=0;
  }
  datan.gsbitc=0;
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"%s/gsbit",datan.srcpath);
  if ((pathc<1)||(pathc>=sizeof(path))) return -1;
  int srcc=fmn_file_read(&datan.gsbittext,path);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read gsbit file.\n",path);
    return -2;
  }
  struct sr_decoder decoder={.v=datan.gsbittext,.c=srcc};
  const char *line;
  int linec,lineno=1;
  for (;linec=sr_decode_line(&line,&decoder);lineno++) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    int err=read_gsbit_line(line,linec,path,lineno);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error processing gsbit line: '%.*s'\n",path,lineno,linec,line);
      return -2;
    }
  }
  return 0;
}

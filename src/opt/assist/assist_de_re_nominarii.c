#include "assist_internal.h"

/* Name of sound effect.
 */
 
static struct assist_sfxname {
  int id;
  char *name;
  int namec;
} *assist_sfxnamev=0;
static int assist_sfxnamec=0,assist_sfxnamea=0;

static int assist_sfxnamev_require() {
  if (assist_sfxnamec<assist_sfxnamea) return 0;
  int na=assist_sfxnamea+64;
  if (na>INT_MAX/sizeof(struct assist_sfxname)) return -1;
  void *nv=realloc(assist_sfxnamev,sizeof(struct assist_sfxname)*na);
  if (!nv) return -1;
  assist_sfxnamev=nv;
  assist_sfxnamea=na;
  return 0;
}

static int assist_sfxnamev_search(int id) {
  int lo=0,hi=assist_sfxnamec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct assist_sfxname *q=assist_sfxnamev+ck;
         if (id<q->id) hi=ck;
    else if (id>q->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int assist_sfxnamev_insert(int p,int id,const char *name,int namec) {
  if (assist_sfxnamev_require()<0) return -1;
  char *nv=malloc(namec+1);
  if (!nv) return -1;
  memcpy(nv,name,namec);
  nv[namec]=0;
  struct assist_sfxname *entry=assist_sfxnamev+p;
  memmove(entry+1,entry,sizeof(struct assist_sfxname)*(assist_sfxnamec-p));
  assist_sfxnamec++;
  entry->id=id;
  entry->name=nv;
  entry->namec=namec;
  return 0;
}

static int assist_read_sound_names(const char *path) {
  if (assist_sfxnamev_require()<0) return -1; // don't leave the buffer empty, even (especially!) if reading fails
  char *src=0;
  int srcc=fmn_file_read(&src,path);
  if (srcc<0) return -1;
  int srcp=0,lineno=1;
  for (;srcp<srcc;lineno++) {
  
    const char *line=src+srcp;
    int linec=0;
    while ((srcp<srcc)&&(src[srcp++]!=0x0a)) linec++;
    if ((linec<16)||memcmp(line,"#define FMN_SFX_",16)) continue;
    int linep=16;
    const char *name=line+linep;
    int namec=0;
    while ((linep<linec)&&((unsigned char)line[linep]>0x20)) { linep++; namec++; }
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    const char *idtoken=line+linep;
    int idtokenc=0;
    while ((linep<linec)&&((unsigned char)line[linep]>0x20)) { linep++; idtokenc++; }
    if (idtokenc<1) continue;
    int id=0; // ids are all positive, and all written decimal, easy.
    for (;idtokenc-->0;idtoken++) {
      if ((*idtoken<'0')||(*idtoken>'9')) { id=-1; break; }
      id*=10;
      id+=(*idtoken)-'0';
    }
    if (id<1) continue;
    
    int p=assist_sfxnamev_search(id);
    if (p>=0) {
      fprintf(stderr,
        "%s:WARNING: Duplicate names '%.*s' and '%.*s' for sound id %d.\n",
        path,assist_sfxnamev[p].namec,assist_sfxnamev[p].name,namec,name,id
      );
      continue;
    }
    p=-p-1;
    assist_sfxnamev_insert(p,id,name,namec);
  }
  free(src);
  return 0;
}
 
int assist_get_sound_name(void *dstpp,int id) {
  if (!assist_sfxnamea) assist_read_sound_names("src/app/fmn_platform.h");
  int p=assist_sfxnamev_search(id);
  if (p<0) return 0;
  *(void**)dstpp=assist_sfxnamev[p].name;
  return assist_sfxnamev[p].namec;
}

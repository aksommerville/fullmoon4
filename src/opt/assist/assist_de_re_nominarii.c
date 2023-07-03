#include "assist_internal.h"

/* Globals.
 */
 
#define ASSIST_NAMEFILE_PLATFORM 1 /* src/app/fmn_platform.h */
#define ASSIST_NAMEFILE_SPRITE   2 /* src/app/sprite/fmn_sprite.h */
#define ASSIST_NAMEFILE_GAME     3 /* src/app/fmn_game.h */

// We'll capture names based on their macro, wherever we find them. But expect them only in the named file.
#define ASSIST_CAT_SFX 1 /* PLATFORM */
#define ASSIST_CAT_SPRCTL 2 /* SPRITE */
#define ASSIST_CAT_ITEM 3 /* PLATFORM */
#define ASSIST_CAT_SPRITE_STYLE 4 /* PLATFORM */
#define ASSIST_CAT_CELLPHYSICS 5 /* PLATFORM */
#define ASSIST_CAT_SPELLID 6 /* PLATFORM */
#define ASSIST_CAT_PHYSICS 7 /* SPRITE */
#define ASSIST_CAT_XFORM 8 /* PLATFORM. Kind of silly to generalize these, they won't change and there's only three. */
#define ASSIST_CAT_MAP_EVID 9 /* PLATFORM */
#define ASSIST_CAT_MAP_CALLBACK 10 /* GAME */
 
static struct assist_names {
  struct assist_namefile {
    int namefile_id;
    struct assist_name {
      int cat;
      int id;
      char *v;
      int c;
    } *namev;
    int namec,namea;
  } *namefilev;
  int namefilec,namefilea;
  struct assist_restype {
    char *tname;
    int tnamec;
    struct assist_res {
      int id;
      int qualifier; // unused
      char *name;
      int namec;
    } *resv;
    int resc,resa;
  } *restypev;
  int restypec,restypea;
  struct assist_gsbit {
    int gsbit;
    char *name;
    int namec;
  } *gsbitv;
  int gsbitc,gsbita;
} assist_names={0};

/* List primitives.
 */
 
static int assist_namefilev_search(int id) {
  int lo=0,hi=assist_names.namefilec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (id<assist_names.namefilev[ck].namefile_id) hi=ck;
    else if (id>assist_names.namefilev[ck].namefile_id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct assist_namefile *assist_namefilev_insert(int p,int id) {
  if ((p<0)||(p>assist_names.namefilec)) return 0;
  if (assist_names.namefilec>=assist_names.namefilea) {
    int na=assist_names.namefilea+8;
    if (na>INT_MAX/sizeof(struct assist_namefile)) return 0;
    void *nv=realloc(assist_names.namefilev,sizeof(struct assist_namefile)*na);
    if (!nv) return 0;
    assist_names.namefilev=nv;
    assist_names.namefilea=na;
  }
  struct assist_namefile *namefile=assist_names.namefilev+p;
  memmove(namefile+1,namefile,sizeof(struct assist_namefile)*(assist_names.namefilec-p));
  assist_names.namefilec++;
  memset(namefile,0,sizeof(struct assist_namefile));
  namefile->namefile_id=id;
  return namefile;
}

static int assist_namev_search_cat_id(const struct assist_namefile *namefile,int cat,int id) {
  int lo=0,hi=namefile->namec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct assist_name *name=namefile->namev+ck;
         if (cat<name->cat) hi=ck;
    else if (cat>name->cat) lo=ck+1;
    else if (id<name->id) hi=ck;
    else if (id>name->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int assist_namev_insert(struct assist_namefile *namefile,int p,int cat,int id,const char *v,int c) {
  if ((p<0)||(p>namefile->namec)) return -1;
  if (namefile->namec>=namefile->namea) {
    int na=namefile->namea+128;
    if (na>INT_MAX/sizeof(struct assist_name)) return -1;
    void *nv=realloc(namefile->namev,sizeof(struct assist_name)*na);
    if (!nv) return -1;
    namefile->namev=nv;
    namefile->namea=na;
  }
  char *nv=malloc(c+1);
  if (!nv) return -1;
  memcpy(nv,v,c);
  nv[c]=0;
  struct assist_name *name=namefile->namev+p;
  memmove(name+1,name,sizeof(struct assist_name)*(namefile->namec-p));
  namefile->namec++;
  name->cat=cat;
  name->id=id;
  name->v=nv;
  name->c=c;
  return 0;
}

static int assist_id_by_cat_name(const struct assist_namefile *namefile,int cat,const char *v,int c) {
  int p=assist_namev_search_cat_id(namefile,cat,0);
  if (p<0) p=-p-1;
  const struct assist_name *name=namefile->namev+p;
  for (;p<namefile->namec;p++,name++) {
    if (name->cat!=cat) break;
    if (name->c!=c) continue;
    if (memcmp(name->v,v,c)) continue;
    return name->id;
  }
  return -1;
}

static int assist_name_add(struct assist_namefile *namefile,int cat,int id,const char *v,int c) {
  int p=assist_namev_search_cat_id(namefile,cat,id);
  if (p>=0) return 0; // keep the first one, tho maybe we should report an error
  p=-p-1;
  return assist_namev_insert(namefile,p,cat,id,v,c);
}

static int assist_res_search(const struct assist_restype *restype,int id,int qualifier) {
  int lo=0,hi=restype->resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct assist_res *res=restype->resv+ck;
         if (id<res->id) hi=ck;
    else if (id>res->id) lo=ck+1;
    else if (qualifier<res->qualifier) hi=ck;
    else if (qualifier>res->qualifier) lo=ck=1;
    else return ck;
  }
  return -lo-1;
}

static int assist_res_insert(struct assist_restype *restype,int p,int id,int qualifier,const char *v,int c) {
  if ((p<0)||(p>restype->resc)) return -1;
  if (restype->resc>=restype->resa) {
    int na=restype->resa+32;
    if (na>INT_MAX/sizeof(struct assist_res)) return -1;
    void *nv=realloc(restype->resv,sizeof(struct assist_res)*na);
    if (!nv) return -1;
    restype->resv=nv;
    restype->resa=na;
  }
  char *nv=malloc(c+1);
  if (!nv) return -1;
  memcpy(nv,v,c);
  nv[c]=0;
  struct assist_res *res=restype->resv+p;
  memmove(res+1,res,sizeof(struct assist_res)*(restype->resc-p));
  restype->resc++;
  memset(res,0,sizeof(struct assist_res));
  res->id=id;
  res->qualifier=qualifier;
  res->name=nv;
  res->namec=c;
  return 0;
}

static int assist_restype_id_by_name(const struct assist_restype *restype,const char *v,int c) {
  if (!v) return -1;
  if (c<0) { c=0; while (v[c]) c++; }
  const struct assist_res *res=restype->resv;
  int i=restype->resc;
  for (;i-->0;res++) {
    if (res->namec!=c) continue;
    if (memcmp(res->name,v,c)) continue;
    return res->id;
  }
  return -1;
}

static int assist_gsbit_search(int gsbit) {
  int lo=0,hi=assist_names.gsbitc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct assist_gsbit *r=assist_names.gsbitv+ck;
         if (gsbit<r->gsbit) hi=ck;
    else if (gsbit>r->gsbit) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int assist_gsbit_insert(int p,int gsbit,const char *name,int namec) {
  if ((p<0)||(p>assist_names.gsbitc)) return -1;
  if (assist_names.gsbitc>=assist_names.gsbita) {
    int na=assist_names.gsbita+128;
    if (na>INT_MAX/sizeof(struct assist_gsbit)) return -1;
    void *nv=realloc(assist_names.gsbitv,sizeof(struct assist_gsbit)*na);
    if (!nv) return -1;
    assist_names.gsbitv=nv;
    assist_names.gsbita=na;
  }
  char *nv=malloc(namec+1);
  if (!nv) return -1;
  memcpy(nv,name,namec);
  nv[namec]=0;
  struct assist_gsbit *r=assist_names.gsbitv+p;
  memmove(r+1,r,sizeof(struct assist_gsbit)*(assist_names.gsbitc-p));
  assist_names.gsbitc++;
  r->gsbit=gsbit;
  r->name=nv;
  r->namec=namec;
  return 0;
}

static int assist_gsbit_id_by_name(const char *name,int namec) {
  const struct assist_gsbit *r=assist_names.gsbitv;
  int i=assist_names.gsbitc;
  for (;i-->0;r++) {
    if (r->namec!=namec) continue;
    if (memcmp(r->name,name,namec)) continue;
    return r->gsbit;
  }
  return -1;
}

/* Evaluate symbol value.
 * We only accept simple positive integers. Decimal or hexadecimal.
 */
 
static int assist_namefile_value_eval(int *dst,const char *src,int srcc) {
  if (srcc<1) return -1;
  *dst=0;
  if ((srcc>=3)&&(src[0]=='0')&&(src[1]=='x')) {
    int srcp=2;
    for (;srcp<srcc;srcp++) {
      int digit;
           if ((src[srcp]>='0')&&(src[srcp]<='9')) digit=src[srcp]-'0';
      else if ((src[srcp]>='a')&&(src[srcp]<='f')) digit=src[srcp]-'a'+10;
      else if ((src[srcp]>='A')&&(src[srcp]<='F')) digit=src[srcp]-'A'+10;
      else return -1;
      (*dst)<<=4;
      (*dst)|=digit;
    }
    return 0;
  }
  int srcp=0;
  for (;srcp<srcc;srcp++) {
    int digit=src[srcp]-'0';
    if ((digit<0)||(digit>9)) return -1;
    (*dst)*=10;
    (*dst)+=digit;
  }
  return 0;
}

/* Read symbols into a new namefile.
 */
 
static int assist_namefile_read(struct assist_namefile *namefile,const char *src,int srcc,const char *path) {
  int srcp=0,lineno=1;
  for (;srcp<srcc;lineno++) {
    const char *line=src+srcp;
    int linec=0;
    while ((srcp<srcc)&&(src[srcp++]!=0x0a)) linec++;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    if ((linec<7)||memcmp(line,"#define",7)) continue;
    line+=7;
    linec-=7;
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    
    const char *symbol=line;
    int symbolc=0;
    while (linec&&((unsigned char)line[0]>0x20)) { line++; linec--; symbolc++; }
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    
    const char *vtoken=line;
    int vtokenc=0;
    while (linec&&((unsigned char)line[0]>0x20)) { line++; linec--; vtokenc++; }
    int v;
    if (assist_namefile_value_eval(&v,vtoken,vtokenc)<0) continue;
    
    #define _(pfxstr,cattag) if ((symbolc>=sizeof(pfxstr))&&!memcmp(symbol,pfxstr,sizeof(pfxstr)-1)) { \
      if (assist_name_add(namefile,ASSIST_CAT_##cattag,v,symbol+sizeof(pfxstr)-1,symbolc-sizeof(pfxstr)+1)<0) return -1; \
    } else
    _("FMN_SFX_",SFX)
    _("FMN_SPRCTL_",SPRCTL)
    _("FMN_SPRITE_STYLE_",SPRITE_STYLE)
    _("FMN_CELLPHYSICS_",CELLPHYSICS)
    _("FMN_SPELLID_",SPELLID)
    _("FMN_PHYSICS_",PHYSICS)
    _("FMN_XFORM_",XFORM)
    _("FMN_ITEM_",ITEM)
    _("FMN_MAP_EVID_",MAP_EVID)
    _("FMN_MAP_CALLBACK_",MAP_CALLBACK)
    continue;
    #undef _
  }
  return 0;
}

/* Get a namefile, and create if absent.
 */
 
static struct assist_namefile *assist_namefile_get(int namefile_id) {
  int p=assist_namefilev_search(namefile_id);
  if (p>=0) return assist_names.namefilev+p;
  p=-p-1;
  struct assist_namefile *namefile=assist_namefilev_insert(p,namefile_id);
  if (!namefile) return 0;
  const char *path=0;
  switch (namefile_id) {
    case ASSIST_NAMEFILE_PLATFORM: path="src/app/fmn_platform.h"; break;
    case ASSIST_NAMEFILE_SPRITE: path="src/app/sprite/fmn_sprite.h"; break;
    case ASSIST_NAMEFILE_GAME: path="src/app/fmn_game.h"; break;
    default: return namefile;
  }
  char *src=0;
  int srcc=fmn_file_read(&src,path);
  if (srcc<0) return namefile;
  assist_namefile_read(namefile,src,srcc,path);
  free(src);
  return namefile;
}

/* Add a resource with ID and name.
 */
 
static int assist_restype_add(struct assist_restype *restype,int id,const char *v,int c) {
  int p=assist_res_search(restype,id,0);
  if (p>=0) return 0;
  p=-p-1;
  return assist_res_insert(restype,p,id,0,v,c);
}

/* Populate a new restype.
 */
 
static int assist_restype_read_cb(const char *path,const char *base,char type,void *userdata) {
  struct assist_restype *restype=userdata;
  if (type=='d') return 0;
  
  int id=0;
  for (;*base;base++) {
    if (*base=='-') break;
    if (*base=='.') break;
    int digit=(*base)-'0';
    if ((digit<0)||(digit>10)) return 0;
    id*=10;
    id+=digit;
  }
  
  if (id>0) {
    while (*base=='-') base++;
    int basec=0;
    while (base[basec]&&(base[basec]!='.')) basec++;
    if (basec) {
      assist_restype_add(restype,id,base,basec);
    }
  }
  
  return 0;
}
 
static int assist_restype_read(struct assist_restype *restype) {
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"src/data/%.*s",restype->tnamec,restype->tname);
  if ((pathc<1)||(pathc>=sizeof(path))) return -1;
  fmn_dir_read(path,assist_restype_read_cb,restype);
  return 0;
}

/* Get a restype, and create if absent.
 */
 
static struct assist_restype *assist_restype_get(const char *tname) {
  if (!tname||!tname[0]) return 0;
  int tnamec=0;
  while (tname[tnamec]) tnamec++;
  struct assist_restype *restype=assist_names.restypev;
  int i=assist_names.restypec;
  for (;i-->0;restype++) {
    if (restype->tnamec!=tnamec) continue;
    if (memcmp(restype->tname,tname,tnamec)) continue;
    return restype;
  }
  if (assist_names.restypec>=assist_names.restypea) {
    int na=assist_names.restypea+16;
    if (na>INT_MAX/sizeof(struct assist_restype)) return 0;
    void *nv=realloc(assist_names.restypev,sizeof(struct assist_restype)*na);
    if (!nv) return 0;
    assist_names.restypev=nv;
    assist_names.restypea=na;
  }
  char *nv=malloc(tnamec+1);
  if (!nv) return 0;
  memcpy(nv,tname,tnamec);
  nv[tnamec]=0;
  restype=assist_names.restypev+assist_names.restypec++;
  memset(restype,0,sizeof(struct assist_restype));
  restype->tname=nv;
  restype->tnamec=tnamec;
  assist_restype_read(restype);
  return restype;
}

/* Read and store gsbit, if we haven't yet.
 */
 
static int assist_gsbit_read(const char *src,int srcc) {
  int srcp=0;
  while (srcp<srcc) {
    const char *line=src+srcp;
    int linec=0;
    while ((srcp<srcc)&&(src[srcp++]!=0x0a)) linec++;
    
    int linep=0;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    
    int id=0;
    while ((linep<linec)&&(line[linep]>='0')&&(line[linep]<='9')) {
      id*=10;
      id+=line[linep++]-'0';
    }
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    
    const char *name=line+linep;
    int namec=linec-linep;
    while (namec&&((unsigned char)name[namec-1]<=0x20)) namec--;
    
    int p=assist_gsbit_search(id);
    if (p<0) {
      p=-p-1;
      if (assist_gsbit_insert(p,id,name,namec)<0) return -1;
    }
  }
  return 0;
}
 
static int assist_gsbit_require() {
  if (assist_names.gsbita>0) return 0;
  // Preallocate gsbitv a little, so in case it's empty we won't keep retrying.
  if (!(assist_names.gsbitv=malloc(sizeof(struct assist_gsbit)*128))) return -1;
  assist_names.gsbita=128;
  char *src=0;
  int srcc=fmn_file_read(&src,"src/data/gsbit");
  if (srcc<0) return 0;
  int err=assist_gsbit_read(src,srcc);
  free(src);
  return err;
}

/* Public entry points.
 */
 
int assist_get_sound_name(void *dstpp,int id) {
  struct assist_namefile *namefile=assist_namefile_get(ASSIST_NAMEFILE_PLATFORM);
  if (!namefile) return 0;
  int p=assist_namev_search_cat_id(namefile,ASSIST_CAT_SFX,id);
  if (p<0) return 0;
  const struct assist_name *name=namefile->namev+p;
  *(void**)dstpp=name->v;
  return name->c;
}

int assist_get_sound_id_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  struct assist_namefile *namefile=assist_namefile_get(ASSIST_NAMEFILE_PLATFORM);
  if (!namefile) return 0;
  return assist_id_by_cat_name(namefile,ASSIST_CAT_SFX,name,namec);
}

int assist_get_sprite_name(void *dstpp,int id) {
  struct assist_namefile *namefile=assist_namefile_get(ASSIST_NAMEFILE_SPRITE);
  if (!namefile) return 0;
  int p=assist_namev_search_cat_id(namefile,ASSIST_CAT_SPRCTL,id);
  if (p<0) return 0;
  const struct assist_name *name=namefile->namev+p;
  *(void**)dstpp=name->v;
  return name->c;
}

int assist_get_sprite_id_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  struct assist_namefile *namefile=assist_namefile_get(ASSIST_NAMEFILE_SPRITE);
  if (!namefile) return 0;
  return assist_id_by_cat_name(namefile,ASSIST_CAT_SPRCTL,name,namec);
}

int assist_get_sprite_style_by_name(const char *name,int namec) {
  // I goofed and defined these lower-case in the file, but upper-case as macros.
  if (!name) return -1;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  char norm[16];
  if (namec>=sizeof(norm)) return -1;
  int i=namec; while (i-->0) if ((name[i]>='a')&&(name[i]<='z')) norm[i]=name[i]-0x20; else norm[i]=name[i];
  struct assist_namefile *namefile=assist_namefile_get(ASSIST_NAMEFILE_PLATFORM);
  if (!namefile) return -1;
  return assist_id_by_cat_name(namefile,ASSIST_CAT_SPRITE_STYLE,norm,namec);
}

int assist_get_sprite_physics_by_name(const char *name,int namec) {
  if (!name) return -1;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  char norm[16];
  if (namec>=sizeof(norm)) return -1;
  int i=namec; while (i-->0) if ((name[i]>='a')&&(name[i]<='z')) norm[i]=name[i]-0x20; else norm[i]=name[i];
  struct assist_namefile *namefile=assist_namefile_get(ASSIST_NAMEFILE_SPRITE);
  if (!namefile) return -1;
  return assist_id_by_cat_name(namefile,ASSIST_CAT_PHYSICS,norm,namec);
}

int assist_get_xform_by_name(const char *name,int namec) {
  if (!name) return -1;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  char norm[16];
  if (namec>=sizeof(norm)) return -1;
  int i=namec; while (i-->0) if ((name[i]>='a')&&(name[i]<='z')) norm[i]=name[i]-0x20; else norm[i]=name[i];
  struct assist_namefile *namefile=assist_namefile_get(ASSIST_NAMEFILE_PLATFORM);
  if (!namefile) return -1;
  return assist_id_by_cat_name(namefile,ASSIST_CAT_XFORM,norm,namec);
}

int assist_get_item_by_name(const char *name,int namec) {
  if (!name) return -1;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  char norm[16];
  if (namec>=sizeof(norm)) return -1;
  int i=namec; while (i-->0) if ((name[i]>='a')&&(name[i]<='z')) norm[i]=name[i]-0x20; else norm[i]=name[i];
  struct assist_namefile *namefile=assist_namefile_get(ASSIST_NAMEFILE_PLATFORM);
  if (!namefile) return -1;
  return assist_id_by_cat_name(namefile,ASSIST_CAT_ITEM,norm,namec);
}

int assist_get_map_event_by_name(const char *name,int namec) {
  if (!name) return -1;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  char norm[16];
  if (namec>=sizeof(norm)) return -1;
  int i=namec; while (i-->0) if ((name[i]>='a')&&(name[i]<='z')) norm[i]=name[i]-0x20; else norm[i]=name[i];
  struct assist_namefile *namefile=assist_namefile_get(ASSIST_NAMEFILE_PLATFORM);
  if (!namefile) return -1;
  return assist_id_by_cat_name(namefile,ASSIST_CAT_MAP_EVID,norm,namec);
}

int assist_get_map_callback_by_name(const char *name,int namec) {
  if (!name) return -1;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  struct assist_namefile *namefile=assist_namefile_get(ASSIST_NAMEFILE_GAME);
  if (!namefile) return -1;
  return assist_id_by_cat_name(namefile,ASSIST_CAT_MAP_CALLBACK,name,namec);
}

int assist_get_resource_id_by_name(const char *tname,const char *rname,int rnamec) {
  struct assist_restype *restype=assist_restype_get(tname);
  if (!restype) return -1;
  return assist_restype_id_by_name(restype,rname,rnamec);
}

int assist_get_resource_name_by_id(void *dstpp,const char *tname,int id) {
  struct assist_restype *restype=assist_restype_get(tname);
  if (!restype) return 0;
  int p=assist_res_search(restype,id,0);
  if (p<0) p=-p-1;
  if ((p<restype->resc)&&(restype->resv[p].id==id)) {
    *(void**)dstpp=restype->resv[p].name;
    return restype->resv[p].namec;
  }
  return 0;
}

int assist_get_gsbit_by_name(const char *name,int namec) {
  if (!name) return -1;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  if (assist_gsbit_require()<0) return -1;
  return assist_gsbit_id_by_name(name,namec);
}

/* Instrument names actually don't exist.
 * We could hard-code the General MIDI names, but that's not much help because we're not using them.
 */
 
int assist_get_instrument_name(void *dstpp,int id) {
  return 0;
}

int assist_get_spell_id_by_name(const char *name,int namec) {
  if (!name) return -1;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  struct assist_namefile *namefile=assist_namefile_get(ASSIST_NAMEFILE_PLATFORM);
  if (!namefile) return -1;
  return assist_id_by_cat_name(namefile,ASSIST_CAT_SPELLID,name,namec);
}

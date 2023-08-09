#include "datan_internal.h"

/* Where qualifiers are in play, assert that each resource exists for each qualifier.
 * Warnings only, no errors.
 * eg a string is present in English but we forgot the Finnish translation, our work is unfinnished.
 */
 
struct qualifier_list {
  uint16_t v[256];
  uint8_t c;
  uint16_t type;
};

struct id_list {
  uint32_t *v;
  int c,a;
  uint16_t type;
  uint16_t qualifier;
};

static void id_list_cleanup(struct id_list *idlist) {
  if (idlist->v) free(idlist->v);
}

static int datan_qualifier_cb(uint16_t qualifier,void *userdata) {
  struct qualifier_list *qlist=userdata;
  if (qlist->c==0xff) {
    fprintf(stderr,"%s: Too many qualifiers for type %s, >255.\n",datan.arpath,fmn_restype_repr(qlist->type));
    return -2;
  }
  qlist->v[qlist->c++]=qualifier;
  return 0;
}

static int datan_id_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct id_list *idlist=userdata;
  if (idlist->c>=idlist->a) {
    int na=idlist->a+128;
    if (na>INT_MAX/sizeof(uint32_t)) return -1;
    void *nv=realloc(idlist->v,sizeof(uint32_t)*na);
    if (!nv) return -1;
    idlist->v=nv;
    idlist->a=na;
  }
  idlist->v[idlist->c++]=id;
  return 0;
}

static int id_list_cmp(const struct id_list *a,const struct id_list *b) {
  int i=0;
  for (;;i++) {
    if (i>=a->c) {
      if (i>=b->c) break;
      fprintf(stderr,"%s: %s:%d present for qualifier %d but missing for qualifier %d\n",datan.arpath,fmn_restype_repr(a->type),b->v[i],b->qualifier,a->qualifier);
      return -2;
    } else if (i>=b->c) {
      fprintf(stderr,"%s: %s:%d present for qualifier %d but missing for qualifier %d\n",datan.arpath,fmn_restype_repr(a->type),a->v[i],a->qualifier,b->qualifier);
      return -2;
    }
    uint32_t aid=a->v[i];
    uint32_t bid=b->v[i];
    if (aid<bid) {
      fprintf(stderr,"%s: %s:%d present for qualifier %d but missing for qualifier %d\n",datan.arpath,fmn_restype_repr(a->type),aid,a->qualifier,b->qualifier);
      return -2;
    } else if (aid>bid) {
      fprintf(stderr,"%s: %s:%d present for qualifier %d but missing for qualifier %d\n",datan.arpath,fmn_restype_repr(a->type),bid,b->qualifier,a->qualifier);
      return -2;
    }
  }
  return 0;
}
 
static int datan_validate_cross_qualifier_type(uint16_t type) {
  struct qualifier_list qlist={.type=type};
  int err=fmn_datafile_for_each_qualifier(datan.datafile,type,datan_qualifier_cb,&qlist);
  if (err<0) return err;
  struct id_list idlist={.type=type};
  struct id_list pvlist={0};
  int qi=0; for (;qi<qlist.c;qi++) {
    idlist.qualifier=qlist.v[qi];
    if ((err=fmn_datafile_for_each_of_qualified_type(datan.datafile,type,idlist.qualifier,datan_id_cb,&idlist))<0) {
      id_list_cleanup(&idlist);
      id_list_cleanup(&pvlist);
      return err;
    }
    if (pvlist.c) {
      if ((err=id_list_cmp(&pvlist,&idlist))<0) {
        id_list_cleanup(&idlist);
        id_list_cleanup(&pvlist);
        return err;
      }
    }
    id_list_cleanup(&pvlist);
    pvlist=idlist;
    memset(&idlist,0,sizeof(struct id_list));
  }
  id_list_cleanup(&idlist);
  id_list_cleanup(&pvlist);
  return 0;
}
 
int datan_validate_cross_qualifier() {
  int err;
  #define _(tag) if ((err=datan_validate_cross_qualifier_type(FMN_RESTYPE_##tag))<0) return err;
  FMN_FOR_EACH_RESTYPE
  #undef _
  return 0;
}

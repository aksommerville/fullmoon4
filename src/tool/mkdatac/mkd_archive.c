#include "mkd_internal.h"
#include "mkd_ar.h"
#include "mkd_instrument_format.h"

/* Helper for storing and applying type/qualifier filters.
 */
 
struct mkd_qfilter {
  struct mkd_qfilter_rule {
    int type,qualifier;
  } *rulev;
  int rulec,rulea;
};

static void mkd_qfilter_cleanup(struct mkd_qfilter *qfilter) {
  if (qfilter->rulev) free(qfilter->rulev);
}

static int mkd_qfilter_search(const struct mkd_qfilter *qfilter,int type,int qualifier) {
  int lo=0,hi=qfilter->rulec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct mkd_qfilter_rule *q=qfilter->rulev+ck;
         if (type<q->type) hi=ck;
    else if (type>q->type) lo=ck+1;
    else if (qualifier<q->qualifier) hi=ck;
    else if (qualifier>q->qualifier) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

// Take input like "instrument:WebAudio" or "1:123" and add a rule.
static int mkd_qfilter_add_string(struct mkd_qfilter *qfilter,const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int sepp=-1,i=0;
  for (;i<srcc;i++) if (src[i]==':') { sepp=i; break; }
  if (sepp<0) return -1;
  int type,qualifier;
  if ((type=mkd_restype_eval(src,sepp))<1) return -1;
  if ((qualifier=mkd_qualifier_eval(type,src+sepp+1,srcc-sepp-1))<0) return -1;
  int p=mkd_qfilter_search(qfilter,type,qualifier);
  if (p>=0) return 0;
  p=-p-1;
  if (qfilter->rulec>=qfilter->rulea) {
    int na=qfilter->rulea+8;
    if (na>INT_MAX/sizeof(struct mkd_qfilter_rule)) return -1;
    void *nv=realloc(qfilter->rulev,sizeof(struct mkd_qfilter_rule)*na);
    if (!nv) return -1;
    qfilter->rulev=nv;
    qfilter->rulea=na;
  }
  struct mkd_qfilter_rule *rule=qfilter->rulev+p;
  memmove(rule+1,rule,sizeof(struct mkd_qfilter_rule)*(qfilter->rulec-p));
  memset(rule,0,sizeof(struct mkd_qfilter_rule));
  qfilter->rulec++;
  rule->type=type;
  rule->qualifier=qualifier;
  return 0;
}

static int mkd_qfilter_test(const struct mkd_qfilter *qfilter,int type,int qualifier) {
  int p=mkd_qfilter_search(qfilter,type,qualifier);
  if (p>=0) return 1; // This combo is explicitly named. YES.
  p=-p-1;
  // This type was named with any other qualifier: NO.
  if ((p>0)&&(qfilter->rulev[p-1].type==type)) return 0;
  if ((p<qfilter->rulec)&&(qfilter->rulev[p].type==type)) return 0;
  // Never heard of this type: YES
  return 1;
}

/* Apply filter to one resource.
 */
 
static int mkd_archive_filter_cb(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata) {
  struct mkd_qfilter *qfilter=userdata;
  return mkd_qfilter_test(qfilter,type,qualifier);
}

/* Digest, after adding all input files.
 */
 
static int mkd_archive_digest(struct mkd_ar *ar) {
  
  // Apply qfilter if present.
  struct mkd_qfilter qfilter={0};
  const struct mkd_arg *arg=mkd.config.argv;
  int i=mkd.config.argc;
  for (;i-->0;arg++) {
    if (arg->kc!=7) continue;
    if (memcmp(arg->k,"qfilter",7)) continue;
    if (mkd_qfilter_add_string(&qfilter,arg->v,arg->vc)<0) {
      fprintf(stderr,"%s: Failed to evaluate filter rule '%.*s'\n",mkd.config.exename,arg->vc,arg->v);
      mkd_qfilter_cleanup(&qfilter);
      fprintf(stderr,"%s:%d\n",__FILE__,__LINE__);
      return -2;
    }
  }
  if (qfilter.rulec) {
    if (mkd_ar_filter(ar,mkd_archive_filter_cb,&qfilter)<0) {
      mkd_qfilter_cleanup(&qfilter);
      return -1;
    }
  }
  mkd_qfilter_cleanup(&qfilter);
  
  return 0;
}

/* Add multiple INSTRUMENT and SOUND resources from one input file.
 */
 
static int mkd_archive_add_instruments(struct mkd_ar *ar,struct mkd_respath *respath,const void *serial,int serialc) {

  const struct mkd_instrument_format *format=0;
  switch (respath->resq) {
    case 1: format=&mkd_instrument_format_WebAudio; break;
    case 2: format=&mkd_instrument_format_minsyn; break;
    case 3: format=&mkd_instrument_format_stdsyn; break;
    default: {
        fprintf(stderr,"%s: %s:%d: No defined instrument format for qualifier %d.\n",respath->path,__FILE__,__LINE__,respath->resq);
        return -2;
      }
  }

  struct sr_decoder decoder={.v=serial,.c=serialc};
  const char *line=0;
  int linec=0,lineno=1;
  int type=0,id=0;
  void *obj=0;
  for (;linec=sr_decode_line(&line,&decoder);lineno++) {
    int i=0; for (;i<linec;i++) if (line[i]=='#') linec=i;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    const char *kw=line;
    int kwc=0;
    while (linec&&((unsigned char)line[0]>0x20)) { line++; linec--; kwc++; }
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    
    if ((kwc==3)&&!memcmp(kw,"end",3)) {
      if (!obj) {
        fprintf(stderr,"%s:%d: Unexpected 'end'\n",respath->path,lineno);
        return -2;
      }
      switch (type) {
        case FMN_RESTYPE_INSTRUMENT: {
            int err=0;
            struct sr_encoder encoder={0};
            if (format->instrument_encode) err=format->instrument_encode(&encoder,obj);
            if (format->instrument_del) format->instrument_del(obj);
            obj=0;
            if (err<0) {
              sr_encoder_cleanup(&encoder);
              fprintf(stderr,"%s:%d: Failed to encode instrument\n",respath->path,lineno);
              return -2;
            }
            if (mkd_ar_add_handoff(ar,type,respath->resq,id,respath->path,encoder.v,encoder.c)<0) {
              sr_encoder_cleanup(&encoder);
              return -1;
            }
            // encoder.v handed off
            type=0;
            id=0;
          } break;
        case FMN_RESTYPE_SOUND: {
            int err=0;
            struct sr_encoder encoder={0};
            if (format->sound_encode) err=format->sound_encode(&encoder,obj);
            if (format->sound_del) format->sound_del(obj);
            obj=0;
            if (err<0) {
              sr_encoder_cleanup(&encoder);
              fprintf(stderr,"%s:%d: Failed to encode sound\n",respath->path,lineno);
              return -2;
            }
            if (mkd_ar_add_handoff(ar,type,respath->resq,id,respath->path,encoder.v,encoder.c)<0) {
              sr_encoder_cleanup(&encoder);
              return -1;
            }
            // encoder.v handed off
            type=0;
            id=0;
          } break;
      }
      continue;
    }
    
    switch (type) {
    
      case 0: { // "begin ID" or "sound ID" only.
          if ((kwc==5)&&!memcmp(kw,"begin",5)) {
            if (sr_int_eval(&id,line,linec)<2) {
              fprintf(stderr,"%s:%d: Expected ID after 'begin'\n",respath->path,lineno);
              return -2;
            }
            if (!format->instrument_new||!(obj=format->instrument_new(id,respath->path,lineno))) {
              fprintf(stderr,"%s:%d: Failed to create %s instrument %d\n",respath->path,lineno,format->name,id);
              return -2;
            }
            type=FMN_RESTYPE_INSTRUMENT;
          } else if ((kwc==5)&&!memcmp(kw,"sound",5)) {
            if (sr_int_eval(&id,line,linec)>=2) ;
            else if ((id=assist_get_sound_id_by_name(line,linec))>0) ;
            else {
              fprintf(stderr,"%s:%d: Expected ID after 'sound'\n",respath->path,lineno);
              return -2;
            }
            if (!format->sound_new||!(obj=format->sound_new(id,respath->path,lineno))) {
              fprintf(stderr,"%s:%d: Failed to create %s sound %d\n",respath->path,lineno,format->name,id);
              return -2;
            }
            type=FMN_RESTYPE_SOUND;
          } else {
            fprintf(stderr,"%s:%d: Expected 'begin ID' or 'sound ID'\n",respath->path,lineno);
            return -2;
          }
        } break;
        
      case FMN_RESTYPE_INSTRUMENT: {
          if (!format->instrument_line||(format->instrument_line(obj,kw,kwc,line,linec,respath->path,lineno)<0)) {
            if (format->instrument_del) format->instrument_del(obj);
            fprintf(stderr,"%s:%d: Error processing instrument\n",respath->path,lineno);
            return -2;
          }
        } break;
        
      case FMN_RESTYPE_SOUND: {
          if (!format->sound_line||(format->sound_line(obj,kw,kwc,line,linec,respath->path,lineno)<0)) {
            if (format->sound_del) format->sound_del(obj);
            fprintf(stderr,"%s:%d: Error processing sound\n",respath->path,lineno);
            return -2;
          }
        } break;
    }
  }
  if (obj) {
    fprintf(stderr,"%s:%d: Block not closed\n",respath->path,lineno);
    switch (type) {
      case FMN_RESTYPE_INSTRUMENT: if (format->instrument_del) format->instrument_del(obj); break;
      case FMN_RESTYPE_SOUND: if (format->sound_del) format->sound_del(obj); break;
    }
    return -2;
  }
  return 0;
}

/* Add multiple STRING resources from one input file.
 */
 
static int mkd_archive_add_strings(struct mkd_ar *ar,struct mkd_respath *respath,const void *serial,int serialc) {
  struct sr_decoder decoder={.v=serial,.c=serialc};
  const char *line=0;
  int linec=0,lineno=1;
  for (;linec=sr_decode_line(&line,&decoder);lineno++) {
    int id=0,linep=0;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    if (linep>=linec) continue;
    if (line[linep]=='#') continue;
    while ((linep<linec)&&((unsigned char)line[linep]>0x20)) {
      if ((line[linep]<'0')||(line[linep]>'9')) { id=0; break; }
      id*=10;
      id+=line[linep++]-'0';
    }
    if (id<1) {
      fprintf(stderr,"%s:%d: Failed to evaluate string id.\n",respath->path,lineno);
      return -2;
    }
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    if (mkd_ar_add_copy(ar,FMN_RESTYPE_STRING,respath->resq,id,respath->path,line+linep,linec-linep)<0) return -1;
  }
  return 0;
}

/* Add one input file.
 */
 
static int mkd_archive_add_file(struct mkd_ar *ar,const char *path) {
  struct mkd_respath respath={0};
  if (mkd_respath_eval(&respath,path)<0) return 0;
  if (respath.restype==FMN_RESTYPE_KNOWN_UNKNOWN) return 0;
  if (!respath.restype) {
    fprintf(stderr,"%s: Failed to determine resource type.\n",path);
    return -2;
  }
  void *serial=0;
  int serialc=fmn_file_read(&serial,path);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",path);
    return -2;
  }
  
  // Call out for the special types that produce multiple resources per input file.
  switch (respath.restype) {
    case FMN_RESTYPE_SOUND: // SOUND shouldn't exist, but if so, assume it's the same as INSTRUMENT.
    case FMN_RESTYPE_INSTRUMENT: {
        int err=mkd_archive_add_instruments(ar,&respath,serial,serialc);
        free(serial);
        return err;
      }
    case FMN_RESTYPE_STRING: {
        int err=mkd_archive_add_strings(ar,&respath,serial,serialc);
        free(serial);
        return err;
      }
  }
  
  // Everything else is expected to have a valid ID.
  if ((respath.resid<1)||(respath.resid>0xffff)) {
    free(serial);
    fprintf(stderr,"%s: Resource ID in 1..65535 required.\n",path);
    return -2;
  }
  
  if (mkd_ar_add_handoff(ar,respath.restype,respath.resq,respath.resid,path,serial,serialc)<0) {
    free(serial);
    return -1;
  }
  return 0;
}

/* Archive packing, main entry point.
 */
 
int mkd_main_archive() {
  struct mkd_ar *ar=mkd_ar_new();
  if (!ar) return -1;
  int i=0,err;
  for (;i<mkd.config.srcpathc;i++) {
    if ((err=mkd_archive_add_file(ar,mkd.config.srcpathv[i]))<0) {
      mkd_ar_del(ar);
      return err;
    }
  }
  if ((err=mkd_archive_digest(ar))<0) {
    mkd_ar_del(ar);
    return err;
  }
  mkd.dst.c=0;
  err=mkd_ar_encode(&mkd.dst,ar);
  mkd_ar_del(ar);
  if (err<0) return err;
  return mkd_write_single_output();
}

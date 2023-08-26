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

/* Compile one string.
 * Decode UTF-8, then encode to our private code page.
 */
 
static int mkd_string_compile(char *dst,int dsta,const char *src,int srcc) {
  int srcp=0,dstc=0;
  while (srcp<srcc) {
    int err,ch;
    if ((err=sr_utf8_decode(&ch,src+srcp,srcc-srcp))<1) return -1;
    srcp+=err;
    
    // The first four rows of ASCII are verbatim. (digits, uppercase letters, most punctuation).
    if ((ch>=0x20)&&(ch<0x60)) {
    
    // Lowercase ASCII, make it uppercase.
    } else if ((ch>=0x51)&&(ch<=0x7a)) {
      ch-=0x20;
      
    // Keep 0x7e Tilde verbatim -- we use it for spacing in the credits. (the glyph is blank).
    } else if (ch==0x7e) {
      
    // Unsupported ASCII: Grave, curlies, pipe.
    
    // Uppercase Cyrillic, rows 6 and 7.
    } else if ((ch>=0x410)&&(ch<0x430)) {
      ch=ch+0x60-0x410;
      
    // Lowercase Cyrillic, force to uppercase.
    } else if ((ch>=0x430)&&(ch<0x450)) {
      ch=ch+0x60-0x430;
      
    // Bunch of loose letters not in Unicode order. Mostly accented Roman letters.
    } else switch (ch) {
      
      case 0xcb: case 0xeb: ch=0x01; break; // E-diaresis
      case 0xc8: case 0xe8: ch=0x02; break; // E-grave
      case 0xc9: case 0xe9: ch=0x03; break; // E-acute
      case 0xca: case 0xea: ch=0x04; break; // E-circonflex
      case 0xc0: case 0xe0: ch=0x05; break; // A-grave
      case 0xc1: case 0xe1: ch=0x06; break; // A-acute
      case 0xc2: case 0xe2: ch=0x07; break; // A-circonflex
      case 0xc3: case 0xe3: ch=0x08; break; // A-tilde
      case 0xc4: case 0xe4: ch=0x09; break; // A-diaresis
      case 0xc5: case 0xe5: ch=0x0a; break; // A-halo
      case 0xc7: case 0xe7: ch=0x0b; break; // C-cedilla
      case 0xcc: case 0xec: ch=0x0c; break; // I-grave
      case 0xcd: case 0xed: ch=0x0d; break; // I-acute
      case 0xce: case 0xee: ch=0x0e; break; // I-circonflex
      case 0xcf: case 0xef: ch=0x0f; break; // I-diaresis
      case 0xd2: case 0xf2: ch=0x10; break; // O-grave
      case 0xd3: case 0xf3: ch=0x11; break; // O-acute
      case 0xd4: case 0xf4: ch=0x12; break; // O-circonflex
      case 0xd5: case 0xf5: ch=0x13; break; // O-tilde
      case 0xd6: case 0xf6: ch=0x14; break; // O-diaresis
      case 0xd8: case 0xf8: ch=0x15; break; // O-stroke
      case 0xd9: case 0xf9: ch=0x16; break; // U-grave
      case 0xda: case 0xfa: ch=0x17; break; // U-acute
      case 0xdb: case 0xfb: ch=0x18; break; // U-circonflex
      case 0xdc: case 0xfc: ch=0x19; break; // U-diaresis
      case 0xdd: case 0xfd: ch=0x1a; break; // Y-acute
      case 0xff: ch=0x1b; break; // Y-diaresis (only lowercase in Latin-1. Upper probly exists somewhere in Unicode but whatever, we're not using it)
      case 0xd0: case 0xf0: ch=0x1c; break; // Eth
      case 0xde: case 0xfe: ch=0x1d; break; // Thorn
      case 0xdf: ch=0x1e; break; // Esszett (lowercase only)
      
      case 0x401: case 0x451: ch=0x01; break; // Cyrillic Yo, same as E-diaresis
    
      // Anything else, log it and fail.
      default: {
          fprintf(stderr,"Code point U+%x not supported (at %d/%d in string).\n",ch,srcp,srcc);
          return -1; // not -2: Let our caller log the path and line.
        }
    }
    
    if (dstc<dsta) dst[dstc]=ch;
    dstc++;
  }
  return dstc;
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
    char tmp[128];
    int tmpc=mkd_string_compile(tmp,sizeof(tmp),line+linep,linec-linep);
    if ((tmpc<0)||(tmpc>sizeof(tmp))) {
      if (tmpc>sizeof(tmp)) fprintf(stderr,"%s:%d: String too long (%d, limit %d)\n",respath->path,lineno,tmpc,(int)sizeof(tmp));
      else if (tmpc!=-2) fprintf(stderr,"%s:%d: Error reencoding string.\n",respath->path,lineno);
      return -2;
    }
    if (mkd_ar_add_copy(ar,FMN_RESTYPE_STRING,respath->resq,id,respath->path,tmp,tmpc)<0) return -1;
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

static int mkd_archive_add_directory(struct mkd_ar *ar,const char *path);

static int mkd_archive_add_directory_1(const char *path,const char *base,char ftype,void *userdata) {
  if (!ftype) ftype=fmn_file_get_type(path);
  if (ftype=='d') return mkd_archive_add_directory(userdata,path);
  return mkd_archive_add_file(userdata,path);
}

static int mkd_archive_add_directory(struct mkd_ar *ar,const char *path) {
  return fmn_dir_read(path,mkd_archive_add_directory_1,ar);
}

/* Archive packing, main entry point.
 */
 
int mkd_main_archive() {
  struct mkd_ar *ar=mkd_ar_new();
  if (!ar) return -1;
  int i=0,err;
  for (;i<mkd.config.srcpathc;i++) {
    if (fmn_file_get_type(mkd.config.srcpathv[i])=='d') {
      if ((err=mkd_archive_add_directory(ar,mkd.config.srcpathv[i]))<0) {
        mkd_ar_del(ar);
        return err;
      }
    } else {
      if ((err=mkd_archive_add_file(ar,mkd.config.srcpathv[i]))<0) {
        mkd_ar_del(ar);
        return err;
      }
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

#include "fiddle_internal.h"
#include "opt/datafile/fmn_datafile.h"
#include "opt/midi/midi.h"

int assist_get_sound_name(void *dstpp,int id);
int assist_get_instrument_name(void *dstpp,int id);
int assist_get_resource_name_by_id(void *dstpp,const char *tname,int id);

/* Read query parameter "qualifier" and evaluate.
 */
 
static int fiddle_qualifier_from_query(const struct http_xfer *xfer) {
  char src[32]={0};
  int srcc=http_xfer_get_query_string(src,sizeof(src),xfer,"qualifier",9);
  if ((srcc==8)&&!memcmp(src,"WebAudio",8)) return 1;
  if ((srcc==6)&&!memcmp(src,"minsyn",6)) return 2;
  if ((srcc==6)&&!memcmp(src,"stdsyn",6)) return 3;
  return 0;
}

/* GET /api/status => {synth,song,sound,instrument}
 */
 
int fiddle_httpcb_get_status(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  struct sr_encoder dst={0};
  sr_encode_json_object_start(&dst,0,0);
  
  if (fiddle.synth) {
    sr_encode_json_string(&dst,"synth",5,fiddle.synth->type->name,-1);
  } else {
    sr_encode_json_null(&dst,"synth",5);
  }
  if (fiddle.synth&&fiddle.synth->type->get_instrument_by_channel) {
    sr_encode_json_int(&dst,"instrument",10,fiddle.synth->type->get_instrument_by_channel(fiddle.synth,14));
  } else {
    sr_encode_json_int(&dst,"instrument",10,0);
  }
  sr_encode_json_int(&dst,"song",4,fiddle.songid);
  sr_encode_json_int(&dst,"sound",5,fiddle.latest_soundid);
  
  sr_encode_json_object_end(&dst,0);
  http_xfer_set_body(rsp,dst.v,dst.c);
  http_xfer_set_header(rsp,"Content-Type",12,"application/json",-1);
  sr_encoder_cleanup(&dst);
  return http_xfer_set_status(rsp,200,"OK");
}

/* GET /api/synths => [qualifier,...]
 */
 
struct fiddle_httpcb_get_synths_context {
  struct sr_encoder encoder;
};

static int fiddle_httpcb_get_synths_cb(uint16_t qualifier,void *userdata) {
  struct fiddle_httpcb_get_synths_context *ctx=userdata;
  switch (qualifier) {
    case 1: sr_encode_json_string(&ctx->encoder,0,0,"WebAudio",8); break;
    case 2: sr_encode_json_string(&ctx->encoder,0,0,"minsyn",6); break;
    case 3: sr_encode_json_string(&ctx->encoder,0,0,"stdsyn",6); break;
    default: sr_encode_json_int(&ctx->encoder,0,0,qualifier); break;
  }
  return 0;
}
 
int fiddle_httpcb_get_synths(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  struct fiddle_httpcb_get_synths_context ctx={0};
  sr_encode_json_array_start(&ctx.encoder,0,0);
  fmn_datafile_for_each_qualifier(fiddle.datafile,FMN_RESTYPE_INSTRUMENT,fiddle_httpcb_get_synths_cb,&ctx);
  sr_encode_json_array_end(&ctx.encoder,0);
  http_xfer_set_body(rsp,ctx.encoder.v,ctx.encoder.c);
  sr_encoder_cleanup(&ctx.encoder);
  http_xfer_set_header(rsp,"Content-Type",12,"application/json",-1);
  return http_xfer_set_status(rsp,200,"OK");
}

/* GET /api/sounds?qualifier => [{id,name},...]
 */
 
struct fiddle_httpcb_get_sounds_context {
  struct sr_encoder encoder;
};

static int fiddle_httpcb_get_sounds_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct fiddle_httpcb_get_sounds_context *ctx=userdata;
  const char *name=0;
  int namec=assist_get_sound_name(&name,id);
  if (namec<0) namec=0;
  int jsonctx=sr_encode_json_object_start(&ctx->encoder,0,0);
  sr_encode_json_string(&ctx->encoder,"name",4,name,namec);
  sr_encode_json_int(&ctx->encoder,"id",2,id);
  return sr_encode_json_object_end(&ctx->encoder,jsonctx);
}
 
int fiddle_httpcb_get_sounds(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  struct fiddle_httpcb_get_sounds_context ctx={0};
  sr_encode_json_array_start(&ctx.encoder,0,0);
  int qualifier=fiddle_qualifier_from_query(req);
  fmn_datafile_for_each_of_qualified_type(
    fiddle.datafile,FMN_RESTYPE_SOUND,qualifier,
    fiddle_httpcb_get_sounds_cb,&ctx
  );
  sr_encode_json_array_end(&ctx.encoder,0);
  http_xfer_set_body(rsp,ctx.encoder.v,ctx.encoder.c);
  sr_encoder_cleanup(&ctx.encoder);
  http_xfer_set_header(rsp,"Content-Type",12,"application/json",-1);
  return http_xfer_set_status(rsp,200,"OK");
}

/* GET /api/instruments?qualifier => [{id,name},...]
 */
 
struct fiddle_httpcb_get_instruments_context {
  struct sr_encoder encoder;
};

static int fiddle_httpcb_get_instruments_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct fiddle_httpcb_get_instruments_context *ctx=userdata;
  const char *name=0;
  int namec=assist_get_instrument_name(&name,id);
  if (namec<0) namec=0;
  int jsonctx=sr_encode_json_object_start(&ctx->encoder,0,0);
  sr_encode_json_string(&ctx->encoder,"name",4,name,namec);
  sr_encode_json_int(&ctx->encoder,"id",2,id);
  return sr_encode_json_object_end(&ctx->encoder,jsonctx);
}
 
int fiddle_httpcb_get_instruments(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  struct fiddle_httpcb_get_instruments_context ctx={0};
  sr_encode_json_array_start(&ctx.encoder,0,0);
  int qualifier=fiddle_qualifier_from_query(req);
  fmn_datafile_for_each_of_qualified_type(
    fiddle.datafile,FMN_RESTYPE_INSTRUMENT,qualifier,
    fiddle_httpcb_get_instruments_cb,&ctx
  );
  sr_encode_json_array_end(&ctx.encoder,0);
  http_xfer_set_body(rsp,ctx.encoder.v,ctx.encoder.c);
  sr_encoder_cleanup(&ctx.encoder);
  http_xfer_set_header(rsp,"Content-Type",12,"application/json",-1);
  return http_xfer_set_status(rsp,200,"OK");
}

/* GET /api/songs => [{id,name},...]
 */
 
struct fiddle_httpcb_get_songs_context {
  struct sr_encoder encoder;
};

static int fiddle_httpcb_get_songs_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct fiddle_httpcb_get_songs_context *ctx=userdata;
  const char *name=0;
  int namec=assist_get_resource_name_by_id(&name,"song",id);
  if (namec<0) namec=0;
  int jsonctx=sr_encode_json_object_start(&ctx->encoder,0,0);
  sr_encode_json_string(&ctx->encoder,"name",4,name,namec);
  sr_encode_json_int(&ctx->encoder,"id",2,id);
  if (sr_encode_json_object_end(&ctx->encoder,jsonctx)<0) return -1;
  return 0;
}
 
int fiddle_httpcb_get_songs(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  struct fiddle_httpcb_get_songs_context ctx={0};
  sr_encode_json_array_start(&ctx.encoder,0,0);
  fmn_datafile_for_each_of_type(
    fiddle.datafile,FMN_RESTYPE_SONG,
    fiddle_httpcb_get_songs_cb,&ctx
  );
  sr_encode_json_array_end(&ctx.encoder,0);
  http_xfer_set_body(rsp,ctx.encoder.v,ctx.encoder.c);
  sr_encoder_cleanup(&ctx.encoder);
  http_xfer_set_header(rsp,"Content-Type",12,"application/json",-1);
  return http_xfer_set_status(rsp,200,"OK");
}

/* GET /api/insusage
 */
 
struct fiddle_httpcb_insusage_context {
  struct sr_encoder encoder;
  uint8_t *insv,*sndv;
  int insc,insa,sndc,snda;
};

static void fiddle_httpcb_insusage_context_cleanup(struct fiddle_httpcb_insusage_context *ctx) {
  sr_encoder_cleanup(&ctx->encoder);
  if (ctx->insv) free(ctx->insv);
  if (ctx->sndv) free(ctx->sndv);
}

static int fiddle_httpcb_insusage_search(const uint8_t *v,int c,uint8_t id) {
  int lo=0,hi=c;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (id<v[ck]) hi=ck;
    else if (id>v[ck]) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int fiddle_httpcb_insusage_insert(uint8_t **v,int *c,int *a,int p,uint8_t id) {
  if ((p<0)||(p>*c)) return -1;
  if (*c>=*a) {
    int na=(*a)+16;
    void *nv=realloc(*v,na);
    if (!nv) return -1;
    *v=nv;
    *a=na;
  }
  memmove((*v)+p+1,(*v)+p,(*c)-p);
  (*c)++;
  (*v)[p]=id;
  return 0;
}

static void fiddle_httpcb_insusage_add_ins(struct fiddle_httpcb_insusage_context *ctx,uint8_t ins) {
  int p=fiddle_httpcb_insusage_search(ctx->insv,ctx->insc,ins);
  if (p>=0) return;
  fiddle_httpcb_insusage_insert(&ctx->insv,&ctx->insc,&ctx->insa,-p-1,ins);
}

static void fiddle_httpcb_insusage_add_snd(struct fiddle_httpcb_insusage_context *ctx,uint8_t snd) {
  int p=fiddle_httpcb_insusage_search(ctx->sndv,ctx->sndc,snd);
  if (p>=0) return;
  fiddle_httpcb_insusage_insert(&ctx->sndv,&ctx->sndc,&ctx->snda,-p-1,snd);
}

static int fiddle_httpcb_insusage_1(struct fiddle_httpcb_insusage_context *ctx,const uint8_t *src,int srcc) {
  int jsonctx,i;
  ctx->insc=0;
  ctx->sndc=0;
  
  struct midi_file *file=midi_file_new_borrow(src,srcc);
  if (!file) return -1;
  while (1) {
    struct midi_event event={0};
    int framec=midi_file_next(&event,file,0);
    if (framec<0) {
      break;
    } else if (framec>0) {
      midi_file_advance(file,framec);
    } else {
      switch (event.opcode) {
        case MIDI_OPCODE_PROGRAM: fiddle_httpcb_insusage_add_ins(ctx,event.a); break;
        case MIDI_OPCODE_NOTE_ON: if (event.chid==0x0f) fiddle_httpcb_insusage_add_snd(ctx,event.a); break;
      }
    }
  }
  midi_file_del(file);
  
  if ((jsonctx=sr_encode_json_array_start(&ctx->encoder,"instruments",11))<0) return -1;
  for (i=0;i<ctx->insc;i++) sr_encode_json_int(&ctx->encoder,0,0,ctx->insv[i]);
  sr_encode_json_array_end(&ctx->encoder,jsonctx);
  if ((jsonctx=sr_encode_json_array_start(&ctx->encoder,"sounds",6))<0) return -1;
  for (i=0;i<ctx->sndc;i++) sr_encode_json_int(&ctx->encoder,0,0,ctx->sndv[i]);
  sr_encode_json_array_end(&ctx->encoder,jsonctx);
  return 0;
}

static int fiddle_httpcb_get_insusage_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct fiddle_httpcb_insusage_context *ctx=userdata;
  int jsonctx=sr_encode_json_object_start(&ctx->encoder,0,0);
  sr_encode_json_int(&ctx->encoder,"id",2,id);
  if (fiddle_httpcb_insusage_1(ctx,v,c)<0) return -1;
  if (sr_encode_json_object_end(&ctx->encoder,jsonctx)<0) return -1;
  return 0;
}
 
int fiddle_httpcb_get_insusage(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  struct fiddle_httpcb_insusage_context ctx={0};
  sr_encode_json_array_start(&ctx.encoder,0,0);
  fmn_datafile_for_each_of_type(fiddle.datafile,FMN_RESTYPE_SONG,fiddle_httpcb_get_insusage_cb,&ctx);
  if (sr_encode_json_array_end(&ctx.encoder,0)<0) {
    fiddle_httpcb_insusage_context_cleanup(&ctx);
    return http_xfer_set_status(rsp,500,"Internal error");
  }
  http_xfer_set_body(rsp,ctx.encoder.v,ctx.encoder.c);
  http_xfer_set_header(rsp,"Content-Type",12,"application/json",-1);
  fiddle_httpcb_insusage_context_cleanup(&ctx);
  return http_xfer_set_status(rsp,200,"OK");
}

/* GET /api/datarate
 */
 
struct fiddle_datarate_context {
  struct sr_encoder encoder;
  int sndc,sndb,sndms;
  int songc,songb,songms;
};

static void fiddle_datarate_context_cleanup(struct fiddle_datarate_context *ctx) {
  sr_encoder_cleanup(&ctx->encoder);
}

static int fiddle_datarate_cb_song(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct fiddle_datarate_context *ctx=userdata;
  ctx->songc++;
  ctx->songb+=c;
  struct midi_file *midi=midi_file_new_borrow(v,c);
  if (midi) {
    midi_file_set_output_rate(midi,1000); // tick=ms
    while (1) {
      struct midi_event event={0};
      int err=midi_file_next(&event,midi,0);
      if (err<0) break;
      if (!err) continue;
      ctx->songms+=err;
      midi_file_advance(midi,err);
    }
    midi_file_del(midi);
  }
  return 0;
}

static int fiddle_datarate_cb_sound(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct fiddle_datarate_context *ctx=userdata;
  const uint8_t *src=v;
  ctx->sndc++;
  ctx->sndb+=c;
  switch (qualifier) {
    case 1: break; // TODO WebAudio
    case 2: { // minsyn: Data starts with duration s, u1.7
        if (c>=1) {
          ctx->sndms+=(src[0]*1000)>>7;
        }
      } break;
    case 3: break; // TODO stdsyn
  }
  return 0;
}
 
int fiddle_httpcb_get_datarate(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  struct fiddle_datarate_context ctx={0};
  
  fmn_datafile_for_each_of_type(fiddle.datafile,FMN_RESTYPE_SONG,fiddle_datarate_cb_song,&ctx);
  fmn_datafile_for_each_of_qualified_type(fiddle.datafile,FMN_RESTYPE_SOUND,fiddle.drivers_qualifier,fiddle_datarate_cb_sound,&ctx);
  
  sr_encode_json_object_start(&ctx.encoder,0,0);
  sr_encode_json_int(&ctx.encoder,"soundCount",10,ctx.sndc);
  sr_encode_json_int(&ctx.encoder,"soundSize",9,ctx.sndb);
  sr_encode_json_int(&ctx.encoder,"soundTime",9,ctx.sndms);
  sr_encode_json_int(&ctx.encoder,"songCount",9,ctx.songc);
  sr_encode_json_int(&ctx.encoder,"songSize",8,ctx.songb);
  sr_encode_json_int(&ctx.encoder,"songTime",8,ctx.songms);
  if (sr_encode_json_object_end(&ctx.encoder,0)<0) {
    fiddle_datarate_context_cleanup(&ctx);
    return http_xfer_set_status(rsp,500,"Internal error");
  }
  http_xfer_set_body(rsp,ctx.encoder.v,ctx.encoder.c);
  http_xfer_set_header(rsp,"Content-Type",12,"application/json",-1);
  fiddle_datarate_context_cleanup(&ctx);
  return http_xfer_set_status(rsp,200,"OK");
}

/* POST /api/synth/use?qualifier
 */
 
int fiddle_httpcb_post_synth_use(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  int qualifier=fiddle_qualifier_from_query(req);
  if (fiddle_drivers_set(qualifier)<0) {
    return http_xfer_set_status(rsp,500,"Failed to init driver %d",qualifier);
  }
  return http_xfer_set_status(rsp,200,"OK");
}

/* POST /api/sound/play?id
 */
 
int fiddle_httpcb_post_sound_play(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  if (!fiddle.synth) return http_xfer_set_status(rsp,400,"Synth disabled");
  int id=0;
  if (http_xfer_get_query_int(&id,req,"id",2)<0) return http_xfer_set_status(rsp,400,"Sound id required");
  if ((id<1)||(id>0x7f)) return http_xfer_set_status(rsp,500,"Sound id %d not in 1..127",id);
  fiddle.latest_soundid=id;
  if (fiddle.synth->type->event) {
    if (fiddle_drivers_lock()>=0) {
      fiddle.synth->type->event(fiddle.synth,0x0f,0x98,id,0x40);
      fiddle_drivers_unlock();
    }
  }
  return http_xfer_set_status(rsp,200,"OK");
}

/* POST /api/song/play?id # Stops playback if id invalid (eg zero)
 */
 
int fiddle_httpcb_post_song_play(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  if (!fiddle.synth) return http_xfer_set_status(rsp,400,"Synth disabled");
  int id=0;
  if (http_xfer_get_query_int(&id,req,"id",2)<0) return http_xfer_set_status(rsp,400,"Song id required");
  fiddle.songid=0;
  const void *serial=0;
  int serialc=fmn_datafile_get_any(&serial,fiddle.datafile,FMN_RESTYPE_SONG,id);
  if (serialc<0) serialc=0;
  if (fiddle.synth->type->play_song) {
    if (fiddle_drivers_lock()>=0) {
      fiddle.synth->type->play_song(fiddle.synth,serial,serialc,1,1);
      fiddle_drivers_unlock();
      fiddle.songid=id;
    }
  }
  return http_xfer_set_status(rsp,200,"OK");
}

/* POST /api/midi?chid&opcode&a&b
 */
 
int fiddle_httpcb_post_midi(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  if (!fiddle.synth) return http_xfer_set_status(rsp,400,"Synth disabled");
  int chid=0,opcode=0,a=0,b=0;
  http_xfer_get_query_int(&chid,req,"chid",4);
  http_xfer_get_query_int(&opcode,req,"opcode",6);
  http_xfer_get_query_int(&a,req,"a",1);
  http_xfer_get_query_int(&b,req,"b",1);
  if (
    (chid<0)||(chid>0xff)||
    (opcode<0)||(opcode>0xff)||
    (a<0)||(a>0xff)||
    (b<0)||(b>0xff)
  ) return http_xfer_set_status(rsp,400,"MIDI event out of range (%d,%d,%d,%d) limit 0..255",chid,opcode,a,b);
  if (fiddle.synth->type->event) {
    if (fiddle_drivers_lock()>=0) {
      fiddle.synth->type->event(fiddle.synth,chid,opcode,a,b);
      fiddle_drivers_unlock();
    }
  }
  return http_xfer_set_status(rsp,200,"OK");
}

/* GET /
 * When the browser refreshes, we serve index.html, but we also run make first.
 * The ideal would be that we detect changes to the data files, eagerly run make, and notify the client via WebSocket.
 * But I'm not there yet.
 */
 
static int fiddle_httpcb_root_cb(int status,const char *log,int logc,void *userdata) {
  struct http_xfer *rsp=userdata;
  
  if (status) {
    http_xfer_set_body(rsp,log,logc);
    http_xfer_set_header(rsp,"Content-Type",12,"text/plain",10);
    return http_xfer_set_status(rsp,500,"make failed");
  }
  
  fmn_datafile_reopen(fiddle.datafile);
  
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"%.*s/index.html",fiddle.htdocsc,fiddle.htdocs);
  if ((pathc<1)||(pathc>=sizeof(path))) return -1;
  
  void *indexhtml=0;
  int indexhtmlc=fmn_file_read(&indexhtml,path);
  if (indexhtmlc<0) return http_xfer_set_status(rsp,404,"Not found");
  http_xfer_set_body(rsp,indexhtml,indexhtmlc);
  free(indexhtml);
  http_xfer_set_header(rsp,"Content-Type",12,"text/html",9);
  
  return http_xfer_set_status(rsp,200,"OK");
}
 
int fiddle_httpcb_get_root(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  if (!fiddle.htdocsc) return http_xfer_set_status(rsp,404,"Not found");
  if (!fiddle.datafile) return http_xfer_set_status(rsp,404,"Not found");
  char cmd[256];
  int cmdc=snprintf(cmd,sizeof(cmd),"make %.*s",fiddle.datac,fiddle.data);
  if ((cmdc<1)||(cmdc>=sizeof(cmd))) return http_xfer_set_status(rsp,500,"Invalid path");
  if (fiddle_spawn_process_sync(cmd,fiddle_httpcb_root_cb,rsp)<0) {
    http_xfer_del(rsp);
    return http_xfer_set_status(rsp,500,"Failed to spawn make");
  }
  return 0;
}

/* GET /*
 */
 
static void fiddle_sanitize_path(char *path,int pathc) {
  // Don't want to overthink this too hard. Anything that doesn't look kosher, replace with 'X'.
  // Don't allow double-dot entries, that's the main thing.
  int pathp=0;
  while (pathp<pathc) {
    if ((path[pathp]<0x20)||(path[pathp]>0x7e)) path[pathp]='X';
    else if ((pathp<=pathc-4)&&!memcmp(path+pathp,"/../",4)) memcpy(path+pathp,"/XX/",4);
    pathp++;
  }
}
 
int fiddle_httpcb_static(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {

  // Acquire the local path.
  if (!fiddle.htdocsc) return http_xfer_set_status(rsp,404,"Not found");
  const char *rpath=0;
  int rpathc=http_xfer_get_path(&rpath,req);
  if ((rpathc<1)||(rpath[0]!='/')) return http_xfer_set_status(rsp,400,"Invalid request path");
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"%.*s%.*s",fiddle.htdocsc,fiddle.htdocs,rpathc,rpath);
  if ((pathc<1)||(pathc>=sizeof(path))) return http_xfer_set_status(rsp,404,"Not found");
  fiddle_sanitize_path(path,pathc);
  
  // Regular files only.
  if (fmn_file_get_type(path)!='f') return http_xfer_set_status(rsp,404,"Not found");
  
  // Shovel into the response.
  void *serial=0;
  int serialc=fmn_file_read(&serial,path);
  if (serialc<0) return http_xfer_set_status(rsp,404,"Not found",path);
  if (http_xfer_set_body(rsp,serial,serialc)<0) {
    free(serial);
    return -1;
  }
  const char *type=fiddle_guess_mime_type(path,serial,serialc);
  if (type&&type[0]) http_xfer_set_header(rsp,"Content-Type",12,type,-1);
  free(serial);
  
  return 0;
}

/* Illegal method or path.
 */
 
int fiddle_httpcb_nonesuch(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  return http_xfer_set_status(rsp,404,"Not found");
}

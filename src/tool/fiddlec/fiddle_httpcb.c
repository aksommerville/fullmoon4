#include "fiddle_internal.h"
#include "opt/datafile/fmn_datafile.h"

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
  return sr_encode_json_object_end(&ctx->encoder,jsonctx);
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
      fiddle.synth->type->play_song(fiddle.synth,serial,serialc,1);
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

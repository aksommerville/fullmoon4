#include "mkd_internal.h"
#include "opt/midi/midi.h"

struct mkd_rfer_context {
  const void *v;
  int c;
  int (*cb)(int type,int id,void *userdata);
  void *userdata;
};

/* Song.
 */
 
static int mkd_song_for_each_reference(struct mkd_rfer_context *ctx) {
  int err=0;
  struct midi_file *file=midi_file_new_borrow(ctx->v,ctx->c);
  if (!file) return -1;
  while (1) {
    struct midi_event event;
    int delay=midi_file_next(&event,file,0);
    if (delay<0) break;
    if (delay>0) {
      if ((err=midi_file_advance(file,delay))<0) break;
      continue;
    }
    switch (event.opcode) {
      case MIDI_OPCODE_PROGRAM: if (err=ctx->cb(FMN_RESTYPE_INSTRUMENT,event.a,ctx->userdata)) goto _done_; break;
      case MIDI_OPCODE_NOTE_ON: if (event.chid==0x0f) { // our channel 15 is drums. (NB conventionally it's 9)
          if (err=ctx->cb(FMN_RESTYPE_SOUND,event.a,ctx->userdata)) goto _done_;
        } break;
    }
  }
 _done_:;
  midi_file_del(file);
  return err;
}

/* Map.
 */
 
static int mkd_map_for_each_reference_cb(uint8_t opcode,const uint8_t *argv,int argc,void *userdata) {
  int err=0;
  struct mkd_rfer_context *ctx=userdata;
  switch (opcode) {
    case 0x20: err=ctx->cb(FMN_RESTYPE_SONG,argv[0],ctx->userdata); break;
    case 0x21: {
        if (err=ctx->cb(FMN_RESTYPE_IMAGE,argv[0],ctx->userdata)) return err;
        if (err=ctx->cb(FMN_RESTYPE_TILEPROPS,argv[0],ctx->userdata)) return err;
      } break;
    case 0x40: err=ctx->cb(FMN_RESTYPE_MAP,(argv[0]<<8)|argv[1],ctx->userdata); break;
    case 0x41: err=ctx->cb(FMN_RESTYPE_MAP,(argv[0]<<8)|argv[1],ctx->userdata); break;
    case 0x42: err=ctx->cb(FMN_RESTYPE_MAP,(argv[0]<<8)|argv[1],ctx->userdata); break;
    case 0x43: err=ctx->cb(FMN_RESTYPE_MAP,(argv[0]<<8)|argv[1],ctx->userdata); break;
    case 0x60: err=ctx->cb(FMN_RESTYPE_MAP,(argv[1]<<8)|argv[2],ctx->userdata); break;
    
    case 0x80: {
        int spriteid=(argv[1]<<8)|argv[2];
        if (err=ctx->cb(FMN_RESTYPE_SPRITE,spriteid,ctx->userdata)) return err;
        // Spawn points can refer to other resources, if the sprite's "argtype" says so.
        // This is complicated and I don't feel like supporting it.
        // Only chalkguard uses it; hard-code that case.
        // fyi, the correct handling is shown at src/tool/datan/datan_validate_reachability.c
        if (spriteid==19) {
          if (err=ctx->cb(FMN_RESTYPE_STRING,argv[3],ctx->userdata)) return err;
          if (err=ctx->cb(FMN_RESTYPE_STRING,argv[4],ctx->userdata)) return err;
        }
      } break;
      
    case 0x81: err=ctx->cb(FMN_RESTYPE_MAP,(argv[3]<<8)|argv[4],ctx->userdata); break;
  }
  return 0;
}
 
static int mkd_map_for_each_reference(struct mkd_rfer_context *ctx) {
  return fmn_map_for_each_command(ctx->v,ctx->c,mkd_map_for_each_reference_cb,ctx);
}

/* Sprite.
 */
 
static int mkd_sprite_for_each_reference(struct mkd_rfer_context *ctx) {
  const uint8_t *src=ctx->v;
  int srcp=0,srcc=ctx->c,err;
  while (srcp<srcc) {
    uint8_t opcode=src[srcp++];
    if (!opcode) break;
    const uint8_t *argv=src+srcp;
         if (opcode<0x20) srcp+=0;
    else if (opcode<0x40) srcp+=1;
    else if (opcode<0x60) srcp+=2;
    else if (opcode<0x80) srcp+=3;
    else if (opcode<0xa0) srcp+=4;
    else {
      fprintf(stderr,"%s:%d: Long sprite ops not supported, oops i didn't think we used them\n",__FILE__,__LINE__);
      return -1;
    }
    if (srcp>srcc) return -1;
    
    switch (opcode) {
      case 0x20: if (err=ctx->cb(FMN_RESTYPE_IMAGE,argv[0],ctx->userdata)) return err; break;
    }
  }
  return 0;
}

/* For each reference, main entry point.
 */
 
int mkd_res_for_each_reference(int type,const void *v,int c,int (*cb)(int type,int id,void *userdata),void *userdata) {
  struct mkd_rfer_context ctx={
    .v=v,
    .c=c,
    .cb=cb,
    .userdata=userdata,
  };
  switch (type) {
    case FMN_RESTYPE_SONG: return mkd_song_for_each_reference(&ctx);
    case FMN_RESTYPE_MAP: return mkd_map_for_each_reference(&ctx);
    case FMN_RESTYPE_SPRITE: return mkd_sprite_for_each_reference(&ctx);
  }
  return 0;
}

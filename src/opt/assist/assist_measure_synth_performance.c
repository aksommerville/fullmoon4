#include "assist_internal.h"
#include "opt/bigpc/bigpc_synth.h" /* Header only; we can not actually link against bigpc. */
#include "opt/bigpc/bigpc_audio.h"
#include <time.h>

extern const struct bigpc_synth_type bigpc_synth_type_minsyn;
extern const struct bigpc_synth_type bigpc_synth_type_stdsyn;

/* Context.
 */
 
struct msp_context {
  const char *path;
  struct fmn_datafile *datafile;
  struct bigpc_synth_driver *driver;
};

static void msp_delete_driver(struct msp_context *ctx) {
  if (ctx->driver) {
    if (ctx->driver->type->del) ctx->driver->type->del(ctx->driver);
    free(ctx->driver);
    ctx->driver=0;
  }
}

static void msp_context_cleanup(struct msp_context *ctx) {
  fmn_datafile_del(ctx->datafile);
  msp_delete_driver(ctx);
}

/* Current CPU time.
 */
 
static double msp_now() {
  struct timespec tv={0};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tv);
  return (double)tv.tv_sec+(double)tv.tv_nsec/1000000000.0;
}

/* Play a song and record how long it takes, redlining the CPU.
 */
 
static int msp_play_song_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  #define BUFFER_SIZE 1024 /* should have a substantial effect on performance */
  struct msp_context *ctx=userdata;
  double timea=msp_now();
  if (ctx->driver->type->play_song(ctx->driver,v,c,1,0)<0) {
    fprintf(stderr,"%s: Error starting song %d, %d bytes\n",ctx->driver->type->name,id,c);
    return -2;
  }
  int framec=0;
  int16_t lo=0,hi=0;
  double sqsum=0.0;
  int16_t buffer[BUFFER_SIZE]; // TODO if we're going to support formats other than s16n, must handle it here.
  while (1) {
    ctx->driver->update(buffer,BUFFER_SIZE,ctx->driver);
    
    const int16_t *v=buffer;
    int i=BUFFER_SIZE;
    for (;i-->0;v++) {
      if (*v<lo) lo=*v; else if (*v>hi) hi=*v;
      double fv=(double)(*v)/32768.0;
      sqsum+=fv*fv;
    }
    
    framec+=BUFFER_SIZE;
    if (ctx->driver->song_finished) break;
    if (framec>100000000) { // 100 Mc is more than half an hour at 44.1 kHz. Runs this long, assume we're not detecting end of song.
      fprintf(stderr,"!!! PANIC !!! Driver %s did not finish song %d after 100 M samples.\n",ctx->driver->type->name,id);
      break;
    }
  }
  #undef BUFFER_SIZE
  double timez=msp_now();
  double elapsed=timez-timea;
  double generated=(double)framec/(double)ctx->driver->rate;
  double rms=sqrt(sqsum/framec);
  
  fprintf(stderr,
    "%s song:%02d framec=%07d elapsed=%.03f peak=%05d..%05d rms=%.06f cost=%.06f\n",
    ctx->driver->type->name,id,framec,elapsed,lo,hi,rms,elapsed/generated
  );
  
  return 0;
}

/* Load instrument or sound.
 */
 
static int msp_set_instrument_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct msp_context *ctx=userdata;
  if (ctx->driver->type->set_instrument(ctx->driver,id,v,c)<0) {
    fprintf(stderr,"%s: Error loading instrument %d.\n",ctx->driver->type->name,id);
    return -2;
  }
  return 0;
}
 
static int msp_set_sound_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct msp_context *ctx=userdata;
  if (ctx->driver->type->set_sound(ctx->driver,id,v,c)<0) {
    fprintf(stderr,"%s: Error loading sound %d.\n",ctx->driver->type->name,id);
    return -2;
  }
  return 0;
}

/* Run full test and emit report, for one synthesizer.
 */
 
static int msp_for_synthesizer(struct msp_context *ctx,const struct bigpc_synth_type *type,int resq) {
  int err;
  
  double time0=msp_now();
  
  /* Instantiate synthesizer.
   */
  msp_delete_driver(ctx);
  if (!(ctx->driver=calloc(1,type->objlen))) return -1;
  ctx->driver->type=type;
  ctx->driver->refc=1;
  ctx->driver->rate=44100;
  ctx->driver->chanc=1;
  ctx->driver->format=BIGPC_AUDIO_FORMAT_s16n;
  ctx->driver->music_enable=1;
  if (type->init&&(type->init(ctx->driver)<0)) {
    fprintf(stderr,"%s: init failed. chanc=%d rate=%d format=%d\n",type->name,ctx->driver->chanc,ctx->driver->rate,ctx->driver->format);
    return -2;
  }
  
  /* Load instruments and sounds.
   */
  if (type->set_instrument) {
    if ((err=fmn_datafile_for_each_of_qualified_type(ctx->datafile,FMN_RESTYPE_INSTRUMENT,resq,msp_set_instrument_cb,ctx))<0) {
      return err;
    }
  }
  if (type->set_sound) {
    if ((err=fmn_datafile_for_each_of_qualified_type(ctx->datafile,FMN_RESTYPE_SOUND,resq,msp_set_sound_cb,ctx))<0) {
      return err;
    }
  }
  
  double time_init=msp_now();
  
  /* Play each song to completion.
   */
  if (type->play_song) {
    if ((err=fmn_datafile_for_each_of_type(ctx->datafile,FMN_RESTYPE_SONG,msp_play_song_cb,ctx))<0) {
      return err;
    }
  } else {
    fprintf(stderr,"%s: play_song not implemented!\n",type->name);
  }
  
  double time_end=msp_now();
  
  /* Final report.
   */
  fprintf(stderr,"Completed test for synthesizer '%s'.\n",type->name);
  fprintf(stderr,"Rate: %d Hz\n",ctx->driver->rate);
  fprintf(stderr,"Chanc: %d\n",ctx->driver->chanc);
  fprintf(stderr,"Format: %d (%s)\n",ctx->driver->format,(ctx->driver->format==BIGPC_AUDIO_FORMAT_s16n)?"s16n":(ctx->driver->format==BIGPC_AUDIO_FORMAT_f32n)?"f32n":"?");
  fprintf(stderr,"Total CPU time: %.06f\n",time_end-time0);
  
  return 0;
}

/* Measure synth performance, main entry point.
 */
 
int assist_measure_synth_performance(const char *path) {
  struct msp_context ctx={
    .path=path,
  };
  if (!(ctx.datafile=fmn_datafile_open(path))) {
    fprintf(stderr,"%s: Failed to open archive.\n",path);
    return 1;
  }
  
  int err;
  #if FMN_USE_minsyn
    if ((err=msp_for_synthesizer(&ctx,&bigpc_synth_type_minsyn,2))<0) {
      if (err!=-2) fprintf(stderr,"%s:minsyn: Unspecified error.\n",path);
      msp_context_cleanup(&ctx);
      return 1;
    }
  #endif
  #if FMN_USE_stdsyn
    if ((err=msp_for_synthesizer(&ctx,&bigpc_synth_type_stdsyn,3))<0) {
      if (err!=-2) fprintf(stderr,"%s:stdsyn: Unspecified error.\n",path);
      msp_context_cleanup(&ctx);
      return 1;
    }
  #endif
  
  msp_context_cleanup(&ctx);
  return 0;
}

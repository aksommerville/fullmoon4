#include "assist_internal.h"
#include "opt/pcmprint/pcmprint.h"
#include <math.h>

/* Find all the minsyn sound effects, print them, judge printing performance and measure levels.
 */
 
struct assist_minsyn_context {
  // From caller:
  struct fmn_datafile *file;
  const char *path;
  
  // Extra configuration:
  int rate;
  int buffer_size;
  double level_head_length;
  const char *raw_pcm_out_path;
  
  // State:
  struct assist_minsyn_res {
    int id;
    void *serial;
    int serialc;
    struct pcmprint *pcmprint;
    int16_t *pcm;
    int pcmc,pcmp;
  } *resv;
  int resc,resa;
  int pcmc_total;
  int pcmc_max;
  double printed_duration;
  struct assist_timestamp t_start;
  struct assist_timestamp t_decoded;
  struct assist_timestamp t_printed;
};

static void assist_minsyn_context_cleanup(struct assist_minsyn_context *ctx) {
  if (ctx->resv) {
    struct assist_minsyn_res *res=ctx->resv;
    int i=ctx->resc;
    for (;i-->0;res++) {
      if (res->serial) free(res->serial);
      pcmprint_del(res->pcmprint);
      if (res->pcm) free(res->pcm);
    }
    free(ctx->resv);
  }
}

static int assist_minsyn_cb(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  struct assist_minsyn_context *ctx=userdata;
  
  // Restrict to one ID if desired.
  if (0) {
    if (id!=FMN_SFX_PLANT) return 0;
  }
  
  if (ctx->resc>=ctx->resa) {
    int na=ctx->resa+32;
    if (na>INT_MAX/sizeof(struct assist_minsyn_res)) return -1;
    void *nv=realloc(ctx->resv,sizeof(struct assist_minsyn_res)*na);
    if (!nv) return -1;
    ctx->resv=nv;
    ctx->resa=na;
  }
  struct assist_minsyn_res *res=ctx->resv+ctx->resc++;
  memset(res,0,sizeof(struct assist_minsyn_res));
  res->id=id;
  if (!(res->serial=malloc(c))) return -1;
  memcpy(res->serial,v,c);
  res->serialc=c;
  if (!(res->pcmprint=pcmprint_new(ctx->rate,v,c))) return -1;
  if ((res->pcmc=pcmprint_get_length(res->pcmprint))<1) return -1;
  if (!(res->pcm=malloc(res->pcmc<<1))) return -1;
  ctx->pcmc_total+=res->pcmc;
  if (res->pcmc>ctx->pcmc_max) ctx->pcmc_max=res->pcmc;
  return 0;
}

// dst[0]=peak, dst[1]=rms. Note that input is -32768..32767 and output is -1..1
static void assert_minsyn_measure_levels(double *dst,const int16_t *v,int c) {
  dst[0]=dst[1]=0.0;
  if (c<1) return;
  int i=c;
  for (;i-->0;v++) {
    double f=*v;
    if (f<0.0) f=-f;
    f/=32768.0;
    if (f>dst[0]) dst[0]=f;
    dst[1]+=f*f;
  }
  dst[1]=sqrt(dst[1]/c);
}

static int assist_write_pcm_expanding_stereo(const char *path,const int16_t *mono,int framec) {
  int16_t *stereo=malloc(framec<<2);
  if (!stereo) return -1;
  const int16_t *src=mono;
  int16_t *dst=stereo;
  int i=framec;
  for (;i-->0;src++,dst+=2) dst[0]=dst[1]=src[0];
  int err=fmn_file_write(path,stereo,framec<<2);
  free(stereo);
  return err;
}
 
static int assist_measure_minsyn_sound_levels_and_performance(
  struct fmn_datafile *file,const char *path
) {
  struct assist_minsyn_context ctx={
    .file=file,
    .path=path,
    .rate=44100, // expect performance to scale linearly with rate
    .buffer_size=1024, // print in chunks no larger than this. shouldn't make a big difference but larger might be better.
    // Note that there is no technical reason for buffer_size here, it's just to simulate real-world conditions.
    .level_head_length=0.25, // for level checks, only look at the first so many seconds. (so a long tail doesn't muddy the results)
    .raw_pcm_out_path="scratch/sfx.s16le", // write concatenation of all sounds here for manual review, or null to skip
  };
  
  assist_now(&ctx.t_start);
  
  // Read all the minsyn sound effects, create a printer for each, and allocate their pcm buffers.
  if (fmn_datafile_for_each_of_qualified_type(file,FMN_RESTYPE_SOUND,2,assist_minsyn_cb,&ctx)<0) {
    fprintf(stderr,"%s: Error reading TOC or initial processing.\n",path);
    assist_minsyn_context_cleanup(&ctx);
    return -1;
  }
  
  assist_now(&ctx.t_decoded);
  
  // Run each printer to completion.
  struct assist_minsyn_res *res=ctx.resv;
  int i=ctx.resc;
  for (;i-->0;res++) {
    while (1) {
      int updc=res->pcmc-res->pcmp;
      if (updc<=0) break;
      if (updc>ctx.buffer_size) updc=ctx.buffer_size;
      pcmprint_updatei(res->pcm+res->pcmp,updc,res->pcmprint);
      res->pcmp+=updc;
    }
  }
  
  assist_now(&ctx.t_printed);
  
  // Discuss performance.
  ctx.printed_duration=(double)ctx.pcmc_total/(double)ctx.rate;
  fprintf(stderr,"%s test complete\n",__func__);
  fprintf(stderr,"  path: %s\n",path);
  fprintf(stderr,"  rate: %d\n",ctx.rate);
  fprintf(stderr,"  buffer: %d\n",ctx.buffer_size);
  fprintf(stderr,"  sound count: %d\n",ctx.resc);
  fprintf(stderr,"  total length: %d samples (%.03f s)\n",ctx.pcmc_total,ctx.printed_duration);
  fprintf(stderr,"  longest sound: %d samples (%.03f s)\n",ctx.pcmc_max,(double)ctx.pcmc_max/(double)ctx.rate);
  fprintf(stderr,"  real time: %.06f s\n",ctx.t_printed.real-ctx.t_start.real);
  fprintf(stderr,"  cpu time: %.06f s\n",ctx.t_printed.cpu-ctx.t_start.cpu);
  fprintf(stderr,"  cpu usage: %.06f, should be close to 1\n",(ctx.t_printed.cpu-ctx.t_start.cpu)/(ctx.t_printed.real-ctx.t_start.real));
  fprintf(stderr,"  real rate: %.06f, lower is better, panic around 0.2?\n",(ctx.t_printed.real-ctx.t_start.real)/ctx.printed_duration);
  
  // Measure and report levels for each sound.
  int lenlimit=ctx.level_head_length*ctx.rate;
  if (lenlimit<1) lenlimit=1;
  if (lenlimit<ctx.pcmc_max) {
    fprintf(stderr,"Level checks for the first %.03f s of each sound:\n",ctx.level_head_length);
  } else {
    fprintf(stderr,"Level checks for full sounds:\n");
  }
  fprintf(stderr,"%5s %20s %5s %5s %5s\n","ID","NAME","LEN","PEAK","RMS");
  char visual[60]; // safe to adjust length here
  for (res=ctx.resv,i=ctx.resc;i-->0;res++) {
    double v[2];
    int c=res->pcmc;
    if (c>lenlimit) c=lenlimit;
    assert_minsyn_measure_levels(v,res->pcm,c);
    const char *name=0;
    int namec=assist_get_sound_name(&name,res->id);
    if (namec>20) namec=20;
    else if (namec<0) namec=0;
    
    int peakp=v[0]*sizeof(visual);
    int rmsp=v[1]*sizeof(visual);
    if (rmsp<0) rmsp=0;
    if (peakp>=sizeof(visual)) peakp=sizeof(visual)-1;
    if (peakp<1) peakp=1;
    if (rmsp>=peakp) rmsp=peakp-1;
    memset(visual,'*',rmsp);
    visual[rmsp]='R';
    memset(visual+rmsp+1,'_',peakp-rmsp-1);
    visual[peakp]='!';
    memset(visual+peakp+1,'.',sizeof(visual)-peakp);
    
    fprintf(stderr,
      "%5d %20.*s %.03f %.03f %.03f %.*s\n",
      res->id,namec,name,
      (double)res->pcmc/(double)ctx.rate,v[0],v[1],
      (int)sizeof(visual),visual
    );
  }
  
  // Dump PCM to a file for manual review, if requested.
  if (ctx.raw_pcm_out_path) {
    int16_t *raw=malloc(ctx.pcmc_total<<1);
    if (raw) {
      int rawc=0;
      for (res=ctx.resv,i=ctx.resc;i-->0;res++) {
        memcpy(raw+rawc,res->pcm,res->pcmc<<1);
        rawc+=res->pcmc;
      }
      // A lil extra cudgel for brain-dead systems like mine that can't play 44100 mono or even 22050 stereo.
      if (1) {
        if (assist_write_pcm_expanding_stereo(ctx.raw_pcm_out_path,raw,rawc)>=0) {
          fprintf(stderr,"%s: Saved PCM for manual review. Hint:\n  aplay -traw -fS16_LE -r%d -c2 %s\n",ctx.raw_pcm_out_path,ctx.rate,ctx.raw_pcm_out_path);
        }
      } else {
        if (fmn_file_write(ctx.raw_pcm_out_path,raw,rawc<<1)>=0) {
          fprintf(stderr,"%s: Saved PCM for manual review. Hint:\n  aplay -traw -fS16_LE -r%d -c1 %s\n",ctx.raw_pcm_out_path,ctx.rate,ctx.raw_pcm_out_path);
        }
      }
      free(raw);
    }
  }
  
  assist_minsyn_context_cleanup(&ctx);
  return 0;
}

/* Test results on Nuc:
assist_measure_minsyn_sound_levels_and_performance test complete
  path: out/assist/data
  rate: 44100
  buffer: 1024
  sound count: 66
  total length: 1628930 samples (36.937 s)
  real time: 0.021980 s
  cpu time: 0.021972 s
  cpu usage: 0.000000, should be close to 1 *** reporting error, ignore
  real rate: 0.000595, lower is better, panic around 0.2?

assist_measure_minsyn_sound_levels_and_performance test complete
  path: out/assist/data
  rate: 44100
  buffer: 999999999
  sound count: 66
  total length: 1628930 samples (36.937 s)
  real time: 0.024986 s
  cpu time: 0.024976 s
  cpu usage: 0.000000, should be close to 1 *** reporting error, ignore
  real rate: 0.000676, lower is better, panic around 0.2?
...long buffer. no significant difference.

assist_measure_minsyn_sound_levels_and_performance test complete
  path: out/assist/data
  rate: 11025
  buffer: 1024
  sound count: 66
  total length: 407211 samples (36.935 s)
  real time: 0.006060 s
  cpu time: 0.006059 s
  cpu usage: 0.999865, should be close to 1
  real rate: 0.000164, lower is better, panic around 0.2?
...low output rate. linearly improved performance, as expected
*/

/* Analyze data archive, main entry point.
 */
 
int assist_analyze_data(const char *path) {
  fprintf(stderr,"%s %s\n",__func__,path);
  struct fmn_datafile *file=fmn_datafile_open(path);
  if (!file) {
    fprintf(stderr,"%s: Failed to read or decode data archive.\n",path);
    return 1;
  }
  
  /* TODO We'll probably want a bunch of different kinds of validation.
   * Make these selectable at the command line.
   * For now, I'm only interested in audio.
   */
  if (assist_measure_minsyn_sound_levels_and_performance(file,path)<0) return 1;
  
  fmn_datafile_del(file);
  return 0;
}

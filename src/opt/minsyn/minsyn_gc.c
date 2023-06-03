#include "fmn_minsyn_internal.h"

/* Usage tracker.
 */
 
struct minsyn_usage {
  int *pcmv,*wavev;
  int pcmc,pcma,wavec,wavea;
};

static void minsyn_usage_cleanup(struct minsyn_usage *usage) {
  if (usage->pcmv) free(usage->pcmv);
  if (usage->wavev) free(usage->wavev);
}

/* ID list primitives.
 */
 
static int minsyn_usage_search(const int *v,int c,int q) {
  int lo=0,hi=c;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (q<v[ck]) hi=ck;
    else if (q>v[ck]) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int minsyn_usage_insert(int **v,int *c,int *a,int insp,int id) {
  if ((insp<0)||(insp>*c)) return -1;
  if (*c>=*a) {
    int na=(*a)+32;
    if (na>INT_MAX/sizeof(int)) return -1;
    void *nv=realloc(*v,sizeof(int)*na);
    if (!nv) return -1;
    *v=nv;
    *a=na;
  }
  memmove((*v)+insp+1,(*v)+insp,sizeof(int)*((*c)-insp));
  (*c)++;
  (*v)[insp]=id;
  return 0;
}

/* Add things to usage tracker.
 */
 
static int minsyn_usage_add_wave(struct minsyn_usage *usage,int id) {
  if (id<1) return 0;
  int p=minsyn_usage_search(usage->wavev,usage->wavec,id);
  if (p>=0) return 0;
  return minsyn_usage_insert(&usage->wavev,&usage->wavec,&usage->wavea,-p-1,id);
}

static int minsyn_usage_add_pcm(struct minsyn_usage *usage,int id) {
  if (id<1) return 0;
  int p=minsyn_usage_search(usage->pcmv,usage->pcmc,id);
  if (p>=0) return 0;
  return minsyn_usage_insert(&usage->pcmv,&usage->pcmc,&usage->pcma,-p-1,id);
}

static int minsyn_usage_add_wave_pointer(struct minsyn_usage *usage,struct bigpc_synth_driver *driver,void *v) {
  if (!v) return 0;
  struct minsyn_wave **p=DRIVER->wavev;
  int i=DRIVER->wavec;
  for (;i-->0;p++) if ((*p)->v==v) {
    return minsyn_usage_add_wave(usage,(*p)->id);
  }
  return 0;
}

/* Drop all unused things.
 * Returns count of objects removed (both PCMs and Waves).
 */
 
static int minsyn_usage_drop_unused(struct minsyn_usage *usage,struct bigpc_synth_driver *driver) {
  int rmc=0,idp,op;
  
  for (idp=0,op=0;op<DRIVER->pcmc;) {
    if ((idp>=usage->pcmc)||(usage->pcmv[idp]!=DRIVER->pcmv[op]->id)) {
      minsyn_pcm_del(DRIVER->pcmv[op]);
      DRIVER->pcmc--;
      memmove(DRIVER->pcmv+op,DRIVER->pcmv+op+1,sizeof(void*)*(DRIVER->pcmc-op));
      rmc++;
    } else {
      op++;
      idp++;
    }
  }
  
  for (idp=0,op=0;op<DRIVER->wavec;) {
    if ((idp>=usage->wavec)||(usage->wavev[idp]!=DRIVER->wavev[op]->id)) {
      free(DRIVER->wavev[op]);
      DRIVER->wavec--;
      memmove(DRIVER->wavev+op,DRIVER->wavev+op+1,sizeof(void*)*(DRIVER->wavec-op));
      rmc++;
    } else {
      op++;
      idp++;
    }
  }
  
  return rmc;
}

/* Garbage collection, main entry point.
 * Hopefully this doesn't happen at all in production; tweak thresholds to prevent it.
 * But it is very necessary for long fiddle sessions.
 */
 
void minsyn_gc(struct bigpc_synth_driver *driver) {
  fprintf(stderr,"%s pcmc=%d wavec=%d\n",__func__,DRIVER->pcmc,DRIVER->wavec);
  struct minsyn_usage usage={0};
  
  /* If any resource refers to it, it's in use.
   */
  struct minsyn_resource *res=DRIVER->resourcev;
  int i=DRIVER->resourcec;
  for (;i-->0;res++) {
    minsyn_usage_add_wave(&usage,res->waveid);
    minsyn_usage_add_wave(&usage,res->bwaveid);
    minsyn_usage_add_pcm(&usage,res->pcmid);
  }
  
  /* Any PCMs still printing, they are in use (even if not playing).
   */
  for (i=DRIVER->printerc;i-->0;) {
    minsyn_usage_add_pcm(&usage,DRIVER->printerv[i]->pcm->id);
  }
  
  /* And of course anything currently being played is in use.
   */
  struct minsyn_voice *voice=DRIVER->voicev;
  for (i=DRIVER->voicec;i-->0;voice++) {
    minsyn_usage_add_wave_pointer(&usage,driver,(void*)voice->v);
    minsyn_usage_add_wave_pointer(&usage,driver,(void*)voice->bv);
  }
  struct minsyn_playback *playback=DRIVER->playbackv;
  for (i=DRIVER->playbackc;i-->0;playback++) {
    if (!playback->pcm) continue;
    minsyn_usage_add_pcm(&usage,playback->pcm->id);
  }
  
  /* Now anything missing from (usage), drop it.
   */
  int rmc=minsyn_usage_drop_unused(&usage,driver);
  if (!rmc) {
    // This is bad. It means we will trigger again the next time anything is added.
    fprintf(stderr,"!!! %s triggered but did not detect any unused resources !!!\n",__func__);
  } else {
    fprintf(stderr,"...removed %d objects\n",rmc);
  }
  
  minsyn_usage_cleanup(&usage);
}

#include "fmn_render_internal.h"

#define WIND_NORM 0.5f /* magnitude of wind in non-blowback cases */
#define BLOWBACK_PRESENT ( \
  (fmn_global.blowbackx<-0.01f)|| \
  (fmn_global.blowbackx>0.01f)|| \
  (fmn_global.blowbacky<-0.01f)|| \
  (fmn_global.blowbacky>0.01f) \
)

/* The game-over fade out, if applicable.
 */
 
void fmn_render_fade_out() {
  if (fmn_global.werewolf_dead||fmn_global.hero_dead) {
    if (fmn_global.terminate_time>=1.0f) {
      // display as usual
    } else {
      // fade out
      int alpha=fmn_global.terminate_time*255.0f;
      if (alpha>0xff) alpha=0;
      else if (alpha<0) alpha=0xff;
      else alpha=0xff-alpha;
      uint32_t rgba=alpha;
      struct fmn_draw_rect vtx={0,0,fmn_render_global.fbw,fmn_render_global.fbh,rgba};
      fmn_draw_rect(&vtx,1);
    }
  }
}

/* Pulsing darkness, if applicable.
 */
 
void fmn_render_darkness() {
  if (!fmn_global.mapdark) return;
  uint32_t rgba;
  float countdown=(fmn_global.illumination_time>fmn_global.match_illumination_time)?fmn_global.illumination_time:fmn_global.match_illumination_time;
  if (countdown<=0.0f) { // full black
    rgba=0x000000ff;
    fmn_render_global.illuminationp=0.0f;
  } else { // complicated
    float level=FMN_RENDER_ILLUMINATION_PEAK-FMN_RENDER_ILLUMINATION_RANGE*0.5f; // amount of light (we need its reverse eventually)
    fmn_render_global.illuminationp+=(M_PI*2.0f)/FMN_RENDER_ILLUMINATION_PERIOD;
    if (fmn_render_global.illuminationp>M_PI) fmn_render_global.illuminationp-=M_PI*2.0f;
    float adjust=sinf(fmn_render_global.illuminationp)*FMN_RENDER_ILLUMINATION_RANGE*0.5f;
    level+=adjust;
    if (countdown<FMN_RENDER_ILLUMINATION_FADE_TIME) {
      level=(level*countdown)/FMN_RENDER_ILLUMINATION_FADE_TIME;
    }
    if (level<=0.0f) rgba=0x000000ff;
    else if (level>=1.0f) return;
    else {
      int alpha=(1.0f-level)*0x100;
      if (alpha<0) alpha=0;
      else if (alpha>0xff) alpha=0xff;
      rgba=alpha;
    }
  }
  struct fmn_draw_rect vtx={0,0,fmn_render_global.fbw,fmn_render_global.fbh,rgba};
  fmn_draw_rect(&vtx,1);
}

/* Fill the particle buffer randomly if needed.
 */
 
static void fmn_require_particles() {
  while (fmn_render_global.particlec<FMN_RENDER_PARTICLE_LIMIT) {
    struct fmn_draw_line *particle=fmn_render_global.particlev+fmn_render_global.particlec++;
    particle->ax=rand()%fmn_render_global.fbw;
    particle->ay=rand()%fmn_render_global.fbh;
  }
}

/* Rain.
 */
 
static void fmn_render_rain() {
  fmn_require_particles();
  const int16_t speed=4;
  const int16_t length=12;
  int16_t bottom=fmn_render_global.fbh+speed;
  struct fmn_draw_line *particle=fmn_render_global.particlev;
  int i=fmn_render_global.particlec;
  for (;i-->0;particle++) {
    particle->ay+=speed;
    if (particle->ay>bottom) {
      particle->ay=0;
      particle->ax=rand()%fmn_render_global.fbw;
    }
    particle->bx=particle->ax;
    particle->by=particle->ay-length;
    particle->pixel=0x00008880; //TODO pixfmt
  }
  fmn_draw_line(fmn_render_global.particlev,fmn_render_global.particlec);
}

/* Wind.
 */
 
static void fmn_render_wind(float nx,float ny) {
  fmn_require_particles();
  const float speed_limit=8.0f;
  const float tail_limit=-16.0f;
  int16_t movedx=nx*speed_limit+0.9f;
  int16_t movedy=ny*speed_limit+0.9f;
  int16_t taildx=nx*tail_limit;
  int16_t taildy=ny*tail_limit;
  struct fmn_draw_line *particle=fmn_render_global.particlev;
  int i=fmn_render_global.particlec;
  for (;i-->0;particle++) {
  
    particle->ax+=movedx;
    particle->ay+=movedy;
    if (particle->ax<0) {
      particle->ax=fmn_render_global.fbw;
      particle->ay=rand()%fmn_render_global.fbh;
    } else if (particle->ay<0) {
      particle->ax=rand()%fmn_render_global.fbw;
      particle->ay=fmn_render_global.fbh;
    } else if (particle->ax>fmn_render_global.fbw) {
      particle->ax=0;
      particle->ay=rand()%fmn_render_global.fbh;
    } else if (particle->ay>fmn_render_global.fbh) {
      particle->ax=rand()%fmn_render_global.fbw;
      particle->ay=0;
    }
    particle->bx=particle->ax+taildx;
    particle->by=particle->ay+taildy;
    
    particle->pixel=0xccccccc0;
  }
  fmn_draw_line(fmn_render_global.particlev,fmn_render_global.particlec);
}

/* Earthquake: Shuffle the whole framebuffer left or right.
 * I'm taking the safe way, at heavy expense, and copying to a scratch buffer.
 * CanvasRenderingContext2D says it's fine to self-copy.
 * I'm not sure GL would work, and much less sure about every GL implementation out there in the field.
 */
 
static void fmn_render_earthquake(int16_t dx) {
  int16_t fbw=fmn_render_global.fbw,fbh=fmn_render_global.fbh;
  if (fmn_draw_set_output(FMN_IMAGEID_SCRATCH)<0) return;
  struct fmn_draw_rect rect={0,0,0,fbh,0x000000ff};
  if (dx<0) {
    rect.x=fbw+dx;
    rect.w=-dx;
  } else {
    rect.w=dx;
  }
  fmn_draw_rect(&rect,1);
  struct fmn_draw_decal vtx={dx,0,fbw,fbh,0,0,fbw,fbh};
  fmn_draw_decal(&vtx,1,0);
  fmn_draw_set_output(0);
  vtx.dstx=0;
  fmn_draw_decal(&vtx,1,FMN_IMAGEID_SCRATCH);
}

/* General weather: Earthquake, rain, and wind, if applicable.
 */
 
void fmn_render_weather() {

  if (fmn_global.earthquake_dir) {
    int16_t dx=0;
    int phase=(int)(fmn_global.earthquake_time*10.0f)%4;
    switch (phase) {
      case 1: dx=1; break;
      case 3: dx=-1; break;
    }
    if (dx) fmn_render_earthquake(dx);
  }

  // Logically there can be rain and wind at the same time.
  // But it never happens by design, and it's convenient to use the same particle buffer for both.
  if (fmn_global.rain_time>0.0f) {
    fmn_render_rain();
  } else if (fmn_global.wind_time>0.0f) switch (fmn_global.wind_dir) {
    case FMN_DIR_N: fmn_render_wind(0.0f,-WIND_NORM); break;
    case FMN_DIR_S: fmn_render_wind(0.0f,WIND_NORM); break;
    case FMN_DIR_W: fmn_render_wind(-WIND_NORM,0.0f); break;
    case FMN_DIR_E: fmn_render_wind(WIND_NORM,0.0f); break;
  } else if (BLOWBACK_PRESENT) {
    fmn_render_wind(fmn_global.blowbackx,fmn_global.blowbacky);
  } else {
    fmn_render_global.particlec=0;
  }
}

/* Idle warning. Um. Not "weather" by any stretch, but shut up.
 */
 
void fmn_render_idle_warning(int s) {
  const int16_t ts=fmn_render_global.tilesize;
  const int16_t fullw=fmn_render_global.fbw;
  const int16_t fullh=fmn_render_global.fbh;
  
  struct fmn_draw_rect rect={0,0,fullw,fullh,0xff000080};
  fmn_draw_rect(&rect,1);

  int16_t srcx=0,srcy=ts*12;
  int16_t w=ts*7,h=ts*4;
  int16_t dstx=(fullw>>1)-(w>>1);
  int16_t dsty=(fullh>>1)-(h>>1);
  struct fmn_draw_decal vtxv[3];
  vtxv[0]=(struct fmn_draw_decal){dstx,dsty,w,h,srcx,srcy,w,h};
  
  int hidigit=s/10; if (hidigit>=10) hidigit=9;
  int lodigit=s%10;
  int digitsx=dstx+(w*2)/3-8;
  int digitsy=dsty+(h*3)/4-6;
  vtxv[1]=(struct fmn_draw_decal){digitsx+0,digitsy,8,12,srcx+w+hidigit*8,ts*16-12,8,12};
  vtxv[2]=(struct fmn_draw_decal){digitsx+8,digitsy,8,12,srcx+w+lodigit*8,ts*16-12,8,12};
  if (!hidigit) vtxv[1].srcw=vtxv[1].dstw=0;
  
  fmn_draw_decal(vtxv,3,14);
}

#include "../fmn_gl2_internal.h"

#define WIND_NORM 0.5f /* magnitude of wind in non-blowback cases */
#define BLOWBACK_PRESENT ( \
  (fmn_global.blowbackx<-0.01f)|| \
  (fmn_global.blowbackx>0.01f)|| \
  (fmn_global.blowbacky<-0.01f)|| \
  (fmn_global.blowbacky>0.01f) \
)

/* Fill the particle buffer randomly if needed.
 */
 
static void fmn_gl2_require_particles(struct bigpc_render_driver *driver) {
  while (DRIVER->game.particlec<FMN_GL2_PARTICLE_LIMIT) {
    struct fmn_gl2_vertex_raw *particle=DRIVER->game.particlev+DRIVER->game.particlec++;
    particle->x=rand()%DRIVER->mainfb.w;
    particle->y=rand()%DRIVER->mainfb.h;
  }
}

/* Rain.
 */
 
static void fmn_gl2_render_rain(struct bigpc_render_driver *driver) {
  fmn_gl2_require_particles(driver);
  const int16_t speed=4;
  const int16_t length=12;
  int16_t bottom=DRIVER->mainfb.h+speed;
  struct fmn_gl2_vertex_raw *particle=DRIVER->game.particlev;
  int i=DRIVER->game.particlec>>1;
  for (;i-->0;particle+=2) {
    particle[0].y+=speed;
    if (particle[0].y>bottom) {
      particle[0].y=0;
      particle[0].x=rand()%DRIVER->mainfb.w;
    }
    particle[1].x=particle[0].x;
    particle[1].y=particle[0].y-length;
    particle[0].r=particle[1].r=0x00;
    particle[0].g=particle[1].g=0x00;
    particle[0].b=particle[1].b=0x88;
    particle[0].a=0x80;
    particle[1].a=0x00;
  }
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw(GL_LINES,DRIVER->game.particlev,DRIVER->game.particlec);
}

/* Wind.
 */
 
static void fmn_gl2_render_wind(struct bigpc_render_driver *driver,float nx,float ny) {
  fmn_gl2_require_particles(driver);
  const float speed_limit=8.0f;
  const float tail_limit=-16.0f;
  int16_t movedx=nx*speed_limit+0.9f;
  int16_t movedy=ny*speed_limit+0.9f;
  int16_t taildx=nx*tail_limit;
  int16_t taildy=ny*tail_limit;
  struct fmn_gl2_vertex_raw *particle=DRIVER->game.particlev;
  int i=DRIVER->game.particlec>>1;
  for (;i-->0;particle+=2) {
  
    particle[0].x+=movedx;
    particle[0].y+=movedy;
    if (particle[0].x<0) {
      particle[0].x=DRIVER->mainfb.w;
      particle[0].y=rand()%DRIVER->mainfb.h;
    } else if (particle[0].y<0) {
      particle[0].x=rand()%DRIVER->mainfb.w;
      particle[0].y=DRIVER->mainfb.h;
    } else if (particle[0].x>DRIVER->mainfb.w) {
      particle[0].x=0;
      particle[0].y=rand()%DRIVER->mainfb.h;
    } else if (particle[0].y>DRIVER->mainfb.h) {
      particle[0].x=rand()%DRIVER->mainfb.w;
      particle[0].y=0;
    }
    particle[1].x=particle[0].x+taildx;
    particle[1].y=particle[0].y+taildy;
    
    particle[0].r=particle[1].r=0xcc;
    particle[0].g=particle[1].g=0xcc;
    particle[0].b=particle[1].b=0xcc;
    particle[0].a=0xc0;
    particle[1].a=0x00;
  }
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw(GL_LINES,DRIVER->game.particlev,DRIVER->game.particlec);
}

/* Darkness.
 */
 
void fmn_gl2_render_mapdark(struct bigpc_render_driver *driver) {
  if (!fmn_global.mapdark) return;
  uint32_t rgba;
  float countdown=(fmn_global.illumination_time>fmn_global.match_illumination_time)?fmn_global.illumination_time:fmn_global.match_illumination_time;
  if (countdown<=0.0f) { // full black
    rgba=0x000000ff;
    DRIVER->game.illuminationp=0.0f;
  } else { // complicated
    float level=FMN_GL2_ILLUMINATION_PEAK-FMN_GL2_ILLUMINATION_RANGE*0.5f; // amount of light (we need its reverse eventually)
    DRIVER->game.illuminationp+=(M_PI*2.0f)/FMN_GL2_ILLUMINATION_PERIOD;
    if (DRIVER->game.illuminationp>M_PI) DRIVER->game.illuminationp-=M_PI*2.0f;
    float adjust=sinf(DRIVER->game.illuminationp)*FMN_GL2_ILLUMINATION_RANGE*0.5f;
    level+=adjust;
    if (countdown<FMN_GL2_ILLUMINATION_FADE_TIME) {
      level=(level*countdown)/FMN_GL2_ILLUMINATION_FADE_TIME;
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
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw_rect(0,0,DRIVER->mainfb.w,DRIVER->mainfb.h,rgba);
}

/* Weather, main entry point.
 */
 
void fmn_gl2_game_render_weather(struct bigpc_render_driver *driver) {
  DRIVER->fbquakex=0;
  DRIVER->fbquakey=0;
  if (fmn_global.earthquake_dir) {
    int phase=(int)(fmn_global.earthquake_time*10.0f)%4;
    switch (phase) {
      case 1: DRIVER->fbquakex=1; break;
      case 3: DRIVER->fbquakex=-1; break;
    }
  }
  // Logically there can be rain and wind at the same time.
  // But it never happens by design, and it's convenient to use the same particle buffer for both.
  if (fmn_global.rain_time>0.0f) {
    fmn_gl2_render_rain(driver);
  } else if (fmn_global.wind_time>0.0f) switch (fmn_global.wind_dir) {
    case FMN_DIR_N: fmn_gl2_render_wind(driver,0.0f,-WIND_NORM); break;
    case FMN_DIR_S: fmn_gl2_render_wind(driver,0.0f,WIND_NORM); break;
    case FMN_DIR_W: fmn_gl2_render_wind(driver,-WIND_NORM,0.0f); break;
    case FMN_DIR_E: fmn_gl2_render_wind(driver,WIND_NORM,0.0f); break;
  } else if (BLOWBACK_PRESENT) {
    fmn_gl2_render_wind(driver,fmn_global.blowbackx,fmn_global.blowbacky);
  } else {
    DRIVER->game.particlec=0;
  }
}

#include "app/fmn_platform.h"
#include "app/menu/fmn_scene.h"

#define FMN_SCENE_BLACKOUT_TIME 3.0f

#define FMN_IMAGEID_MAPBITS 301 /* fmn_render_internal.h */

/* Render map.
 */
 
static void fmn_scene_render_mapbits(struct fmn_scene *scene,const uint8_t *v) {
  if (fmn_draw_set_output(FMN_IMAGEID_MAPBITS)<0) return;
  struct fmn_draw_mintile vtxv[FMN_COLC*FMN_SCENE_ROWC];
  struct fmn_draw_mintile *vtx=vtxv;
  const int16_t tilesize=16;
  int16_t y=tilesize>>1;
  int yi=FMN_SCENE_ROWC; for (;yi-->0;y+=tilesize) {
    int16_t x=tilesize>>1;
    int xi=FMN_COLC; for (;xi-->0;vtx++,v++,x+=tilesize) {
      vtx->x=x;
      vtx->y=y;
      vtx->tileid=*v;
      vtx->xform=0;
    }
  }
  fmn_draw_mintile(vtxv,FMN_COLC*FMN_SCENE_ROWC,scene->maptsid);
  fmn_draw_set_output(0);
}

/* Read commands from map.
 */
 
static void fmn_scene_read_map_commands(struct fmn_scene *scene,const uint8_t *src,int16_t srcc) {
  int16_t srcp=0;
  while (srcp<srcc) {
    uint8_t opcode=src[srcp++];
    if (!opcode) break;
    const uint8_t *argv=src+srcp;
    uint8_t argc=0;
         if (opcode<0x20) argc=0;
    else if (opcode<0x40) argc=1;
    else if (opcode<0x60) argc=2;
    else if (opcode<0x80) argc=4;
    else if (opcode<0xa0) argc=6;
    else if (opcode<0xc0) argc=8;
    else break; // No extension or variable-length commands should be in use. Stop reading if one comes up.
    if (srcp>srcc-argc) break;
    srcp+=argc;
    switch (opcode) {
      case 0x21: scene->maptsid=argv[0]; break; // TILESHEET
      case 0x45: break; // HERO (cellp,spellid)
      case 0x80: break; // SPRITE (cellp,spriteid(2),arg0,arg1,arg2)
    }
  }
}

/* Init.
 */
 
int fmn_scene_init(struct fmn_scene *scene,const struct fmn_scene_setup *setup) {
  scene->setup=setup;
  scene->blackout=0xff;
  scene->clock=0.0f;
  scene->maptsid=0;
  scene->animclock=0.0f;
  scene->animframe=0;
  scene->stage=0;
  scene->stageextra=0;
  scene->mapdirty=0;
  scene->stageclock=0.0f;
  
  if (scene->setup->mapid) {
    uint8_t map[512];
    int16_t mapc=fmn_get_map(map,sizeof(map),scene->setup->mapid);
    if ((mapc<FMN_COLC*FMN_ROWC)||(mapc>sizeof(map))) return -1;
    fmn_scene_read_map_commands(scene,map+FMN_COLC*FMN_ROWC,mapc-FMN_COLC*FMN_ROWC);
    memcpy(scene->map,map,sizeof(scene->map));
    scene->mapdirty=1;
  }
  
  return 0;
}

/* Init blank.
 */
 
void fmn_scene_blank(struct fmn_scene *scene) {
  scene->setup=0;
  scene->blackout=0xff;
  scene->clock=0.0f;
}

/* Update.
 */
 
uint8_t fmn_scene_update(struct fmn_scene *scene,float elapsed) {
  if (!scene->setup) {
    scene->blackout=0xff;
    return 0;
  }
  
  uint8_t result=0;
  scene->clock+=elapsed;
  if (scene->clock>=scene->setup->duration) {
    result=1;
    scene->blackout=0xff;
  } else if (scene->clock>=scene->setup->duration-FMN_SCENE_BLACKOUT_TIME) {
    scene->blackout=((FMN_SCENE_BLACKOUT_TIME-scene->setup->duration+scene->clock)*255.0f)/FMN_SCENE_BLACKOUT_TIME;
  } else if (scene->clock<FMN_SCENE_BLACKOUT_TIME) {
    scene->blackout=((FMN_SCENE_BLACKOUT_TIME-scene->clock)*255.0f)/FMN_SCENE_BLACKOUT_TIME;
  } else {
    scene->blackout=0;
  }
  
  scene->animclock+=elapsed;
  if (scene->animclock>=0.15f) {
    scene->animclock-=0.15f;
    scene->animframe++;
    if (scene->animframe>=4) scene->animframe=0;
  }
  
  scene->stageclock+=elapsed; // for renderer's use
  
  return result;
}

/* Render Dot dragging the Werewolf.
 */
 
static void fmn_scene_render_drag(int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,struct fmn_scene *scene,uint8_t xform) {
  const float walkspeed=20.0f;//pixel/sec
  const int16_t tilesize=16;
  int16_t doty=dsty+(dsth>>1);
  int16_t dotx=scene->clock*walkspeed;
  if (!(xform&FMN_XFORM_XREV)) dotx=dstw-dotx;
  dotx+=dstx;
  struct fmn_draw_mintile dotvtxv[]={
    {dotx,doty,0x22,xform},
    {dotx,doty-8,0x12,xform},
    {dotx,doty-13,0x02,xform},
  };
  switch (scene->animframe) {
    case 0: break;
    case 1: dotvtxv[0].tileid+=0x10; break;
    case 2: dotvtxv[0].tileid+=0x20; break;
    case 3: dotvtxv[0].tileid+=0x10; break;
  }
  fmn_draw_mintile(dotvtxv,sizeof(dotvtxv)/sizeof(dotvtxv[0]),2);
  struct fmn_draw_decal wolfvtx={
    0,doty-10,tilesize*5,tilesize*2,
    tilesize*6,tilesize*3,tilesize*5,tilesize*2,
  };
  if (xform&FMN_XFORM_XREV) {
    wolfvtx.dstx=dotx+8;
    wolfvtx.dstw=-wolfvtx.dstw;
  } else {
    wolfvtx.dstx=dotx-8;
  }
  fmn_draw_decal(&wolfvtx,1,13);
  
  // Dragon's eyeballs, for the second "drag" scene only. Luckily that's the only scene with XREV, so cheat.
  if (xform&FMN_XFORM_XREV) {
    uint32_t eyecolor=fmn_video_pixel_from_rgba(0xa60c4eff);
    int16_t eyedx=0,eyedy=0;
    if (scene->clock<9.0f) eyedx=-1;
    else if (scene->clock<11.0f) { eyedx=-1; eyedy=1; }
    else if (scene->clock<13.0f) eyedy=1;
    else { eyedx=1; eyedy=1; }
    struct fmn_draw_rect vtxv[]={
      {235+eyedx,46+eyedy,1,1,eyecolor},
      {241+eyedx,47+eyedy,1,1,eyecolor},
    };
    fmn_draw_rect(vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
}

/* Render Dot clothing the orphans.
 */
 
static void fmn_scene_render_clothe(int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,struct fmn_scene *scene) {
  const float walkspeed=20.0f;//pixel/sec
  const int16_t tilesize=16;
  
  // The orphans. 2, 3, or 4 tiles.
  struct fmn_draw_mintile ovtxv[]={
    {dstx+tilesize*6+(tilesize>>1),dsty+tilesize*2+(tilesize>>1),0x77,0}, // billy
    {dstx+tilesize*7+(tilesize>>1),dsty+tilesize*2+(tilesize>>1),0x78,0}, // janie
    {dstx+tilesize*6+(tilesize>>1),dsty+tilesize*1+(tilesize>>1),0x68,0}, // billy's snowflake
    {dstx+tilesize*7+(tilesize>>1),dsty+tilesize*1+(tilesize>>1),0x68,0}, // janie's snowflake
  };
  int ovtxc=4;
  
  // Dot. Initial guess assuming we're in stage zero and she's walking in from the right.
  int16_t doty=dsty+(dsth>>1);
  int16_t dotx=dstx+dstw-scene->clock*walkspeed;
  struct fmn_draw_mintile dotvtxv[]={
    {dotx,doty,0x22,0},
    {dotx,doty-8,0x12,0},
    {dotx,doty-13,0x02,0},
  };
  switch (scene->animframe) {
    case 0: break;
    case 1: dotvtxv[0].tileid+=0x10; break;
    case 2: dotvtxv[0].tileid+=0x20; break;
    case 3: dotvtxv[0].tileid+=0x10; break;
  }
  
  // Stage adjustments.
  switch (scene->stage) {
    case 0: { // Orphans cold, Dot walking in from the right. That's the default, so just check for completion.
        if (dotx<=ovtxv[1].x) {
          scene->stage=1;
          scene->stageclock=0.0f;
          fmn_sound_effect(FMN_SFX_ENCHANT_ANIMAL);
        }
      } break;
    case 1: { // Clothing Janie.
        ovtxc-=1;
        ovtxv[1].tileid=0x7c;
        dotvtxv[0].tileid=0x21;
        dotvtxv[1].tileid=0x11;
        dotvtxv[2].tileid=0x01;
        dotvtxv[0].x=dotvtxv[1].x=dotvtxv[2].x=ovtxv[1].x;
        if (scene->stageclock>=1.0f) {
          scene->stage=2;
          scene->stageclock=0.0f;
        }
      } break;
    case 2: { // Walk from Janie to Billy. Janie is clothed.
        ovtxc-=1;
        ovtxv[1].tileid=0x7c;
        dotvtxv[0].x=dotvtxv[1].x=dotvtxv[2].x=ovtxv[1].x-walkspeed*scene->stageclock;
        if (dotvtxv[0].x<=ovtxv[0].x) {
          scene->stage=3;
          scene->stageclock=0.0f;
          fmn_sound_effect(FMN_SFX_ENCHANT_ANIMAL);
        }
      } break;
    case 3: { // Clothing Billy.
        ovtxc-=2;
        ovtxv[0].tileid=0x7b;
        ovtxv[1].tileid=0x7c;
        dotvtxv[0].tileid=0x21;
        dotvtxv[1].tileid=0x11;
        dotvtxv[2].tileid=0x01;
        dotvtxv[0].x=dotvtxv[1].x=dotvtxv[2].x=ovtxv[0].x;
        if (scene->stageclock>=1.0f) {
          scene->stage=4;
          scene->stageclock=0.0f;
        }
      } break;
    case 4: { // Orphans are clothed and Dot's walking home. This stage runs forever.
        ovtxc-=2;
        ovtxv[0].tileid=0x7b;
        ovtxv[1].tileid=0x7c;
        dotvtxv[0].x=dotvtxv[1].x=dotvtxv[2].x=ovtxv[0].x+walkspeed*scene->stageclock;
        dotvtxv[0].xform=dotvtxv[1].xform=dotvtxv[2].xform=FMN_XFORM_XREV;
      } break;
  }
  
  // Finally, let the orphans blink from time to time.
  if (scene->stageextra>0) {
    scene->stageextra--;
    ovtxv[0].tileid+=0x10;
  } else if (scene->stageextra<0) {
    scene->stageextra++;
    ovtxv[1].tileid+=0x10;
  } else switch (rand()%300) {
    case 0: scene->stageextra=15; break;
    case 1: scene->stageextra=-15; break;
  }
  
  fmn_draw_mintile(ovtxv,ovtxc,24);
  fmn_draw_mintile(dotvtxv,sizeof(dotvtxv)/sizeof(dotvtxv[0]),2);
}

/* Good night: Just a wee overlay when Dot claps, and an extra blackout after.
 */
 
static void fmn_scene_render_goodnight(int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,struct fmn_scene *scene) {
  const int16_t tilesize=16;
  uint8_t dottile=0;
  
  if (scene->stageclock>=9.6f) { // dark
    scene->stage=5;
    struct fmn_draw_rect vtx={
      dstx+4*tilesize,dsty,dstw-8*tilesize,dsth,
      fmn_video_pixel_from_rgba(0x100820c0),
    };
    fmn_draw_rect(&vtx,1);
    
  } else if (scene->stageclock>=9.3f) { // end second clap
    scene->stage=4;
    dottile=0x5c;
    
  } else if (scene->stageclock>=9.0f) { // second clap enagaged
    if (scene->stage<3) fmn_sound_effect(FMN_SFX_CLAP);
    scene->stage=3;
    dottile=0x6c;
    
  } else if (scene->stageclock>=8.7f) { // end first clap
    scene->stage=2;
    dottile=0x5c;
    
  } else if (scene->stageclock>=8.4f) { // first clap enagaged
    if (scene->stage<1) fmn_sound_effect(FMN_SFX_CLAP);
    scene->stage=1;
    dottile=0x6c;
  }
    
  if (dottile) {
    struct fmn_draw_mintile vtx={
      .x=dstx+14*tilesize,
      .y=dsty+2*tilesize+4,
      .tileid=dottile,//0x5c,
      .xform=0,
    };
    fmn_draw_mintile(&vtx,1,21);
  }
}

/* Butchering the wolf: Big animation.
 */
 
void fmn_scene_render_butcher(int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,struct fmn_scene *scene) {
  const int16_t saww=186;
  const int16_t sawh=105;
  const int16_t dotw=211;
  const int16_t doth=128;
  const int16_t pathx=0; // saw travels along a diagonal line
  const int16_t pathw=80;
  const int16_t pathy=102;
  const int16_t pathh=-40;
  const double sawperiod=1.5f;
  float dummy;
  float sawnorm=modff(scene->stageclock/sawperiod,&dummy);
  float sawflat=sawnorm; // 0..1, full cycle
  if (sawnorm>0.5f) sawnorm=1.0f-sawnorm;
  sawnorm*=2.0f;
  int16_t sawx=pathx+pathw*sawnorm;
  int16_t sawy=pathy+pathh*sawnorm;
  struct fmn_draw_decal decalv[]={
    {dstx,dsty,dstw,dsth, 0,0,dstw,dsth}, // background
    {sawx,sawy,saww,sawh, dstw-saww,384,saww,sawh}, // saw and arm
    {dstx+dstw-dotw,dsty,dotw,doth, dstw-dotw,256,dotw,doth}, // dot minus right arm
  };
  fmn_draw_decal(decalv,sizeof(decalv)/sizeof(decalv[0]),26);
  
  if ((sawflat>=0.100f)&&(sawflat<0.400f)) {
    float t=(sawflat-0.100f)/0.300f;
    struct fmn_draw_decal splatv[]={
      {dstx+30-t*10,dsty+dsth-t*20+10,7,11, 0,256,7,11},
      {dstx+50     ,dsty+dsth-t*40+ 0,7,11, 0,256,7,11},
      {dstx+70+t*10,dsty+dsth-t*20+10,7,11, 0,256,7,11},
    };
    fmn_draw_decal(splatv,sizeof(splatv)/sizeof(splatv[0]),26);
  }
  
  // We exceed the lower limit, so black that out. fmn_menu_credits draws scene before text, don't worry.
  struct fmn_draw_rect rect={dstx,dsty+dsth,dstw,256,fmn_video_pixel_from_rgba(0x000000ff)};
  fmn_draw_rect(&rect,1);
}

/* Sewing: Big animation.
 */
 
void fmn_scene_render_sew(int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,struct fmn_scene *scene) {
  const int16_t lgw=78;
  const int16_t lgh=109;
  const int16_t smw=29;
  const int16_t smh=37;
  int16_t dotx=80;
  int16_t doty=(dsth>>1)-(lgh>>1);
  int16_t frame=0;
  if (scene->stageclock>=1.5f) {
    scene->stageclock-=1.5f;
  } else if (scene->stageclock>=0.75f) {
    frame=1;
  }
  struct fmn_draw_decal decalv[]={
    {dstx,dsty,dstw,dsth, 0,128,dstw,dsth}, // background
    {dstx+dotx,dsty+doty,lgw,lgh, 0,267,lgw,lgh}, // dot, minus right hand
    {dstx+dotx+53,dsty+doty+26,smw,smh, frame*smw,376,smw,smh}, // right hand, animated
  };
  fmn_draw_decal(decalv,sizeof(decalv)/sizeof(decalv[0]),26);
}

/* Render.
 */
 
void fmn_scene_render(int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth,struct fmn_scene *scene) {
  if (scene->blackout!=0xff) {
    if (scene->setup->mapid) {
      if (scene->mapdirty) {
        fmn_scene_render_mapbits(scene,scene->map);
        scene->mapdirty=0;
      }
      struct fmn_draw_decal decal={
        dstx,dsty,dstw,dsth,
        0,0,dstw,dsth,
      };
      fmn_draw_decal(&decal,1,FMN_IMAGEID_MAPBITS);
    }
    switch (scene->setup->action) {
      case FMN_SCENE_ACTION_DRAG_RL: fmn_scene_render_drag(dstx,dsty,dstw,dsth,scene,0); break;
      case FMN_SCENE_ACTION_DRAG_LR: fmn_scene_render_drag(dstx,dsty,dstw,dsth,scene,FMN_XFORM_XREV); break;
      case FMN_SCENE_ACTION_DRAG_HOME: fmn_scene_render_drag(dstx,dsty,dstw,dsth,scene,0); break;
      case FMN_SCENE_ACTION_CLOTHE: fmn_scene_render_clothe(dstx,dsty,dstw,dsth,scene); break;
      case FMN_SCENE_ACTION_GOODNIGHT: fmn_scene_render_goodnight(dstx,dsty,dstw,dsth,scene); break;
      case FMN_SCENE_ACTION_BUTCHER: fmn_scene_render_butcher(dstx,dsty,dstw,dsth,scene); break;
      case FMN_SCENE_ACTION_SEW: fmn_scene_render_sew(dstx,dsty,dstw,dsth,scene); break;
    }
  }
  if (scene->blackout) {
    struct fmn_draw_rect vtx={
      .x=dstx,.y=dsty,.w=dstw,.h=dsth,
      .pixel=fmn_video_pixel_from_rgba(0x00000000|scene->blackout),
    };
    fmn_draw_rect(&vtx,1);
  }
}

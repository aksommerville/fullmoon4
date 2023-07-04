#include "fmn_render_internal.h"
#include "app/hero/fmn_hero.h"

/* Prepare transition.
 */
 
void fmn_prepare_transition(int transition) {
  switch (transition) {
    case FMN_TRANSITION_PAN_LEFT:
    case FMN_TRANSITION_PAN_RIGHT:
    case FMN_TRANSITION_PAN_UP:
    case FMN_TRANSITION_PAN_DOWN: {
        fmn_render_global.hero_above_transition=1;
      } break;
    default: fmn_render_global.hero_above_transition=0;
  }
  fmn_render_global.transition=transition;
  
  fmn_draw_set_output(FMN_IMAGEID_TRANSITION_FROM);
  fmn_render_world(fmn_render_global.hero_above_transition?0:1);
  
  // Capture color and "from" position, in case we need them.
  if (fmn_global.mapflag&FMN_MAPFLAG_DARK) fmn_render_global.transition_color=0x22004400;
  else fmn_render_global.transition_color=0x00000000;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  fmn_render_global.transition_from_x=herox*fmn_render_global.tilesize;
  fmn_render_global.transition_from_y=heroy*fmn_render_global.tilesize;
}

/* Commit transition.
 */
 
void fmn_commit_transition() {
  fmn_render_global.transitionp=0;
  fmn_render_global.transitionc=FMN_RENDER_TRANSITION_FRAMEC; // make it official
  if (fmn_global.mapflag&FMN_MAPFLAG_DARK) fmn_render_global.transition_color=0x22004400; // purple if either "from" or "to" is dark.
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  fmn_render_global.transition_to_x=herox*fmn_render_global.tilesize;
  fmn_render_global.transition_to_y=heroy*fmn_render_global.tilesize;
}

/* Cancel.
 */
 
void fmn_cancel_transition() {
  fmn_render_global.transition=0;
}

/* Transition not possible or necessary; copy one of the edge images verbatim.
 */
 
static void fmn_transition_edge(int imageid) {
  int16_t w=fmn_render_global.fbw,h=fmn_render_global.fbh;
  struct fmn_draw_decal vtx={
    .dstx=0,.dsty=0,.dstw=w,.dsth=h,
    .srcx=0,.srcy=0,.srcw=w,.srch=h,
  };
  fmn_draw_decal(&vtx,1,imageid);
}

/* Pan.
 * (dx,dy) are the direction of the camera's apparent motion. Framebuffers slide the opposite way.
 */
 
static void fmn_render_pan(
  int p,int c,
  int imageid_from,int imageid_to,
  int dx,int dy
) {
  int16_t dxpx=(dx*fmn_render_global.fbw*p)/c;
  int16_t dypx=(dy*fmn_render_global.fbh*p)/c;
  int16_t dstx=-dxpx,dsty=-dypx; // where (from) goes
  {
    struct fmn_draw_decal vtx={
      .dstx=dstx,.dsty=dsty,.dstw=fmn_render_global.fbw,.dsth=fmn_render_global.fbh,
      .srcx=0,.srcy=0,.srcw=fmn_render_global.fbw,.srch=fmn_render_global.fbh,
    };
    fmn_draw_decal(&vtx,1,imageid_from);
  }
  dstx+=dx*fmn_render_global.fbw;
  dsty+=dy*fmn_render_global.fbh;
  {
    struct fmn_draw_decal vtx={
      .dstx=dstx,.dsty=dsty,.dstw=fmn_render_global.fbw,.dsth=fmn_render_global.fbh,
      .srcx=0,.srcy=0,.srcw=fmn_render_global.fbw,.srch=fmn_render_global.fbh,
    };
    fmn_draw_decal(&vtx,1,imageid_to);
  }
}

/* Fade to an intermediate color, then to the final.
 */

static void fmn_render_fade2(
  int p,int c,
  int imageid_from,int imageid_to
) {
  int imageid;
  int halfc=c>>1;
  if (p<halfc) {
    imageid=imageid_from;
    c=halfc;
  } else {
    imageid=imageid_to;
    p=c-p;
    c=halfc;
  }
  if (p<c) {
    struct fmn_draw_decal vtx={
      .dstx=0,.dsty=0,.dstw=fmn_render_global.fbw,.dsth=fmn_render_global.fbh,
      .srcx=0,.srcy=0,.srcw=fmn_render_global.fbw,.srch=fmn_render_global.fbh,
    };
    fmn_draw_decal(&vtx,1,imageid);
  }
  {
    int a=(p*0x100)/c;
    if (a<0) a=0; else if (a>0xff) a=0xff;
    uint32_t rgba=fmn_render_global.transition_color|a;
    struct fmn_draw_rect vtx={
      .x=0,.y=0,.w=fmn_render_global.fbw,.h=fmn_render_global.fbh,
      .pixel=fmn_video_pixel_from_rgba(rgba),
    };
    fmn_draw_rect(&vtx,1);
  }
}

/* Spotlight.
 */
 
static void fmn_render_spotlight(
  int p,int c,
  int imageid_from,int imageid_to
) {
  int imageid;
  int16_t fx,fy;
  int halfc=c>>1;
  if (p<halfc) {
    imageid=imageid_from;
    c=halfc;
    fx=fmn_render_global.transition_from_x;
    fy=fmn_render_global.transition_from_y;
  } else {
    imageid=imageid_to;
    p=c-p;
    c=halfc;
    fx=fmn_render_global.transition_to_x;
    fy=fmn_render_global.transition_to_y;
  }
  if (p>=c) {
    struct fmn_draw_rect vtx={
      .x=0,.y=0,.w=fmn_render_global.fbw,.h=fmn_render_global.fbh,
      .pixel=fmn_video_pixel_from_rgba(fmn_render_global.transition_color|0xff),
    };
    fmn_draw_rect(&vtx,1);
    return;
  }
  
  // Start with the unmodified scene.
  {
    struct fmn_draw_decal vtx={
      .dstx=0,.dsty=0,.dstw=fmn_render_global.fbw,.dsth=fmn_render_global.fbh,
      .srcx=0,.srcy=0,.srcw=fmn_render_global.fbw,.srch=fmn_render_global.fbh,
    };
    fmn_draw_decal(&vtx,1,imageid);
  }
  if (p<=0) return;
  
  // Spotlight's radius is the distance from focus to the furthest corner.
  int16_t fullw=fmn_render_global.fbw;
  int16_t fullh=fmn_render_global.fbh;
  float dx,dy;
  if (fx>fullw>>1) dx=fx; else dx=fullw-fx;
  if (fy>fullh>>1) dy=fy; else dy=fullh-fy;
  int16_t radius_limit=sqrtf(dx*dx+dy*dy)+0.9f;
  int16_t radius=((c-p)*radius_limit)/c;
  
  // Circle shape will be tricky. First draw the four rectangles entirely outside the spotlight.
  uint32_t bgcolor=fmn_video_pixel_from_rgba(fmn_render_global.transition_color|0xff);
  int16_t ax=fx-radius,zx=fx+radius,ay=fy-radius,zy=fy+radius;
  {
    struct fmn_draw_rect vtxv[4];
    int vtxc=0;
    if (ax>0) vtxv[vtxc++]=(struct fmn_draw_rect){0,0,ax,fullh,bgcolor};
    if (ay>0) vtxv[vtxc++]=(struct fmn_draw_rect){0,0,fullw,ay,bgcolor};
    if (zx<fullw) vtxv[vtxc++]=(struct fmn_draw_rect){zx,0,fullw-zx,fullh,bgcolor};
    if (zy<fullh) vtxv[vtxc++]=(struct fmn_draw_rect){0,zy,fullw,fullh-zy,bgcolor};
    fmn_draw_rect(vtxv,vtxc);
  }
  
  // And finally, draw the circle part as a texture.
  {
    int16_t srcw=128,srch=128;//TODO dimensions of image 17. Is there a reasonable way we could fetch these dynamically?
    struct fmn_draw_recal vtx={
      .dstx=ax,.dsty=ay,.dstw=zx-ax,.dsth=zy-ay,
      .srcx=0,.srcy=0,.srcw=srcw,.srch=srch,
      .pixel=bgcolor,
    };
    fmn_draw_recal(&vtx,1,17);
  }
}

/* Apply transition.
 */
 
void fmn_transition_apply(
  int transition,
  int p,int c,
  int imageid_from,int imageid_to
) {
  if (p<=0) {
    fmn_transition_edge(imageid_from);
    return;
  }
  if (p>=c) {
    fmn_transition_edge(imageid_to);
    return;
  }
  switch (transition) {
    case FMN_TRANSITION_PAN_LEFT: fmn_render_pan(p,c,imageid_from,imageid_to,-1,0); return;
    case FMN_TRANSITION_PAN_RIGHT: fmn_render_pan(p,c,imageid_from,imageid_to,1,0); return;
    case FMN_TRANSITION_PAN_UP: fmn_render_pan(p,c,imageid_from,imageid_to,0,-1); return;
    case FMN_TRANSITION_PAN_DOWN: fmn_render_pan(p,c,imageid_from,imageid_to,0,1); return;
    case FMN_TRANSITION_FADE_BLACK: fmn_render_fade2(p,c,imageid_from,imageid_to); return;
    case FMN_TRANSITION_DOOR: fmn_render_spotlight(p,c,imageid_from,imageid_to); return;
    case FMN_TRANSITION_WARP: fmn_render_fade2(p,c,imageid_from,imageid_to); return;
      break;
  }
  fmn_transition_edge(imageid_to);
}

/* Get hero offset, relevant to PAN transitions only.
 */
 
void fmn_transition_get_hero_offset(int16_t *x,int16_t *y) {
  *x=*y=0;
  const int p=fmn_render_global.transitionp;
  const int c=fmn_render_global.transitionc;
  if (p>=c) return;
  const int w=fmn_render_global.fbw;
  const int h=fmn_render_global.fbh;
  switch (fmn_render_global.transition) {
    case FMN_TRANSITION_PAN_LEFT: *x=(p*w)/c-w; break;
    case FMN_TRANSITION_PAN_RIGHT: *x=w-(p*w)/c; break;
    case FMN_TRANSITION_PAN_UP: *y=(p*h)/c-h; break;
    case FMN_TRANSITION_PAN_DOWN: *y=h-(p*h)/c; break;
  }
}

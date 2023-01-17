#include "fmn_sprite.h"

/* Global registry.
 * This is not exposed to the platform.
 * I'm going out on a limb to assume that we'll be able to used a fixed size for all sprites.
 * The only one where that's clearly inadequate is the hero, and there's just one of her, most of her state is external.
 * I feel there's some value in being able to avoid memory allocation.
 */
 
// On my NUC with Linux Chrome, we redline somewhere between 5k and 10k, with trivial sprite logic.
// Same setup but disable rendering, it's more like 6% CPU. Even with 50k sprites, only like 15% CPU.
#define FMN_SPRITE_LIMIT 64

/* The two lists both run to FMN_SPRITE_LIMIT but they are not necessarily parallel.
 * (fmn_spritev) may be sparse, with free slots indicated by (style==0) ie FMN_SPRITE_STYLE_HIDDEN.
 * (fmn_spritepv) is always packed. This is what we assign to (fmn_global.spritev).
 */
static struct fmn_sprite fmn_spritev[FMN_SPRITE_LIMIT]={0};
static struct fmn_sprite *fmn_spritepv[FMN_SPRITE_LIMIT];

/* Clear sprites.
 */
 
void fmn_sprites_clear() {
  struct fmn_sprite *sprite=fmn_spritev;
  int i=FMN_SPRITE_LIMIT;
  for (;i-->0;sprite++) sprite->style=FMN_SPRITE_STYLE_HIDDEN;
  fmn_global.spritec=0;
  fmn_global.spritev=(struct fmn_sprite_header**)fmn_spritepv;
}

/* Allocate sprite.
 * This doesn't touch the sprite. (NB it is still "available" after we return).
 */
 
static struct fmn_sprite *fmn_sprites_find_available() {
  struct fmn_sprite *sprite=fmn_spritev;
  uint32_t i=FMN_SPRITE_LIMIT;
  for (;i-->0;sprite++) {
    if (!sprite->style) return sprite;
  }
  return 0;
}

//XXX
static void XXX_update_bouncer(struct fmn_sprite *sprite,float elapsed) {
  sprite->x+=sprite->fv[0]*elapsed;
  sprite->y+=sprite->fv[1]*elapsed;
  if ((sprite->x<0.0f)&&(sprite->fv[0]<0.0f)) sprite->fv[0]=-sprite->fv[0];
  else if ((sprite->x>FMN_COLC)&&(sprite->fv[0]>0.0f)) sprite->fv[0]=-sprite->fv[0];
  if ((sprite->y<0.0f)&&(sprite->fv[1]<0.0f)) sprite->fv[1]=-sprite->fv[1];
  else if ((sprite->y>FMN_ROWC)&&(sprite->fv[1]>0.0f)) sprite->fv[1]=-sprite->fv[1];
}

/* Spawn sprite.
 */

struct fmn_sprite *fmn_sprite_spawn(
  float x,float y,
  uint16_t spriteid,
  const uint8_t *argv,uint8_t argc
) {
  if (fmn_global.spritec>=FMN_SPRITE_LIMIT) return 0;
  struct fmn_sprite *sprite=fmn_sprites_find_available();
  if (!sprite) return 0; // !!! really shouldn't happen, if spritec<FMN_SPRITE_LIMIT
  fmn_spritepv[fmn_global.spritec++]=sprite;
  
  memset(sprite,0,sizeof(struct fmn_sprite));
  sprite->x=x;
  sprite->y=y;
  sprite->spriteid=spriteid;
  if (argc>=FMN_SPRITE_ARGV_SIZE) memcpy(sprite->argv,argv,FMN_SPRITE_ARGV_SIZE);
  else memcpy(sprite->argv,argv,argc);
  //TODO:
  sprite->style=FMN_SPRITE_STYLE_TILE;
  sprite->imageid=1;
  sprite->tileid=0x33;
  sprite->xform=0;
  sprite->update=XXX_update_bouncer;
  sprite->fv[0]=((rand()&0xffff)*10)/65535.0f-5.0f;
  sprite->fv[1]=((rand()&0xffff)*10)/65535.0f-5.0f;
  
  return sprite;
}

/* Iterate sprites.
 */
 
int fmn_sprites_for_each(int (*cb)(struct fmn_sprite *sprite,void *userdata),void *userdata) {
  struct fmn_sprite **p=fmn_spritepv;
  int i=fmn_global.spritec;
  for (;i-->0;p++) {
    struct fmn_sprite *sprite=*p;
    int err=cb(sprite,userdata);
    if (err) return err;
  }
  return 0;
}

/* Update physics.
 * Velocity has already been decayed and applied.
 * Check for collisions and resolve them.
 */
 
static void fmn_sprite_physics_update(float elapsed) {
  struct fmn_sprite **ap=fmn_spritepv;
  int ai=0; for (;ai<fmn_global.spritec;ai++,ap++) {
    struct fmn_sprite *a=*ap;
    if (a->physics_mode!=FMN_SPRITE_PHYSICS_FULL) continue;
    if (a->radius<=0.0f) continue;
    
    //TODO Check (a) against static geometry.
    
    struct fmn_sprite **bp=fmn_spritepv;
    int bi=0; for (;bi<ai;bi++,bp++) {
      struct fmn_sprite *b=*bp;
      if (b->physics_mode!=FMN_SPRITE_PHYSICS_FULL) continue;
      if (b->radius<=0.0f) continue;
      
      //TODO Check (a) against (b).
    }
  }
}

/* Update all sprites.
 */
 
void fmn_sprites_update(float elapsed) {
  struct fmn_sprite **p=fmn_spritepv;
  int i=fmn_global.spritec;
  for (;i-->0;p++) {
    struct fmn_sprite *sprite=*p;
    
    if (sprite->physics_mode==FMN_SPRITE_PHYSICS_FULL) {
      // Linear decay of velocity.
      if (sprite->velx<0.0f) {
        if ((sprite->velx+=sprite->veldecay*elapsed)>=0.0f) sprite->velx=0.0f;
      } else if (sprite->velx>0.0f) {
        if ((sprite->velx-=sprite->veldecay*elapsed)<=0.0f) sprite->velx=0.0f;
      }
      if (sprite->vely<0.0f) {
        if ((sprite->vely+=sprite->veldecay*elapsed)>=0.0f) sprite->vely=0.0f;
      } else if (sprite->vely>0.0f) {
        if ((sprite->vely-=sprite->veldecay*elapsed)<=0.0f) sprite->vely=0.0f;
      }
      // Apply velocity.
      sprite->x+=sprite->velx*elapsed;
      sprite->y+=sprite->vely*elapsed;
    }
    
    if (sprite->update) {
      sprite->update(sprite,elapsed);
    }
  }
  fmn_sprite_physics_update(elapsed);
}

/* Apply force.
 */
 
void fmn_sprite_apply_force(struct fmn_sprite *sprite,float dx,float dy) {
  if (!sprite) return;
  if (sprite->physics_mode!=FMN_SPRITE_PHYSICS_FULL) return;
  sprite->velx+=dx;
  sprite->vely+=dy;
}

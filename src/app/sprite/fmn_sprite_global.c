#include "fmn_sprite.h"
#include "fmn_physics.h"

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

/* After applying commands, look up the controller.
 */
 
const struct fmn_sprite_controller *fmn_sprite_controller_by_id(uint16_t id) {
  switch (id) {
    #define _(tag) case FMN_SPRCTL_##tag: return &fmn_sprite_controller_##tag;
    FMN_FOR_EACH_SPRCTL
    #undef _
  }
  return 0;
}
 
static void fmn_sprite_apply_controller(struct fmn_sprite *sprite) {
  const struct fmn_sprite_controller *sprctl=fmn_sprite_controller_by_id(sprite->controller);
  if (!sprctl) return;
  sprite->update=sprctl->update;
  sprite->pressure=sprctl->pressure;
  if (sprctl->init) sprctl->init(sprite);
}

/* Apply command to new sprite.
 */
 
static void fmn_sprite_apply_command(struct fmn_sprite *sprite,uint8_t command,const uint8_t *v,uint8_t c) {
  switch (command) {
    case 0x20: sprite->imageid=v[0]; break;
    case 0x21: sprite->tileid=v[0]; break;
    case 0x22: sprite->xform=v[0]; break;
    case 0x23: sprite->style=v[0]; break;
    case 0x24: sprite->physics=v[0]; break;
    case 0x25: sprite->invmass=v[0]; break;
    case 0x40: sprite->veldecay=v[0]+v[1]/256.0f; break;
    case 0x41: sprite->radius=v[0]+v[1]/256.0f; break;
    case 0x42: sprite->controller=(v[0]<<8)|v[1]; break;
  }
}

/* Spawn sprite.
 */

struct fmn_sprite *fmn_sprite_spawn(
  float x,float y,
  uint16_t spriteid,
  const uint8_t *cmdv,uint16_t cmdc,
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
  sprite->style=FMN_SPRITE_STYLE_TILE;
  
  uint16_t cmdp=0; while (cmdp<cmdc) {
    uint8_t lead=cmdv[cmdp++];
    if (!lead) break; // EOF
    int paylen=0;
         if (lead<0x20) paylen=0;
    else if (lead<0x40) paylen=1;
    else if (lead<0x60) paylen=2;
    else if (lead<0x80) paylen=3;
    else if (lead<0xa0) paylen=4;
    else if (lead<0xd0) {
      if (cmdp>=cmdc) break;
      paylen=cmdv[cmdp++];
    } else break; // 0xd0..0xff have explicit unknown lengths; none yet defined
    if (cmdp>cmdc-paylen) break;
    fmn_sprite_apply_command(sprite,lead,cmdv+cmdp,paylen);
    cmdp+=paylen;
  }
  
  fmn_sprite_apply_controller(sprite);
  
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
 
/* TODO Collision detection is definitely imperfect.
 * We check one feature at a time and resolve it immediately.
 * Where two or more collisions exist, we don't correct intelligently for the combined collision.
 * Does it matter? Push this hard, make sure it at least behaves coherently when the situation gets weird.
 */
 
static void fmn_sprite_physics_update(float elapsed) {
  const uint8_t any_physics=FMN_PHYSICS_EDGE|FMN_PHYSICS_SPRITES|FMN_PHYSICS_GRID;
  struct fmn_sprite **ap=fmn_spritepv;
  int ai=0; for (;ai<fmn_global.spritec;ai++,ap++) {
    struct fmn_sprite *a=*ap;
    if (!(a->physics&any_physics)) continue;
    if (a->radius<=0.0f) continue;
    
    float cx,cy;
    if ((a->physics&FMN_PHYSICS_EDGE)&&fmn_physics_check_edges(&cx,&cy,a)) {
      a->x+=cx;
      a->y+=cy;
    }
    if ((a->physics&FMN_PHYSICS_GRID)&&fmn_physics_check_grid(&cx,&cy,a,a->physics)) {
      a->x+=cx;
      a->y+=cy;
    }
    
    if (a->physics&FMN_PHYSICS_SPRITES) {
      struct fmn_sprite **bp=fmn_spritepv;
      int bi=0; for (;bi<ai;bi++,bp++) {
        struct fmn_sprite *b=*bp;
        if (!(b->physics&FMN_PHYSICS_SPRITES)) continue;
        if (b->radius<=0.0f) continue;
      
        if (!fmn_physics_check_sprites(&cx,&cy,a,b)) continue;
        int msum=a->invmass+b->invmass;
        float aweight;
        if (msum<1) aweight=0.5f; // panic, both infinite-mass! divide equally.
        else if (!a->invmass) aweight=0.0f;
        else if (!b->invmass) aweight=1.0f;
        else aweight=(float)a->invmass/(float)msum;
        float acx=cx*aweight;
        float acy=cy*aweight;
        float bcx=acx-cx;
        float bcy=acy-cy;
        a->x+=acx;
        a->y+=acy;
        b->x+=bcx;
        b->y+=bcy;
        
        uint8_t dir=fmn_dir_from_vector_cardinal(cx,cy);
        
        /* Extra mitigation for some round-off error.
         * Unfortunately, it matters. Maybe wouldn't be a problem if we used double instead of float? But not going there.
         * Whichever sprite is lighter, clamp its position in the more significant axis to the other sprite's edge.
         * If they weigh the same, no correction, and possibly some jitter.
         */
        struct fmn_sprite *lighter=0,*heavier=0;
             if (a->invmass>b->invmass) { lighter=a; heavier=b; }
        else if (a->invmass<b->invmass) { lighter=b; heavier=a; }
        if (dir&&lighter) {
          float dstx=lighter->x,dsty=lighter->y;
          switch (dir) {
            case FMN_DIR_N: dsty=heavier->y-heavier->radius-lighter->radius; break;
            case FMN_DIR_S: dsty=heavier->y+heavier->radius+lighter->radius; break;
            case FMN_DIR_W: dstx=heavier->x-heavier->radius-lighter->radius; break;
            case FMN_DIR_E: dstx=heavier->x+heavier->radius+lighter->radius; break;
          }
          lighter->x=dstx;
          lighter->y=dsty;
        }

        if (a->pressure) {
          a->pressure(a,b,dir);
        }
        if (b->pressure) {
          b->pressure(b,a,fmn_dir_reverse(dir));
        }
      }
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
    
    if (sprite->physics&FMN_PHYSICS_MOTION) {
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
  sprite->velx+=dx;
  sprite->vely+=dy;
}

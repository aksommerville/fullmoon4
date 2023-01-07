#include "fmn_platform.h"

#define ELAPSED_TIME_LIMIT 1000

struct my_sprite {
  struct fmn_sprite hdr;
  float dx,dy;
};

struct fmn_app_model fmn_app_model={0};
static struct fmn_scene my_scene;
static struct my_sprite my_sprites[50]; // i don't think we should exceed 100
static struct fmn_sprite *my_sprite_list[50];
static int last_update_time=0;

static void fmn_populate_my_scene(struct fmn_scene *scene) {
  // TODO Data storage is a platform concern, so I think there should be a platform call here. fmn_load_scene() or something.
  memset(scene,0,sizeof(struct fmn_scene));
  scene->maptsid=122;
  scene->songid=2;
  
  uint8_t x,y;
  for (x=0;x<FMN_COLC;x++) {
    scene->map[x]=0x01;
    scene->map[(FMN_ROWC-1)*FMN_COLC+x]=0x01;
  }
  for (y=0;y<FMN_ROWC;y++) {
    scene->map[y*FMN_COLC]=0x01;
    scene->map[y*FMN_COLC+FMN_COLC-1]=0x01;
  }
  
  float topspeed=4.000f; // m/s per axis
  scene->spritev=my_sprite_list;
  scene->spritec=sizeof(my_sprite_list)/sizeof(void*);
  struct my_sprite *sprite=my_sprites;
  int i=0; for (;i<scene->spritec;i++,sprite++) {
    sprite->hdr.x=(rand()%200)/10.0f;
    sprite->hdr.y=(rand()%120)/10.0f;
    sprite->dx=((rand()%1000-500)*topspeed)/500.0f;
    sprite->dy=((rand()%1000-500)*topspeed)/500.0f;
    my_sprite_list[i]=(struct fmn_sprite*)sprite;
    sprite->hdr.imageid=122;
    sprite->hdr.tileid=0x02;
    sprite->hdr.xform=0;
  }
}

int fmn_init() {
  fmn_populate_my_scene(&my_scene);
  fmn_set_scene(&my_scene,FMN_TRANSITION_CUT);
  return 0;
}

void fmn_update(int timems,int input) {
  if (!last_update_time) {
    // First update.
  } else {
    int elapsedms=timems-last_update_time;
    if ((elapsedms>ELAPSED_TIME_LIMIT)||(elapsedms<1)) {
      fmn_log("Clock fault! last_time=%d time=%d elapsed=%d",last_update_time,timems,elapsedms);
      elapsedms=1;
    }
    // Ongoing update.
    float elapsed=elapsedms/1000.0f;
    struct my_sprite *sprite=my_sprites;
    int i=sizeof(my_sprites)/sizeof(struct my_sprite);
    for (;i-->0;sprite++) {
      sprite->hdr.x+=sprite->dx*elapsed;
      if ((sprite->hdr.x<0.0f)&&(sprite->dx<0.0f)) sprite->dx=-sprite->dx;
      else if ((sprite->hdr.x>FMN_COLC)&&(sprite->dx>0.0f)) sprite->dx=-sprite->dx;
      sprite->hdr.y+=sprite->dy*elapsed;
      if ((sprite->hdr.y<0.0f)&&(sprite->dy<0.0f)) sprite->dy=-sprite->dy;
      else if ((sprite->hdr.y>FMN_ROWC)&&(sprite->dy>0.0f)) sprite->dy=-sprite->dy;
    }
  }
  last_update_time=timems;
}

/* inmgr_XXX_add_hardcoded_maps.c
 * A highly temporary cudgel to hard-code mapping for my joysticks.
 * Eventually, there will be rules templates acquired from a config file.
 * Since that's not absolutely necessary for presenting at GDEX 2023, and I'm under the gun to finish in time, well, this ugly mess.
 */

#include "inmgr_internal.h"
#include "app/fmn_platform.h"

#define BTN(id,tag) {.srcbtnid=id,.srclo=1,.srchi=INT_MAX,.mode=INMGR_MAP_MODE_TWOSTATE,.dstbtnid=FMN_INPUT_##tag},
#define HORZ1(id) {.srcbtnid=id,.srclo=-1,.srchi=1,.mode=INMGR_MAP_MODE_THREESTATE,.dstbtnid=FMN_INPUT_LEFT|FMN_INPUT_RIGHT},
#define VERT1(id) {.srcbtnid=id,.srclo=-1,.srchi=1,.mode=INMGR_MAP_MODE_THREESTATE,.dstbtnid=FMN_INPUT_UP|FMN_INPUT_DOWN},
#define HORZ15(id) {.srcbtnid=id,.srclo=-10000,.srchi=10000,.mode=INMGR_MAP_MODE_THREESTATE,.dstbtnid=FMN_INPUT_LEFT|FMN_INPUT_RIGHT},
#define VERT15(id) {.srcbtnid=id,.srclo=-10000,.srchi=10000,.mode=INMGR_MAP_MODE_THREESTATE,.dstbtnid=FMN_INPUT_UP|FMN_INPUT_DOWN},

static const struct inmgr_map inmgr_map_ps2_knockoff[]={ // ok
  BTN(0x00010122,USE)
  BTN(0x00010123,MENU)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_zelda[]={ // ok
  BTN(0x00010130,MENU)
  BTN(0x00010131,USE)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_sn30[]={ // ok (SN30 and Xbox 360)
  BTN(0x00010130,USE)
  BTN(0x00010133,MENU)
  HORZ15(0x00030000)
  VERT15(0x00030001)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_atari_stick[]={ // ok
  BTN(0x00010130,USE)
  BTN(0x00010131,MENU)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_atari_gamepad[]={ // ok
  BTN(0x00010130,USE)
  BTN(0x00010133,MENU)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_black_on_black[]={ // ok
  BTN(0x00010130,MENU)
  BTN(0x00010131,USE)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_xbox[]={ // ok
  BTN(0x00010130,USE)
  BTN(0x00010133,MENU)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_pro2[]={ // ok
  BTN(0x00010131,USE)
  BTN(0x00010134,MENU)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static int select_map_source(void *dstpp,const struct inmgr_device *device) {
  #define BYID(v,p,tag) if ((device->vid==v)&&(device->pid==p)) { *(const void**)dstpp=inmgr_map_##tag; return sizeof(inmgr_map_##tag)/sizeof(struct inmgr_map); }

  BYID(0x0e8f,0x0003,ps2_knockoff)
  BYID(0x20d6,0xa711,zelda)
  BYID(0x045e,0x028e,sn30) // Both SN30 and Xbox 360
  BYID(0x3250,0x1001,atari_stick)
  BYID(0x3250,0x1002,atari_gamepad)
  BYID(0x20d6,0xca6d,black_on_black)
  BYID(0x045e,0x0289,xbox)
  BYID(0x2dc8,0x3010,pro2) // mind you need to hold B+Start while connecting

  #undef BYID
  return 0;
}

void inmgr_XXX_add_hardcoded_maps(struct inmgr_device *device) {
  const struct inmgr_map *mapv=0;
  int mapc=select_map_source(&mapv,device);
  if (mapc>0) {
    if (device->mapv) free(device->mapv);
    if (!(device->mapv=malloc(sizeof(struct inmgr_map)*mapc))) {
      device->mapc=device->mapa=0;
      return;
    }
    memcpy(device->mapv,mapv,sizeof(struct inmgr_map)*mapc);
    device->mapc=device->mapa=mapc;
    struct inmgr_map *map=device->mapv;
    int i=device->mapc;
    for (;i-->0;map++) {
      map->srcdevid=device->devid;
      map->dstplayerid=1;
    }
  }
}

/* inmgr_XXX_add_hardcoded_maps.c
 * A highly temporary cudgel to hard-code mapping for my joysticks.
 * Eventually, there will be rules templates acquired from a config file.
 * Since that's not absolutely necessary for presenting at GDEX 2023, and I'm under the gun to finish in time, well, this ugly mess.
 */

#include "inmgr_internal.h"
#include "app/fmn_platform.h"
#include "opt/bigpc/bigpc_internal.h"

#define BTN(id,tag) {.srcbtnid=id,.srclo=1,.srchi=INT_MAX,.mode=INMGR_MAP_MODE_TWOSTATE,.dstbtnid=FMN_INPUT_##tag},
#define BTNA(id,tag) {.srcbtnid=id,.srclo=1,.srchi=INT_MAX,.mode=INMGR_MAP_MODE_ACTION,.dstbtnid=BIGPC_ACTIONID_##tag},
#define HORZ1(id) {.srcbtnid=id,.srclo=-1,.srchi=1,.mode=INMGR_MAP_MODE_THREESTATE,.dstbtnid=FMN_INPUT_LEFT|FMN_INPUT_RIGHT},
#define VERT1(id) {.srcbtnid=id,.srclo=-1,.srchi=1,.mode=INMGR_MAP_MODE_THREESTATE,.dstbtnid=FMN_INPUT_UP|FMN_INPUT_DOWN},
#define HORZU8(id) {.srcbtnid=id,.srclo=64,.srchi=192,.mode=INMGR_MAP_MODE_THREESTATE,.dstbtnid=FMN_INPUT_LEFT|FMN_INPUT_RIGHT},
#define VERTU8(id) {.srcbtnid=id,.srclo=64,.srchi=192,.mode=INMGR_MAP_MODE_THREESTATE,.dstbtnid=FMN_INPUT_UP|FMN_INPUT_DOWN},
#define HORZ15(id) {.srcbtnid=id,.srclo=-10000,.srchi=10000,.mode=INMGR_MAP_MODE_THREESTATE,.dstbtnid=FMN_INPUT_LEFT|FMN_INPUT_RIGHT},
#define VERT15(id) {.srcbtnid=id,.srclo=-10000,.srchi=10000,.mode=INMGR_MAP_MODE_THREESTATE,.dstbtnid=FMN_INPUT_UP|FMN_INPUT_DOWN},
#define HAT(id,lo) {.srcbtnid=id,.srclo=lo,.srchi=(lo)+7,.mode=INMGR_MAP_MODE_HAT,.dstbtnid=FMN_INPUT_LEFT|FMN_INPUT_RIGHT|FMN_INPUT_UP|FMN_INPUT_DOWN,.srcvalue=(lo)-1},

#if FMN_USE_machid

static const struct inmgr_map inmgr_map_ps2_knockoff[]={
  BTN(6,USE)
  BTN(7,MENU)
  HORZU8(19)
  VERTU8(20)
  HAT(22,0)
};

static const struct inmgr_map inmgr_map_zelda[]={
  BTN(2,MENU)
  BTN(3,USE)
  HAT(16,0)
  // Something is broken, this stick horz jumps low unpredictably.
  //HORZU8(17)
  //VERTU8(18)
};

static const struct inmgr_map inmgr_map_sn30[]={ // SN30 and Xbox 360
};

static const struct inmgr_map inmgr_map_atari_stick[]={  // shows up but events never report
  BTN(5,USE)
  BTN(6,MENU)
  HAT(12,1)
};

static const struct inmgr_map inmgr_map_atari_gamepad[]={
  BTN(5,USE)
  BTN(7,MENU)
  HAT(18,1)
  HORZ15(19)
  VERT15(20)
};

static const struct inmgr_map inmgr_map_black_on_black[]={
  BTN(2,MENU)
  BTN(3,USE)
  HAT(15,0)
  // Similar problems to Zelda. The hell?
  //HORZU8(16)
  //VERTU8(17)
};

static const struct inmgr_map inmgr_map_xbox[]={
};

static const struct inmgr_map inmgr_map_pro2[]={
  BTN(3,USE)
  BTN(6,MENU)
  HAT(17,0)
  HORZU8(18)
  VERTU8(19)
};

static const struct inmgr_map inmgr_map_logikey[]={
};

#elif FMN_USE_evdev

static const struct inmgr_map inmgr_map_ps2_knockoff[]={
  BTN(0x00010122,USE)
  BTN(0x00010123,MENU)
  BTNA(0x00010129,quit)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_zelda[]={
  BTN(0x00010130,MENU)
  BTN(0x00010131,USE)
		BTNA(0x00010139,quit)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_sn30[]={ // SN30 and Xbox 360
  BTN(0x00010130,USE)
  BTN(0x00010133,MENU)
  BTNA(0x0001013a,quit) // select
  //BTNA(0x0001013e,screencap)
  HORZ15(0x00030000)
  VERT15(0x00030001)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_atari_stick[]={
  BTNA(0x000100ac,quit)
  BTN(0x00010130,USE)
  BTN(0x00010131,MENU)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_atari_gamepad[]={
  BTNA(0x0001008b,quit) // start
  BTN(0x00010130,USE)
  BTN(0x00010133,MENU)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_black_on_black[]={
  BTN(0x00010130,MENU)
  BTN(0x00010131,USE)
		BTNA(0x00010139,quit)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_xbox[]={
  BTN(0x00010130,USE)
  BTN(0x00010134,MENU)
		BTNA(0x0001013b,quit)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_pro2[]={
  BTN(0x00010131,USE)
  BTN(0x00010134,MENU)
		BTNA(0x0001013b,quit)
  HORZ1(0x00030010)
  VERT1(0x00030011)
};

static const struct inmgr_map inmgr_map_logikey[]={ // logitech keyboard i use with the pi. NB linux key codes, not USB-HID!
  BTNA(0x00010001,quit) // esc
  BTN(0x0001002c,USE) // z
  BTN(0x0001002d,MENU) // x
  BTN(0x00010067,UP) // arrows...
  BTN(0x00010069,LEFT)
  BTN(0x0001006a,RIGHT)
  BTN(0x0001006c,DOWN)
};

#endif

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
  BYID(0x046d,0xc31c,logikey)

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

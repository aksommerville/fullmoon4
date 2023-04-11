#include "fmn_platform.h"
#include "fmn_game.h"

#define ELAPSED_TIME_LIMIT 1000

struct fmn_global fmn_global={0};
static uint32_t last_update_time=0;
static uint8_t pvinput=0;


int fmn_init() {
  last_update_time=0;
  if (fmn_game_init()<0) return -1;
  return 0;
}

void fmn_update(uint32_t timems,uint8_t input) {

  uint32_t elapsedms=timems-last_update_time;
  if ((elapsedms>ELAPSED_TIME_LIMIT)||(elapsedms<1)) {
    fmn_log("Clock fault! last_time=%d time=%d elapsed=%d",last_update_time,timems,elapsedms);
    elapsedms=1;
  }
  last_update_time=timems;
  float elapsed=elapsedms/1000.0f;
  
  if (input!=pvinput) {
    #define BIT(tag) if ((input&FMN_INPUT_##tag)&&!(pvinput&FMN_INPUT_##tag)) fmn_game_input(FMN_INPUT_##tag,1,input); \
    else if (!(input&FMN_INPUT_##tag)&&(pvinput&FMN_INPUT_##tag)) fmn_game_input(FMN_INPUT_##tag,0,input);
    BIT(LEFT)
    BIT(RIGHT)
    BIT(UP)
    BIT(DOWN)
    BIT(USE)
    BIT(MENU)
    #undef BIT
    pvinput=input;
  }
  
  fmn_game_update(elapsed);
}

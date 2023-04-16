#include "fmn_datafile_internal.h"
#include "app/fmn_platform.h"

/* Iterate map commands.
 */

int fmn_map_for_each_command(
  const void *v,int c,
  int (*cb)(uint8_t opcode,const uint8_t *argv,int argc,void *userdata),
  void *userdata
) {
  const uint8_t *V=v;
  int p=FMN_COLC*FMN_ROWC;
  while (p<c) {
    uint8_t opcode=V[p++];
    int paylen,err;
    switch (opcode&0xe0) {
      case 0x00: paylen=0; break;
      case 0x20: paylen=1; break;
      case 0x40: paylen=2; break;
      case 0x60: paylen=4; break;
      case 0x80: paylen=6; break;
      case 0xa0: paylen=8; break;
      case 0xc0: if (p>=c) paylen=opcode=0; else paylen=V[p++]; break;
      default: paylen=opcode=0;
    }
    if (!opcode) break;
    if (p>c-paylen) break;
    const uint8_t *arg=V+p;
    p+=paylen;
    if (err=cb(opcode,arg,paylen,userdata)) return err;
  }
  return 0;
}

/* Map command location.
 */

int fmn_map_location_for_command(int16_t *xy,uint8_t opcode,const uint8_t *argv,int argc) {
  switch (opcode) {
  
    // NEIGHBOR. I wouldn't expect a caller to ask for these, but ok, return the middle of the neighbor screen.
    case 0x40: xy[0]=-(FMN_COLC>>1); xy[1]=FMN_ROWC>>1; return 1;
    case 0x41: xy[0]=FMN_COLC+(FMN_COLC>>1); xy[1]=FMN_ROWC>>1; return 1;
    case 0x42: xy[0]=FMN_COLC>>1; xy[1]=-(FMN_ROWC>>1); return 1;
    case 0x43: xy[0]=FMN_COLC>>1; xy[1]=FMN_ROWC+(FMN_ROWC>>1); return 1;
    
    // Everything else with a location, it's packed in the first byte.
    case 0x22: // HERO
    case 0x44: // TRANSMOGRIFY
    case 0x60: // DOOR
    case 0x61: // SKETCH
    case 0x80: // SPRITE
        if (argc>=1) {
          xy[0]=argv[0]%FMN_COLC;
          xy[1]=argv[0]/FMN_COLC;
          return 1;
        } break;
  }
  // For everything else, we'll say "zero; location not applicable", but also populate (xy) with the center of the map.
  xy[0]=FMN_COLC>>1;
  xy[1]=FMN_ROWC>>1;
  return 0;
}

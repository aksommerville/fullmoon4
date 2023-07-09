#include "datan_internal.h"

/* Delete.
 */
 
void datan_sprite_del(struct datan_sprite *sprite) {
  if (!sprite) return;
  free(sprite);
}

/* New.
 */
 
struct datan_sprite *datan_sprite_new(uint16_t qualifier,uint32_t id,const void *src,int srcc) {
  if (srcc<0) return 0;
  struct datan_sprite *sprite=calloc(1,sizeof(struct datan_sprite));
  if (!sprite) return 0;
  sprite->qualifier=qualifier;
  sprite->id=id;
  sprite->addl=src;
  sprite->addlc=srcc;
  return sprite;
}

/* Validate.
 */
 
static int datan_sprite_validate_1(uint8_t opcode,const uint8_t *v,int c,void *userdata) {
  struct datan_sprite *sprite=userdata;
  switch (opcode) {
    #define ONCE8(fldname) { \
      if (sprite->fldname) { \
        fprintf(stderr,"%s:sprite:%d(%d): Multiple values for field %s (%d,%d)\n",datan.arpath,sprite->id,sprite->qualifier,#fldname,sprite->fldname,v[0]); \
        return -2; \
      } \
      sprite->fldname=v[0]; \
    }
    #define ONCE16(fldname) { \
      if (sprite->fldname) { \
        fprintf(stderr,"%s:sprite:%d(%d): Multiple values for field %s (%d,%d)\n",datan.arpath,sprite->id,sprite->qualifier,#fldname,sprite->fldname,(v[0]<<8)|v[1]); \
        return -2; \
      } \
      sprite->fldname=(v[0]<<8)|v[1]; \
    }
    case 0x20: ONCE8(imageid) break;
    case 0x21: ONCE8(tileid) break;
    case 0x22: ONCE8(xform) break;
    case 0x23: ONCE8(style) break;
    case 0x24: ONCE8(physics) break;
    case 0x25: ONCE8(invmass) break;
    case 0x26: ONCE8(layer) break;
    case 0x40: ONCE16(veldecay) break;
    case 0x41: ONCE16(radius) break;
    case 0x42: ONCE16(controller) break;
    case 0x43: { // bv
        if (v[0]>=FMN_SPRITE_BV_SIZE) {
          fprintf(stderr,"%s:sprite:%d(%d):WARNING: bv[%d]=0x%02x, but bv size is %d.\n",datan.arpath,sprite->id,sprite->qualifier,v[0],v[1],FMN_SPRITE_BV_SIZE);
        } else {
          sprite->bv[v[0]]=v[1];
        }
      } break;
    #undef ONCE8
    #undef ONCE16
    default: fprintf(stderr,"%s:sprite:%d(%d):WARNING: Unknown opcode 0x%02x.\n",datan.arpath,sprite->id,sprite->qualifier,opcode); break;
  }
  return 0;
}
 
int datan_sprite_validate(struct datan_sprite *sprite) {
  int err=datan_sprite_for_each_command(sprite,datan_sprite_validate_1,sprite);
  if (err<0) return err;
  
  // Opportunity for controller-specific validation here. Single object only.
  
  return 0;
}

/* Iterate commands.
 */
 
int datan_sprite_for_each_command(
  struct datan_sprite *sprite,
  int (*cb)(uint8_t opcode,const uint8_t *v,int c,void *userdata),
  void *userdata
) {
  int addlp=0,err;
  while (addlp<sprite->addlc) {
    uint8_t opcode=sprite->addl[addlp++];
    if (!opcode) break; // Explicit EOF.
    int paylen;
         if (opcode<0x20) paylen=0;
    else if (opcode<0x40) paylen=1;
    else if (opcode<0x60) paylen=2;
    else if (opcode<0x80) paylen=3;
    else if (opcode<0xa0) paylen=4;
    else if (opcode<0xd0) {
      if (addlp>=sprite->addlc) {
        fprintf(stderr,"%s:sprite:%d(%d): Expected length for opcode 0x%02x, found EOF.\n",datan.arpath,sprite->id,sprite->qualifier,opcode);
        return -2;
      }
      paylen=sprite->addl[addlp++];
    } else {
      fprintf(stderr,"%s:sprite:%d(%d): Unknown opcode 0x%02x at %d/%d\n",datan.arpath,sprite->id,sprite->qualifier,opcode,addlp-1,sprite->addlc);
      return -2;
    }
    if (addlp>sprite->addlc-paylen) {
      fprintf(stderr,"%s:sprite:%d(%d): Command 0x%02x at %d/%d overruns EOF.\n",datan.arpath,sprite->id,sprite->qualifier,opcode,addlp,sprite->addlc);
      return -2;
    }
    if (err=cb(opcode,sprite->addl+addlp,paylen,userdata)) return err;
    addlp+=paylen;
  }
  return 0;
}

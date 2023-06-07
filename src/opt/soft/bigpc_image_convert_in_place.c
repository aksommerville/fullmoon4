#include "fmn_soft_internal.h"

/* Primitives.
 */
 
typedef uint32_t (*bigpc_pixcvt_32)(uint32_t src);
typedef uint16_t (*bigpc_pixcvt_16)(uint16_t src);
typedef uint8_t (*bigpc_pixcvt_8)(uint8_t src);

static uint32_t bigpc_pixcvt_32_0123(uint32_t src) {
  return src;
}

static uint32_t bigpc_pixcvt_32_2103(uint32_t src) {
  return (src&0x00ff00ff)|((src&0xff000000)>>16)|((src&0xff00)<<16);
}

static uint32_t bigpc_pixcvt_32_1230(uint32_t src) {
  return (src<<8)|(src>>24);
}

static uint32_t bigpc_pixcvt_32_3210(uint32_t src) {
  return (src>>24)|((src&0xff0000)>>8)|((src&0xff00)<<8)|(src<<24);
}

static uint32_t bigpc_pixcvt_32_3012(uint32_t src) {
  return (src>>8)|(src<<24);
}

static uint32_t bigpc_pixcvt_32_0321(uint32_t src) {
  return (src&0xff00ff00)|((src&0xff)<<16)|((src&0xff0000)>>16);
}

static uint32_t bigpc_pixcvt_32_012x(uint32_t src) {
  return src|0xff;
}

static uint32_t bigpc_pixcvt_32_210x(uint32_t src) {
  uint8_t r=src>>24;
  uint8_t g=src>>16;
  uint8_t b=src>>8;
  return (b<<24)|(g<<16)|(r<<8)|0xff;
}

static uint32_t bigpc_pixcvt_32_123x(uint32_t src) {
  return (src<<8)|0xff;
}

static uint32_t bigpc_pixcvt_32_321x(uint32_t src) {
  uint8_t r=src>>16;
  uint8_t g=src>>8;
  uint8_t b=src;
  return (b<<24)|(g<<16)|(r<<8)|0xff;
}

static uint32_t bigpc_pixcvt_32_x012(uint32_t src) {
  return 0xff000000|(src>>8);
}

static uint32_t bigpc_pixcvt_32_x210(uint32_t src) {
  uint8_t r=src>>24;
  uint8_t g=src>>16;
  uint8_t b=src>>8;
  return 0xff000000|(b<<16)|(g<<8)|r;
}

static uint32_t bigpc_pixcvt_32_x123(uint32_t src) {
  uint8_t r=src>>16;
  uint8_t g=src>>8;
  uint8_t b=src;
  return 0xff000000|(r<<16)|(g<<8)|b;
}

static uint32_t bigpc_pixcvt_32_x321(uint32_t src) {
  uint8_t r=src>>16;
  uint8_t g=src>>8;
  uint8_t b=src;
  return 0xff000000|(b<<16)|(g<<8)|r;
}

static uint32_t bigpc_pixcvt_32_0002(uint32_t src) {
  uint8_t y=src>>24;
  uint8_t a=src;
  return (y<<24)|(y<<16)|(y<<8)|a;
}

static uint32_t bigpc_pixcvt_32_2220(uint32_t src) {
  uint8_t y=src>>8;
  return (src&0x000000ff)|(y<<24)|(y<<16)|(y<<8);
}

static uint32_t bigpc_pixcvt_32_333x(uint32_t src) {
  uint8_t y=src;
  return (y<<24)|(y<<16)|(y<<8)|0xff;
}

static uint32_t bigpc_pixcvt_32_2000(uint32_t src) {
  uint8_t a=src>>8;
  uint8_t y=src>>24;
  return (a<<24)|(y<<16)|(y<<8)|y;
}

static uint32_t bigpc_pixcvt_32_0222(uint32_t src) {
  uint8_t y=src>>8;
  return (src&0xff000000)|(y<<16)|(y<<8)|y;
}

static uint32_t bigpc_pixcvt_32_x333(uint32_t src) {
  uint8_t y=src;
  return 0xff000000|(y<<16)|(y<<8)|y;
}

static uint16_t bigpc_pixcvt_16_identity(uint16_t src) {
  return src;
}

static uint16_t bigpc_pixcvt_565_swap(uint16_t src) {
  return (src&0x07e0)|(src>>11)|(src<<11);
}

static uint16_t bigpc_pixcvt_565_5551(uint16_t src) {
  uint8_t r=(src>>11)&0x1f;
  uint8_t g=(src>>5)&0x3e; if (g&2) g|=1;
  uint8_t b=(src>>1)&0x1f;
  return (r<<11)|(g<<5)|b;
}

static uint16_t bigpc_pixcvt_565_5551_swap(uint16_t src) {
  uint8_t r=(src>>11)&0x1f;
  uint8_t g=(src>>5)&0x3e; if (g&2) g|=1;
  uint8_t b=(src>>1)&0x1f;
  return (b<<11)|(g<<5)|r;
}

static uint16_t bigpc_pixcvt_565_1555(uint16_t src) {
  uint8_t r=(src>>10)&0x1f;
  uint8_t g=(src>>4)&0x3e; if (g&2) g|=1;
  uint8_t b=src&0x1f;
  return (r<<11)|(g<<5)|b;
}

static uint16_t bigpc_pixcvt_565_1555_swap(uint16_t src) {
  uint8_t r=(src>>10)&0x1f;
  uint8_t g=(src>>4)&0x3e; if (g&2) g|=1;
  uint8_t b=src&0x1f;
  return (b<<11)|(g<<5)|r;
}

static uint16_t bigpc_pixcvt_5551_565(uint16_t src) {
  uint8_t r=src>>11;
  uint8_t g=(src>>6)&0x1f; // shift off lsb
  uint8_t b=src&0x1f;
  return (r<<11)|(g<<6)|(b<<1)|0x0001;
}

static uint16_t bigpc_pixcvt_5551_565_swap(uint16_t src) {
  uint8_t r=src>>11;
  uint8_t g=(src>>6)&0x1f; // shift off lsb
  uint8_t b=src&0x1f;
  return (b<<11)|(g<<6)|(r<<1)|0x0001;
}

static uint16_t bigpc_pixcvt_1555_565(uint16_t src) {
  uint8_t r=src>>11;
  uint8_t g=(src>>6)&0x1f; // shift off lsb
  uint8_t b=src&0x1f;
  return 0x8000|(r<<10)|(g<<5)|b;
}

static uint16_t bigpc_pixcvt_1555_565_swap(uint16_t src) {
  uint8_t r=src>>11;
  uint8_t g=(src>>6)&0x1f; // shift off lsb
  uint8_t b=src&0x1f;
  return 0x8000|(b<<10)|(g<<5)|r;
}

static uint16_t bigpc_pixcvt_5551_555x(uint16_t src) {
  return src|0x0001;
}

static uint16_t bigpc_pixcvt_5551_555x_swap(uint16_t src) {
  return (src&0x07c0)|0x0001|((src&0xf800)>>10)|((src&0x003e)<<10);
}

static uint16_t bigpc_pixcvt_5551_x555(uint16_t src) {
  uint8_t r=src>>10;
  uint8_t g=(src>>5)&0x1f;
  uint8_t b=src&0x1f;
  return (r<<11)|(g<<6)|(b<<1)|0x0001;
}

static uint16_t bigpc_pixcvt_5551_x555_swap(uint16_t src) {
  uint8_t r=src>>10;
  uint8_t g=(src>>5)&0x1f;
  uint8_t b=src&0x1f;
  return (b<<11)|(g<<6)|(r<<1)|0x0001;
}

static uint16_t bigpc_pixcvt_1555_555x(uint16_t src) {
  uint8_t r=src>>11;
  uint8_t g=(src>>6)&0x1f;
  uint8_t b=(src>>1)&0x1f;
  return 0x8000|(r<<10)|(g<<5)|b;
}

static uint16_t bigpc_pixcvt_1555_555x_swap(uint16_t src) {
  uint8_t r=src>>11;
  uint8_t g=(src>>6)&0x1f;
  uint8_t b=(src>>1)&0x1f;
  return 0x8000|(b<<10)|(g<<5)|r;
}

static uint16_t bigpc_pixcvt_1555_x555(uint16_t src) {
  return src|0x8000;
}

static uint16_t bigpc_pixcvt_1555_x555_swap(uint16_t src) {
  uint8_t r=(src>>10)&0x1f;
  uint8_t b=src&0x1f;
  return 0x8000|(src&0x03e0)|(b<<10)|r;
}

static uint16_t bigpc_pixcvt_5551_1555(uint16_t src) {
  uint8_t a=src>>15;
  uint8_t r=(src>>10)&0x1f;
  uint8_t g=(src>>5)&0x1f;
  uint8_t b=src&0x1f;
  return (r<<11)|(g<<6)|(b<<1)|a;
}

static uint16_t bigpc_pixcvt_5551_1555_swap(uint16_t src) {
  uint8_t a=src>>15;
  uint8_t r=(src>>10)&0x1f;
  uint8_t g=(src>>5)&0x1f;
  uint8_t b=src&0x1f;
  return (b<<11)|(g<<6)|(r<<1)|a;
}

static uint16_t bigpc_pixcvt_1555_5551(uint16_t src) {
  uint8_t r=src>>11;
  uint8_t g=(src>>6)&0x1f;
  uint8_t b=(src>>1)&0x1f;
  uint8_t a=src&1;
  return (a<<15)|(r<<10)|(g<<5)|b;
}

static uint16_t bigpc_pixcvt_1555_5551_swap(uint16_t src) {
  uint8_t r=src>>11;
  uint8_t g=(src>>6)&0x1f;
  uint8_t b=(src>>1)&0x1f;
  uint8_t a=src&1;
  return (a<<15)|(b<<10)|(g<<5)|r;
}

static uint16_t bigpc_pixcvt_5551_swap(uint16_t src) {
  return (src&0x07c1)|((src&0xf800)>>10)|((src&0x003e)<<10);
}

static uint16_t bigpc_pixcvt_1555_swap(uint16_t src) {
  return (src&0x803e)|((src&0x7c00)>>10)|((src&0x001f)<<10);
}

static uint16_t bigpc_pixcvt_565_YA88(uint16_t src) {
  uint8_t y=src>>11; // 5 bits; middle channel LSB will always be unset
  return (y<<11)|(y<<6)|y;
}

static uint16_t bigpc_pixcvt_565_AY88(uint16_t src) {
  uint8_t y=(src>>3)&0x1f; // 5 bits; middle channel LSB will always be unset
  return (y<<11)|(y<<6)|y;
}

static uint16_t bigpc_pixcvt_565_I16(uint16_t src) {
  uint8_t y=src>>11; // 5 bits; middle channel LSB will always be unset
  return (y<<11)|(y<<6)|y;
}

static uint16_t bigpc_pixcvt_5551_YA88(uint16_t src) {
  uint8_t a=(src>>7)&1;
  uint8_t y=src>>11;
  return (y<<11)|(y<<6)|(y<<1)|a;
}

static uint16_t bigpc_pixcvt_5551_AY88(uint16_t src) {
  uint8_t a=src>>15;
  uint8_t y=(src>>3)&0x1f;
  return (y<<11)|(y<<6)|(y<<1)|a;
}

static uint16_t bigpc_pixcvt_5551_I16(uint16_t src) {
  uint8_t y=src>>11;
  return (y<<11)|(y<<6)|(y<<1)|1;
}

static uint16_t bigpc_pixcvt_1555_YA88(uint16_t src) {
  uint8_t a=(src>>7)&1;
  uint8_t y=src>>8;
  return (a<<15)|(y<<10)|(y<<5)|y;
}

static uint16_t bigpc_pixcvt_1555_AY88(uint16_t src) {
  uint8_t a=src>>15;
  uint8_t y=src;
  return (a<<15)|(y<<10)|(y<<5)|y;
}

static uint16_t bigpc_pixcvt_1555_I16(uint16_t src) {
  uint8_t y=src>>11;
  return 0x8000|(y<<10)|(y<<5)|y;
}

static uint8_t bigpc_pixcvt_8_identity(uint8_t src) {
  return src;
}

static bigpc_pixcvt_32 bigpc_pixcvt_32_get(int dstfmt,int srcfmt) {
  switch (dstfmt) {
    case BIGPC_IMAGE_PIXFMT_RGBA: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGBA: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_BGRA: return bigpc_pixcvt_32_2103;
        case BIGPC_IMAGE_PIXFMT_ARGB: return bigpc_pixcvt_32_1230;
        case BIGPC_IMAGE_PIXFMT_ABGR: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_RGBX: return bigpc_pixcvt_32_012x;
        case BIGPC_IMAGE_PIXFMT_BGRX: return bigpc_pixcvt_32_210x;
        case BIGPC_IMAGE_PIXFMT_XRGB: return bigpc_pixcvt_32_123x;
        case BIGPC_IMAGE_PIXFMT_XBGR: return bigpc_pixcvt_32_321x;
        case BIGPC_IMAGE_PIXFMT_YA16: return bigpc_pixcvt_32_0002;
        case BIGPC_IMAGE_PIXFMT_AY16: return bigpc_pixcvt_32_2220;
        case BIGPC_IMAGE_PIXFMT_I32: return bigpc_pixcvt_32_333x;
      } break;
    case BIGPC_IMAGE_PIXFMT_BGRA: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGBA: return bigpc_pixcvt_32_2103;
        case BIGPC_IMAGE_PIXFMT_BGRA: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_ARGB: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_ABGR: return bigpc_pixcvt_32_1230;
        case BIGPC_IMAGE_PIXFMT_RGBX: return bigpc_pixcvt_32_012x;
        case BIGPC_IMAGE_PIXFMT_BGRX: return bigpc_pixcvt_32_012x;
        case BIGPC_IMAGE_PIXFMT_XRGB: return bigpc_pixcvt_32_321x;
        case BIGPC_IMAGE_PIXFMT_XBGR: return bigpc_pixcvt_32_123x;
        case BIGPC_IMAGE_PIXFMT_YA16: return bigpc_pixcvt_32_0002;
        case BIGPC_IMAGE_PIXFMT_AY16: return bigpc_pixcvt_32_2220;
        case BIGPC_IMAGE_PIXFMT_I32: return bigpc_pixcvt_32_333x;
      } break;
    case BIGPC_IMAGE_PIXFMT_ARGB: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGBA: return bigpc_pixcvt_32_3012;
        case BIGPC_IMAGE_PIXFMT_BGRA: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_ARGB: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_ABGR: return bigpc_pixcvt_32_0321;
        case BIGPC_IMAGE_PIXFMT_RGBX: return bigpc_pixcvt_32_x012;
        case BIGPC_IMAGE_PIXFMT_BGRX: return bigpc_pixcvt_32_x210;
        case BIGPC_IMAGE_PIXFMT_XRGB: return bigpc_pixcvt_32_x123;
        case BIGPC_IMAGE_PIXFMT_XBGR: return bigpc_pixcvt_32_x321;
        case BIGPC_IMAGE_PIXFMT_YA16: return bigpc_pixcvt_32_2000;
        case BIGPC_IMAGE_PIXFMT_AY16: return bigpc_pixcvt_32_0222;
        case BIGPC_IMAGE_PIXFMT_I32: return bigpc_pixcvt_32_x333;
      } break;
    case BIGPC_IMAGE_PIXFMT_ABGR: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGBA: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_BGRA: return bigpc_pixcvt_32_3012;
        case BIGPC_IMAGE_PIXFMT_ARGB: return bigpc_pixcvt_32_0321;
        case BIGPC_IMAGE_PIXFMT_ABGR: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_RGBX: return bigpc_pixcvt_32_x210;
        case BIGPC_IMAGE_PIXFMT_BGRX: return bigpc_pixcvt_32_x012;
        case BIGPC_IMAGE_PIXFMT_XRGB: return bigpc_pixcvt_32_x321;
        case BIGPC_IMAGE_PIXFMT_XBGR: return bigpc_pixcvt_32_x123;
        case BIGPC_IMAGE_PIXFMT_YA16: return bigpc_pixcvt_32_2000;
        case BIGPC_IMAGE_PIXFMT_AY16: return bigpc_pixcvt_32_0222;
        case BIGPC_IMAGE_PIXFMT_I32: return bigpc_pixcvt_32_x333;
      } break;
    case BIGPC_IMAGE_PIXFMT_RGBX: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGBA: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_BGRA: return bigpc_pixcvt_32_2103;
        case BIGPC_IMAGE_PIXFMT_ARGB: return bigpc_pixcvt_32_1230;
        case BIGPC_IMAGE_PIXFMT_ABGR: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_RGBX: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_BGRX: return bigpc_pixcvt_32_2103;
        case BIGPC_IMAGE_PIXFMT_XRGB: return bigpc_pixcvt_32_1230;
        case BIGPC_IMAGE_PIXFMT_XBGR: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_YA16: return bigpc_pixcvt_32_0002;
        case BIGPC_IMAGE_PIXFMT_AY16: return bigpc_pixcvt_32_2220;
        case BIGPC_IMAGE_PIXFMT_I32: return bigpc_pixcvt_32_333x;
      } break;
    case BIGPC_IMAGE_PIXFMT_BGRX: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGBA: return bigpc_pixcvt_32_2103;
        case BIGPC_IMAGE_PIXFMT_BGRA: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_ARGB: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_ABGR: return bigpc_pixcvt_32_1230;
        case BIGPC_IMAGE_PIXFMT_RGBX: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_BGRX: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_XRGB: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_XBGR: return bigpc_pixcvt_32_1230;
        case BIGPC_IMAGE_PIXFMT_YA16: return bigpc_pixcvt_32_0002;
        case BIGPC_IMAGE_PIXFMT_AY16: return bigpc_pixcvt_32_2220;
        case BIGPC_IMAGE_PIXFMT_I32: return bigpc_pixcvt_32_333x;
      } break;
    case BIGPC_IMAGE_PIXFMT_XRGB: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGBA: return bigpc_pixcvt_32_3012;
        case BIGPC_IMAGE_PIXFMT_BGRA: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_ARGB: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_ABGR: return bigpc_pixcvt_32_0321;
        case BIGPC_IMAGE_PIXFMT_RGBX: return bigpc_pixcvt_32_3012;
        case BIGPC_IMAGE_PIXFMT_BGRX: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_XRGB: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_XBGR: return bigpc_pixcvt_32_0321;
        case BIGPC_IMAGE_PIXFMT_YA16: return bigpc_pixcvt_32_2000;
        case BIGPC_IMAGE_PIXFMT_AY16: return bigpc_pixcvt_32_0222;
        case BIGPC_IMAGE_PIXFMT_I32: return bigpc_pixcvt_32_x333;
      } break;
    case BIGPC_IMAGE_PIXFMT_XBGR: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGBA: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_BGRA: return bigpc_pixcvt_32_3012;
        case BIGPC_IMAGE_PIXFMT_ARGB: return bigpc_pixcvt_32_0321;
        case BIGPC_IMAGE_PIXFMT_ABGR: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_RGBX: return bigpc_pixcvt_32_3210;
        case BIGPC_IMAGE_PIXFMT_BGRX: return bigpc_pixcvt_32_3012;
        case BIGPC_IMAGE_PIXFMT_XRGB: return bigpc_pixcvt_32_0321;
        case BIGPC_IMAGE_PIXFMT_XBGR: return bigpc_pixcvt_32_0123;
        case BIGPC_IMAGE_PIXFMT_YA16: return bigpc_pixcvt_32_2000;
        case BIGPC_IMAGE_PIXFMT_AY16: return bigpc_pixcvt_32_0222;
        case BIGPC_IMAGE_PIXFMT_I32: return bigpc_pixcvt_32_x333;
      } break;
    // The rest don't follow a simple pattern, and I don't expect to use them:
    case BIGPC_IMAGE_PIXFMT_YA16: break;
    case BIGPC_IMAGE_PIXFMT_AY16: break;
    case BIGPC_IMAGE_PIXFMT_I32: break;
  }
  return 0;
}

static bigpc_pixcvt_16 bigpc_pixcvt_16_get(int dstfmt,int srcfmt) {
  switch (dstfmt) {
    case BIGPC_IMAGE_PIXFMT_RGB565: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGB565: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_BGR565: return bigpc_pixcvt_565_swap;
        case BIGPC_IMAGE_PIXFMT_RGBA5551: return bigpc_pixcvt_565_5551;
        case BIGPC_IMAGE_PIXFMT_BGRA5551: return bigpc_pixcvt_565_5551_swap;
        case BIGPC_IMAGE_PIXFMT_ARGB1555: return bigpc_pixcvt_565_1555;
        case BIGPC_IMAGE_PIXFMT_ABGR1555: return bigpc_pixcvt_565_1555_swap;
        case BIGPC_IMAGE_PIXFMT_RGBX5551: return bigpc_pixcvt_565_5551;
        case BIGPC_IMAGE_PIXFMT_BGRX5551: return bigpc_pixcvt_565_5551_swap;
        case BIGPC_IMAGE_PIXFMT_XRGB1555: return bigpc_pixcvt_565_1555;
        case BIGPC_IMAGE_PIXFMT_XBGR1555: return bigpc_pixcvt_565_1555_swap;
        case BIGPC_IMAGE_PIXFMT_Y16: return bigpc_pixcvt_565_YA88;
        case BIGPC_IMAGE_PIXFMT_YA88: return bigpc_pixcvt_565_YA88;
        case BIGPC_IMAGE_PIXFMT_AY88: return bigpc_pixcvt_565_AY88;
        case BIGPC_IMAGE_PIXFMT_I16: return bigpc_pixcvt_565_I16;
      } break;
    case BIGPC_IMAGE_PIXFMT_BGR565: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGB565: return bigpc_pixcvt_565_swap;
        case BIGPC_IMAGE_PIXFMT_BGR565: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_RGBA5551: return bigpc_pixcvt_565_5551_swap;
        case BIGPC_IMAGE_PIXFMT_BGRA5551: return bigpc_pixcvt_565_5551;
        case BIGPC_IMAGE_PIXFMT_ARGB1555: return bigpc_pixcvt_565_1555_swap;
        case BIGPC_IMAGE_PIXFMT_ABGR1555: return bigpc_pixcvt_565_1555;
        case BIGPC_IMAGE_PIXFMT_RGBX5551: return bigpc_pixcvt_565_5551_swap;
        case BIGPC_IMAGE_PIXFMT_BGRX5551: return bigpc_pixcvt_565_5551;
        case BIGPC_IMAGE_PIXFMT_XRGB1555: return bigpc_pixcvt_565_1555_swap;
        case BIGPC_IMAGE_PIXFMT_XBGR1555: return bigpc_pixcvt_565_1555;
        case BIGPC_IMAGE_PIXFMT_Y16: return bigpc_pixcvt_565_YA88;
        case BIGPC_IMAGE_PIXFMT_YA88: return bigpc_pixcvt_565_YA88;
        case BIGPC_IMAGE_PIXFMT_AY88: return bigpc_pixcvt_565_AY88;
        case BIGPC_IMAGE_PIXFMT_I16: return bigpc_pixcvt_565_I16;
      } break;
    case BIGPC_IMAGE_PIXFMT_RGBA5551: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGB565: return bigpc_pixcvt_5551_565;
        case BIGPC_IMAGE_PIXFMT_BGR565: return bigpc_pixcvt_5551_565_swap;
        case BIGPC_IMAGE_PIXFMT_RGBA5551: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_BGRA5551: return bigpc_pixcvt_5551_swap;
        case BIGPC_IMAGE_PIXFMT_ARGB1555: return bigpc_pixcvt_5551_1555;
        case BIGPC_IMAGE_PIXFMT_ABGR1555: return bigpc_pixcvt_5551_1555_swap;
        case BIGPC_IMAGE_PIXFMT_RGBX5551: return bigpc_pixcvt_5551_555x;
        case BIGPC_IMAGE_PIXFMT_BGRX5551: return bigpc_pixcvt_5551_555x_swap;
        case BIGPC_IMAGE_PIXFMT_XRGB1555: return bigpc_pixcvt_5551_x555;
        case BIGPC_IMAGE_PIXFMT_XBGR1555: return bigpc_pixcvt_5551_x555_swap;
        case BIGPC_IMAGE_PIXFMT_Y16: return bigpc_pixcvt_5551_YA88;
        case BIGPC_IMAGE_PIXFMT_YA88: return bigpc_pixcvt_5551_YA88;
        case BIGPC_IMAGE_PIXFMT_AY88: return bigpc_pixcvt_5551_AY88;
        case BIGPC_IMAGE_PIXFMT_I16: return bigpc_pixcvt_5551_I16;
      } break;
    case BIGPC_IMAGE_PIXFMT_BGRA5551: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGB565: return bigpc_pixcvt_5551_565_swap;
        case BIGPC_IMAGE_PIXFMT_BGR565: return bigpc_pixcvt_5551_565;
        case BIGPC_IMAGE_PIXFMT_RGBA5551: return bigpc_pixcvt_5551_swap;
        case BIGPC_IMAGE_PIXFMT_BGRA5551: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_ARGB1555: return bigpc_pixcvt_5551_1555_swap;
        case BIGPC_IMAGE_PIXFMT_ABGR1555: return bigpc_pixcvt_5551_1555;
        case BIGPC_IMAGE_PIXFMT_RGBX5551: return bigpc_pixcvt_5551_555x_swap;
        case BIGPC_IMAGE_PIXFMT_BGRX5551: return bigpc_pixcvt_5551_555x;
        case BIGPC_IMAGE_PIXFMT_XRGB1555: return bigpc_pixcvt_5551_x555_swap;
        case BIGPC_IMAGE_PIXFMT_XBGR1555: return bigpc_pixcvt_5551_x555;
        case BIGPC_IMAGE_PIXFMT_Y16: return bigpc_pixcvt_5551_YA88;
        case BIGPC_IMAGE_PIXFMT_YA88: return bigpc_pixcvt_5551_YA88;
        case BIGPC_IMAGE_PIXFMT_AY88: return bigpc_pixcvt_5551_AY88;
        case BIGPC_IMAGE_PIXFMT_I16: return bigpc_pixcvt_5551_I16;
      } break;
    case BIGPC_IMAGE_PIXFMT_ARGB1555: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGB565: return bigpc_pixcvt_1555_565;
        case BIGPC_IMAGE_PIXFMT_BGR565: return bigpc_pixcvt_1555_565_swap;
        case BIGPC_IMAGE_PIXFMT_RGBA5551: return bigpc_pixcvt_1555_5551;
        case BIGPC_IMAGE_PIXFMT_BGRA5551: return bigpc_pixcvt_1555_5551_swap;
        case BIGPC_IMAGE_PIXFMT_ARGB1555: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_ABGR1555: return bigpc_pixcvt_1555_swap;
        case BIGPC_IMAGE_PIXFMT_RGBX5551: return bigpc_pixcvt_1555_555x;
        case BIGPC_IMAGE_PIXFMT_BGRX5551: return bigpc_pixcvt_1555_555x_swap;
        case BIGPC_IMAGE_PIXFMT_XRGB1555: return bigpc_pixcvt_1555_x555;
        case BIGPC_IMAGE_PIXFMT_XBGR1555: return bigpc_pixcvt_1555_x555_swap;
        case BIGPC_IMAGE_PIXFMT_Y16: return bigpc_pixcvt_1555_YA88;
        case BIGPC_IMAGE_PIXFMT_YA88: return bigpc_pixcvt_1555_YA88;
        case BIGPC_IMAGE_PIXFMT_AY88: return bigpc_pixcvt_1555_AY88;
        case BIGPC_IMAGE_PIXFMT_I16: return bigpc_pixcvt_1555_I16;
      } break;
    case BIGPC_IMAGE_PIXFMT_ABGR1555: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGB565: return bigpc_pixcvt_1555_565_swap;
        case BIGPC_IMAGE_PIXFMT_BGR565: return bigpc_pixcvt_1555_565;
        case BIGPC_IMAGE_PIXFMT_RGBA5551: return bigpc_pixcvt_1555_5551_swap;
        case BIGPC_IMAGE_PIXFMT_BGRA5551: return bigpc_pixcvt_1555_5551;
        case BIGPC_IMAGE_PIXFMT_ARGB1555: return bigpc_pixcvt_1555_swap;
        case BIGPC_IMAGE_PIXFMT_ABGR1555: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_RGBX5551: return bigpc_pixcvt_1555_555x_swap;
        case BIGPC_IMAGE_PIXFMT_BGRX5551: return bigpc_pixcvt_1555_555x;
        case BIGPC_IMAGE_PIXFMT_XRGB1555: return bigpc_pixcvt_1555_x555_swap;
        case BIGPC_IMAGE_PIXFMT_XBGR1555: return bigpc_pixcvt_1555_x555;
        case BIGPC_IMAGE_PIXFMT_Y16: return bigpc_pixcvt_1555_YA88;
        case BIGPC_IMAGE_PIXFMT_YA88: return bigpc_pixcvt_1555_YA88;
        case BIGPC_IMAGE_PIXFMT_AY88: return bigpc_pixcvt_1555_AY88;
        case BIGPC_IMAGE_PIXFMT_I16: return bigpc_pixcvt_1555_I16;
      } break;
      
    case BIGPC_IMAGE_PIXFMT_RGBX5551: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGB565: return bigpc_pixcvt_5551_565;
        case BIGPC_IMAGE_PIXFMT_BGR565: return bigpc_pixcvt_5551_565_swap;
        case BIGPC_IMAGE_PIXFMT_RGBA5551: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_BGRA5551: return bigpc_pixcvt_5551_swap;
        case BIGPC_IMAGE_PIXFMT_ARGB1555: return bigpc_pixcvt_5551_1555;
        case BIGPC_IMAGE_PIXFMT_ABGR1555: return bigpc_pixcvt_5551_1555_swap;
        case BIGPC_IMAGE_PIXFMT_RGBX5551: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_BGRX5551: return bigpc_pixcvt_5551_swap;
        case BIGPC_IMAGE_PIXFMT_XRGB1555: return bigpc_pixcvt_5551_1555;
        case BIGPC_IMAGE_PIXFMT_XBGR1555: return bigpc_pixcvt_5551_1555_swap;
        case BIGPC_IMAGE_PIXFMT_Y16: return bigpc_pixcvt_5551_YA88;
        case BIGPC_IMAGE_PIXFMT_YA88: return bigpc_pixcvt_5551_YA88;
        case BIGPC_IMAGE_PIXFMT_AY88: return bigpc_pixcvt_5551_AY88;
        case BIGPC_IMAGE_PIXFMT_I16: return bigpc_pixcvt_5551_I16;
      } break;
    case BIGPC_IMAGE_PIXFMT_BGRX5551: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGB565: return bigpc_pixcvt_5551_565_swap;
        case BIGPC_IMAGE_PIXFMT_BGR565: return bigpc_pixcvt_5551_565;
        case BIGPC_IMAGE_PIXFMT_RGBA5551: return bigpc_pixcvt_5551_swap;
        case BIGPC_IMAGE_PIXFMT_BGRA5551: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_ARGB1555: return bigpc_pixcvt_5551_1555_swap;
        case BIGPC_IMAGE_PIXFMT_ABGR1555: return bigpc_pixcvt_5551_1555;
        case BIGPC_IMAGE_PIXFMT_RGBX5551: return bigpc_pixcvt_5551_swap;
        case BIGPC_IMAGE_PIXFMT_BGRX5551: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_XRGB1555: return bigpc_pixcvt_5551_1555_swap;
        case BIGPC_IMAGE_PIXFMT_XBGR1555: return bigpc_pixcvt_5551_1555;
        case BIGPC_IMAGE_PIXFMT_Y16: return bigpc_pixcvt_5551_YA88;
        case BIGPC_IMAGE_PIXFMT_YA88: return bigpc_pixcvt_5551_YA88;
        case BIGPC_IMAGE_PIXFMT_AY88: return bigpc_pixcvt_5551_AY88;
        case BIGPC_IMAGE_PIXFMT_I16: return bigpc_pixcvt_5551_I16;
      } break;
    case BIGPC_IMAGE_PIXFMT_XRGB1555: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGB565: return bigpc_pixcvt_1555_565;
        case BIGPC_IMAGE_PIXFMT_BGR565: return bigpc_pixcvt_1555_565_swap;
        case BIGPC_IMAGE_PIXFMT_RGBA5551: return bigpc_pixcvt_1555_5551;
        case BIGPC_IMAGE_PIXFMT_BGRA5551: return bigpc_pixcvt_1555_5551_swap;
        case BIGPC_IMAGE_PIXFMT_ARGB1555: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_ABGR1555: return bigpc_pixcvt_1555_swap;
        case BIGPC_IMAGE_PIXFMT_RGBX5551: return bigpc_pixcvt_1555_5551;
        case BIGPC_IMAGE_PIXFMT_BGRX5551: return bigpc_pixcvt_1555_5551_swap;
        case BIGPC_IMAGE_PIXFMT_XRGB1555: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_XBGR1555: return bigpc_pixcvt_1555_swap;
        case BIGPC_IMAGE_PIXFMT_Y16: return bigpc_pixcvt_1555_YA88;
        case BIGPC_IMAGE_PIXFMT_YA88: return bigpc_pixcvt_1555_YA88;
        case BIGPC_IMAGE_PIXFMT_AY88: return bigpc_pixcvt_1555_AY88;
        case BIGPC_IMAGE_PIXFMT_I16: return bigpc_pixcvt_1555_I16;
      } break;
    case BIGPC_IMAGE_PIXFMT_XBGR1555: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_RGB565: return bigpc_pixcvt_1555_565_swap;
        case BIGPC_IMAGE_PIXFMT_BGR565: return bigpc_pixcvt_1555_565;
        case BIGPC_IMAGE_PIXFMT_RGBA5551: return bigpc_pixcvt_1555_5551_swap;
        case BIGPC_IMAGE_PIXFMT_BGRA5551: return bigpc_pixcvt_1555_5551;
        case BIGPC_IMAGE_PIXFMT_ARGB1555: return bigpc_pixcvt_1555_swap;
        case BIGPC_IMAGE_PIXFMT_ABGR1555: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_RGBX5551: return bigpc_pixcvt_1555_5551_swap;
        case BIGPC_IMAGE_PIXFMT_BGRX5551: return bigpc_pixcvt_1555_5551;
        case BIGPC_IMAGE_PIXFMT_XRGB1555: return bigpc_pixcvt_1555_swap;
        case BIGPC_IMAGE_PIXFMT_XBGR1555: return bigpc_pixcvt_16_identity;
        case BIGPC_IMAGE_PIXFMT_Y16: return bigpc_pixcvt_1555_YA88;
        case BIGPC_IMAGE_PIXFMT_YA88: return bigpc_pixcvt_1555_YA88;
        case BIGPC_IMAGE_PIXFMT_AY88: return bigpc_pixcvt_1555_AY88;
        case BIGPC_IMAGE_PIXFMT_I16: return bigpc_pixcvt_1555_I16;
      } break;
    // Not going to bother with the rest:
    case BIGPC_IMAGE_PIXFMT_Y16:
    case BIGPC_IMAGE_PIXFMT_YA88:
    case BIGPC_IMAGE_PIXFMT_AY88:
    case BIGPC_IMAGE_PIXFMT_I16: break;
  }
  return 0;
}

static bigpc_pixcvt_8 bigpc_pixcvt_8_get(int dstfmt,int srcfmt) {
  switch (dstfmt) {
    case BIGPC_IMAGE_PIXFMT_Y8: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_Y8: return bigpc_pixcvt_8_identity;
        case BIGPC_IMAGE_PIXFMT_A8: return bigpc_pixcvt_8_identity;
        case BIGPC_IMAGE_PIXFMT_I8: return bigpc_pixcvt_8_identity;
      } break;
    case BIGPC_IMAGE_PIXFMT_A8: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_Y8: return bigpc_pixcvt_8_identity;
        case BIGPC_IMAGE_PIXFMT_A8: return bigpc_pixcvt_8_identity;
        case BIGPC_IMAGE_PIXFMT_I8: return bigpc_pixcvt_8_identity;
      } break;
    case BIGPC_IMAGE_PIXFMT_I8: switch (srcfmt) {
        case BIGPC_IMAGE_PIXFMT_Y8: return bigpc_pixcvt_8_identity;
        case BIGPC_IMAGE_PIXFMT_A8: return bigpc_pixcvt_8_identity;
        case BIGPC_IMAGE_PIXFMT_I8: return bigpc_pixcvt_8_identity;
      } break;
  }
  return 0;
}

/* Apply conversion.
 */
 
static void bigpc_image_convert_32(struct bigpc_image *image,bigpc_pixcvt_32 cvt) {
  uint32_t *row=image->v;
  int wstride=image->stride>>2;
  int yi=image->h;
  for (;yi-->0;row+=wstride) {
    uint32_t *p=row;
    int xi=image->w;
    for (;xi-->0;p++) *p=cvt(*p);
  }
}
 
static void bigpc_image_convert_16(struct bigpc_image *image,bigpc_pixcvt_16 cvt) {
  uint16_t *row=image->v;
  int wstride=image->stride>>1;
  int yi=image->h;
  for (;yi-->0;row+=wstride) {
    uint16_t *p=row;
    int xi=image->w;
    for (;xi-->0;p++) *p=cvt(*p);
  }
}
 
static void bigpc_image_convert_8(struct bigpc_image *image,bigpc_pixcvt_8 cvt) {
  uint8_t *row=image->v;
  int yi=image->h;
  for (;yi-->0;row+=image->stride) {
    uint8_t *p=row;
    int xi=image->w;
    for (;xi-->0;p++) *p=cvt(*p);
  }
}

/* Convert pixels in place, among same-size formats.
 */
 
void bigpc_image_convert_in_place(struct bigpc_image *image,int pixfmt) {
  if (!image) return;
  if (image->pixfmt==pixfmt) return;
  switch (image->storage) {
    case BIGPC_IMAGE_STORAGE_32: {
        bigpc_pixcvt_32 cvt=bigpc_pixcvt_32_get(pixfmt,image->pixfmt);
        if (!cvt) break;
        bigpc_image_convert_32(image,cvt);
        image->pixfmt=pixfmt;
      } return;
    case BIGPC_IMAGE_STORAGE_16: {
        bigpc_pixcvt_16 cvt=bigpc_pixcvt_16_get(pixfmt,image->pixfmt);
        if (!cvt) break;
        bigpc_image_convert_16(image,cvt);
        image->pixfmt=pixfmt;
      } return;
    case BIGPC_IMAGE_STORAGE_8: {
        bigpc_pixcvt_8 cvt=bigpc_pixcvt_8_get(pixfmt,image->pixfmt);
        if (!cvt) break;;
        bigpc_image_convert_8(image,cvt);
        image->pixfmt=pixfmt;
      } return;
  }
  fprintf(stderr,"%s:WARNING: Failed to convert from %d to %d, storage mode %d\n",__func__,image->pixfmt,pixfmt,image->storage);
}

#include "fmn_platform.h"

/* Force a concrete pixfmt.
 */
 
uint8_t fmn_pixfmt_concrete(uint8_t pixfmt) {
  switch (pixfmt) {
    // Wildcards:
    case FMN_VIDEO_PIXFMT_ANY: return FMN_VIDEO_PIXFMT_RGBA;
    case FMN_VIDEO_PIXFMT_ANY_1: return FMN_VIDEO_PIXFMT_Y1BE;
    case FMN_VIDEO_PIXFMT_ANY_2: return 0; // no 2-bit formats yet
    case FMN_VIDEO_PIXFMT_ANY_4: return 0; // no 4-bit formats yet
    case FMN_VIDEO_PIXFMT_ANY_8: return FMN_VIDEO_PIXFMT_Y8;
    case FMN_VIDEO_PIXFMT_ANY_16: return FMN_VIDEO_PIXFMT_RGB565LE;
    case FMN_VIDEO_PIXFMT_ANY_24: return 0; // no 24-bit formats yet
    case FMN_VIDEO_PIXFMT_ANY_32: return FMN_VIDEO_PIXFMT_RGBA;
    // Known concrete types:
    case FMN_VIDEO_PIXFMT_Y1BE:
    case FMN_VIDEO_PIXFMT_W1BE:
    case FMN_VIDEO_PIXFMT_I8:
    case FMN_VIDEO_PIXFMT_Y8:
    case FMN_VIDEO_PIXFMT_RGB565LE:
    case FMN_VIDEO_PIXFMT_RGBA4444BE:
    case FMN_VIDEO_PIXFMT_RGBA:
    case FMN_VIDEO_PIXFMT_BGRA:
    case FMN_VIDEO_PIXFMT_ARGB:
    case FMN_VIDEO_PIXFMT_ABGR:
      return pixfmt;
  }
  return 0;
}

/* Pixfmt properties.
 */

uint8_t fmn_pixfmt_get_pixel_size(uint8_t pixfmt) {
  switch (pixfmt) {
    case FMN_VIDEO_PIXFMT_ANY_1:
    case FMN_VIDEO_PIXFMT_Y1BE:
    case FMN_VIDEO_PIXFMT_W1BE:
      return 1;
    case FMN_VIDEO_PIXFMT_ANY_2:
      return 2;
    case FMN_VIDEO_PIXFMT_ANY_4:
      return 4;
    case FMN_VIDEO_PIXFMT_ANY_8:
    case FMN_VIDEO_PIXFMT_I8:
    case FMN_VIDEO_PIXFMT_Y8:
      return 8;
    case FMN_VIDEO_PIXFMT_ANY_16:
    case FMN_VIDEO_PIXFMT_RGB565LE:
    case FMN_VIDEO_PIXFMT_RGBA4444BE:
      return 16;
    case FMN_VIDEO_PIXFMT_ANY_24:
      return 24;
    case FMN_VIDEO_PIXFMT_ANY:
    case FMN_VIDEO_PIXFMT_ANY_32:
    case FMN_VIDEO_PIXFMT_RGBA:
    case FMN_VIDEO_PIXFMT_BGRA:
    case FMN_VIDEO_PIXFMT_ARGB:
    case FMN_VIDEO_PIXFMT_ABGR:
      return 32;
  }
  return 0;
}

uint8_t fmn_pixfmt_has_alpha(uint8_t pixfmt) {
  switch (pixfmt) {
    case FMN_VIDEO_PIXFMT_I8: // color table in play, we have to assume "yes"
    case FMN_VIDEO_PIXFMT_RGBA4444BE:
    case FMN_VIDEO_PIXFMT_RGBA:
    case FMN_VIDEO_PIXFMT_BGRA:
    case FMN_VIDEO_PIXFMT_ARGB:
    case FMN_VIDEO_PIXFMT_ABGR:
      return 1;
  }
  return 0;
}

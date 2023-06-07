/* bigpc_image.h
 * Shared software image type, mostly for video driver framebuffers.
 */
 
#ifndef BIGPC_IMAGE_H
#define BIGPC_IMAGE_H

#include <stdint.h>

#define BIGPC_IMAGE_SIZE_LIMIT 4096

/* "storage" refers to how pixels are laid out in memory.
 * In general, we assume that rows are aligned to 1 byte, and pixel order is LRTB.
 * Where there's an exception to that, you'll need a special STORAGE value.
 */
#define BIGPC_IMAGE_STORAGE_32      1
#define BIGPC_IMAGE_STORAGE_B24     2
#define BIGPC_IMAGE_STORAGE_L24     3
#define BIGPC_IMAGE_STORAGE_16      4
#define BIGPC_IMAGE_STORAGE_8       5
#define BIGPC_IMAGE_STORAGE_B4      6
#define BIGPC_IMAGE_STORAGE_L4      7
#define BIGPC_IMAGE_STORAGE_B2      8
#define BIGPC_IMAGE_STORAGE_L2      9
#define BIGPC_IMAGE_STORAGE_B1     10
#define BIGPC_IMAGE_STORAGE_L1     11
#define BIGPC_IMAGE_STORAGE_B64    12 /* 8 bytes, only every other is read. */
#define BIGPC_IMAGE_STORAGE_B48    13 /* 6 bytes, only every other is read. */
#define BIGPC_IMAGE_STORAGE_THUMBY 14

/* "pixfmt" refers to the content of a pixel after you've pulled it out of the image.
 * Channel names and sizes read big-endianly. (even if the pixels are stored little-endian).
 */
#define BIGPC_IMAGE_PIXFMT_RGBA      1
#define BIGPC_IMAGE_PIXFMT_BGRA      2
#define BIGPC_IMAGE_PIXFMT_ARGB      3
#define BIGPC_IMAGE_PIXFMT_ABGR      4
#define BIGPC_IMAGE_PIXFMT_RGBX      5
#define BIGPC_IMAGE_PIXFMT_BGRX      6
#define BIGPC_IMAGE_PIXFMT_XRGB      7
#define BIGPC_IMAGE_PIXFMT_XBGR      8
#define BIGPC_IMAGE_PIXFMT_RGB565    9
#define BIGPC_IMAGE_PIXFMT_BGR565   10
#define BIGPC_IMAGE_PIXFMT_RGBA5551 11
#define BIGPC_IMAGE_PIXFMT_BGRA5551 12
#define BIGPC_IMAGE_PIXFMT_ARGB1555 13
#define BIGPC_IMAGE_PIXFMT_ABGR1555 14
#define BIGPC_IMAGE_PIXFMT_RGBX5551 15
#define BIGPC_IMAGE_PIXFMT_BGRX5551 16
#define BIGPC_IMAGE_PIXFMT_XRGB1555 17
#define BIGPC_IMAGE_PIXFMT_XBGR1555 18
#define BIGPC_IMAGE_PIXFMT_Y1       19
#define BIGPC_IMAGE_PIXFMT_Y2       20
#define BIGPC_IMAGE_PIXFMT_Y4       21
#define BIGPC_IMAGE_PIXFMT_Y8       22
#define BIGPC_IMAGE_PIXFMT_Y16      23
#define BIGPC_IMAGE_PIXFMT_YA88     24
#define BIGPC_IMAGE_PIXFMT_YA16     26
#define BIGPC_IMAGE_PIXFMT_AY88     27
#define BIGPC_IMAGE_PIXFMT_AY16     28
#define BIGPC_IMAGE_PIXFMT_A1       29
#define BIGPC_IMAGE_PIXFMT_A2       30
#define BIGPC_IMAGE_PIXFMT_A4       31
#define BIGPC_IMAGE_PIXFMT_A8       32
#define BIGPC_IMAGE_PIXFMT_W1       33 /* Y but reversed, 0=white 1=black */
#define BIGPC_IMAGE_PIXFMT_I1       34
#define BIGPC_IMAGE_PIXFMT_I2       35
#define BIGPC_IMAGE_PIXFMT_I4       36
#define BIGPC_IMAGE_PIXFMT_I8       37
#define BIGPC_IMAGE_PIXFMT_I16      38
#define BIGPC_IMAGE_PIXFMT_I32      39

#define BIGPC_IMAGE_FLAG_OWNV        0x01
#define BIGPC_IMAGE_FLAG_TRANSPARENT 0x02
#define BIGPC_IMAGE_FLAG_WRITEABLE   0x04

struct bigpc_image {
  void *v;
  int w,h;
  int stride;
  int storage;
  int pixfmt;
  int flags;
  int refc;
  struct bigpc_image *keepalive;
};

void bigpc_image_del(struct bigpc_image *image);
int bigpc_image_ref(struct bigpc_image *image);

// Pixels initially zero.
struct bigpc_image *bigpc_image_new_alloc(int w,int h,int storage,int pixfmt);

// Assumes writeable, and will free at del.
struct bigpc_image *bigpc_image_new_handoff(
  void *v,
  int w,int h,int stride,
  int storage,int pixfmt
);

// Assumes readonly.
struct bigpc_image *bigpc_image_new_borrow(
  const void *v,
  int w,int h,int stride,
  int storage,int pixfmt,
  struct bigpc_image *keepalive
);

/* For the time being at least, we do not actually convert pixels.
 */
struct bigpc_image *bigpc_image_new_convert(
  const struct bigpc_image *src,
  int x,int y,int w,int h,
  int storage,int pixfmt
);

struct bigpc_image_iterator_1d {
  uint8_t *p;
  int q;
  int d; // stride in bits
  int c;
};

struct bigpc_image_iterator {
  uint32_t (*read)(const void *p,int q);
  void (*write)(void *p,int q,uint32_t src);
  struct bigpc_image_iterator_1d minor;
  struct bigpc_image_iterator_1d major;
  struct bigpc_image_iterator_1d minor0;
};

/* Set up an iterator to walk a rectangle of pixels in (image).
 * Bounds must be valid and not empty, we fail otherwise.
 * (xform) only controls the order of iteration. In particular, it does not swap (w) and (h).
 * The natural order of iteration is LRTB, regardless of how they're stored in (image).
 */
int bigpc_image_iterate(
  struct bigpc_image_iterator *iter,
  struct bigpc_image *image,
  int x,int y,int w,int h,
  uint8_t xform
);

#define BIGPC_IMAGE_ITERATOR_NEXT(iter) ({ \
  int _result=0; \
  if ((iter)->minor.c) { \
    (iter)->minor.c--; \
    (iter)->minor.q+=(iter)->minor.d; \
    (iter)->minor.p+=((iter)->minor.q)>>3; \
    (iter)->minor.q&=7; \
    _result=1; \
  } else if ((iter)->major.c) { \
    (iter)->major.c--; \
    (iter)->major.q+=(iter)->major.d; \
    (iter)->major.p+=((iter)->major.q)>>3; \
    (iter)->major.q&=7; \
    (iter)->minor=(iter)->minor0; \
    (iter)->minor.p=(iter)->major.p; \
    _result=1; \
  } \
  _result; \
})

#define BIGPC_IMAGE_ITERATOR_READ(iter) (iter)->read((iter)->minor.p,(iter)->minor.q)
#define BIGPC_IMAGE_ITERATOR_WRITE(iter,pixel) (iter)->write((iter)->minor.p,(iter)->minor.q,pixel)

/* Copy pixels verbatim from one image to another.
 * This doesn't treat transparency special; we'll write "transparent" out to (dst).
 * (dst) and (src) may be the same image but results are undefined if the regions overlap.
 */
void bigpc_image_xfer(
  struct bigpc_image *dst,int dstx,int dsty,
  struct bigpc_image *src,int srcx,int srcy,
  int w,int h
);

int bigpc_image_measure(int w,int h,int storage);

#endif

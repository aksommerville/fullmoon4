/* fontosaur.h
 * Generate images of text from our image resources.
 * Uses a private static cache of images (beware that if you reload the data file, we might keep using the old ones).
 */
 
#ifndef FONTOSAUR_H
#define FONTOSAUR_H

struct fmn_datafile;

// Fontosaur's images must have minimal stride, pack to one byte.
struct fontosaur_image {
  void *v;
  int w,h;
  int pixelsize; // bits
};

void fontosaur_image_cleanup(struct fontosaur_image *image);

/* Generate a text image, looking up from the datafile.
 * The named image is assumed to consist of 16x6 glyphs, for codepoints 0x20..0x7f.
 * Input images must be white-on-black (luma, not alpha!).
 * Output can be 8, 16, or 32 bit. Multibyte output, we write the pixel big-endianly.
 * We break lines only on explicit 0x0a.
 * Text is rendered exactly as is, one byte per glyph.
 * Outside G0, except LF, render as blank spaces, at the glyph width.
 * (align) is <0 for left, 0 for center, or >0 for right.
 * (dst) should be zeroed initially. We will determine dimensions and allocate (v).
 * There are also some draconian size limits; the largest image we can produce is about 20 MB.
 */
int fontosaur_render_text(
  struct fontosaur_image *dst,
  struct fmn_datafile *datafile,int imageid,
  int pixelsize,int bgpixel,int fgpixel,
  int align,
  const char *src,int srcc
);

/* Generate text from a font image you got somewhere else.
 */
int fontosaur_render_text_from_raw_image(
  struct fontosaur_image *dst,
  const struct fontosaur_image *font,
  int pixelsize,int bgpixel,int fgpixel,
  int align,
  const char *src,int srcc
);

#endif

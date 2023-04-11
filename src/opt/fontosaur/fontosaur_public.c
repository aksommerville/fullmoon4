#include "fontosaur_internal.h"

struct fontosaur fontosaur={0};

/* Cleanup image.
 */
 
void fontosaur_image_cleanup(struct fontosaur_image *image) {
  if (image->v) free(image->v);
  memset(image,0,sizeof(struct fontosaur_image));
}

/* Render text from imageid.
 */

int fontosaur_render_text(
  struct fontosaur_image *dst,
  struct fmn_datafile *datafile,int imageid,
  int pixelsize,int bgpixel,int fgpixel,
  int align,
  const char *src,int srcc
) {
  struct fontosaur_image *font=fontosaur_get_image(datafile,imageid);
  if (!font) return -1;
  return fontosaur_render_text_from_raw_image(dst,font,pixelsize,bgpixel,fgpixel,align,src,srcc);
}

/* Render one glyph, filling the entire output box.
 */
 
static void fontosaur_render_glyph(
  struct fontosaur_image *dst,int dstx,int dsty,
  const struct fontosaur_image *src,int srcx,int srcy,
  int w,int h,int bgpixel,int fgpixel
) {
  const uint8_t *srcrow=src->v;
  srcrow+=srcy*src->w+srcx;
  switch (dst->pixelsize) {
    case 8: {
        uint8_t *dstrow=((uint8_t*)dst->v)+dsty*dst->w+dstx;
        for (;h-->0;dstrow+=dst->w,srcrow+=src->w) {
          uint8_t *dstp=dstrow;
          const uint8_t *srcp=srcrow;
          int xi=w; for (;xi-->0;dstp++,srcp++) {
            if (*srcp) *dstp=fgpixel;
            else *dstp=bgpixel;
          }
        }
      } break;
    case 16: {
        uint16_t *dstrow=dst->v;
        dstrow+=dsty*dst->w+dstx;
        for (;h-->0;dstrow+=dst->w,srcrow+=src->w) {
          uint16_t *dstp=dstrow;
          const uint8_t *srcp=srcrow;
          int xi=w; for (;xi-->0;dstp++,srcp++) {
            if (*srcp) *dstp=fgpixel;
            else *dstp=bgpixel;
          }
        }
      } break;
    case 32: {
        uint32_t *dstrow=dst->v;
        dstrow+=dsty*dst->w+dstx;
        for (;h-->0;dstrow+=dst->w,srcrow+=src->w) {
          uint32_t *dstp=dstrow;
          const uint8_t *srcp=srcrow;
          int xi=w; for (;xi-->0;dstp++,srcp++) {
            if (*srcp) *dstp=fgpixel;
            else *dstp=bgpixel;
          }
        }
      } break;
    default: return;
  }
}

/* Fill rectangle.
 */
 
static void fontosaur_fill_rect(
  struct fontosaur_image *dst,
  int x,int y,int w,int h,
  int pixel
) {
  switch (dst->pixelsize) {
    case 8: {
        uint8_t *dstrow=((uint8_t*)dst->v)+y*dst->w+x;
        for (;h-->0;dstrow+=dst->w) memset(dstrow,pixel,w);
      } break;
    case 16: {
        uint16_t *dstrow=dst->v;
        dstrow+=y*dst->w+x;
        for (;h-->0;dstrow+=dst->w) {
          uint16_t *dstp=dstrow;
          int xi=w; for (;xi-->0;dstp++) *dstp=pixel;
        }
      } break;
    case 32: {
        uint32_t *dstrow=dst->v;
        dstrow+=y*dst->w+x;
        for (;h-->0;dstrow+=dst->w) {
          uint32_t *dstp=dstrow;
          int xi=w; for (;xi-->0;dstp++) *dstp=pixel;
        }
      } break;
  }
}

/* Render one line of text.
 * We overwrite the entire row.
 * Bounds must be valid.
 */
 
static void fontosaur_render_line(
  struct fontosaur_image *dst,int dsty,
  const struct fontosaur_image *font,int glyphw,int glyphh,
  int bgpixel,int fgpixel,int align,
  const char *src,int srcc
) {
  int srcw=srcc*glyphw;
  int dstx;
  if (align<0) dstx=0;
  else if (align>0) dstx=dst->w-srcw;
  else dstx=(dst->w>>1)-(srcw>>1);
  fontosaur_fill_rect(dst,0,dsty,dstx,glyphh,bgpixel);
  fontosaur_fill_rect(dst,dstx+srcw,dsty,dst->w-srcw-dstx,glyphh,bgpixel);
  for (;srcc-->0;src++,dstx+=glyphw) {
    if ((unsigned char)(*src)<0x20) continue;
    if ((unsigned char)(*src)>0x7f) continue;
    int srcx=((*src)&15)*glyphw;
    int srcy=(((*src)-0x20)>>4)*glyphh;
    fontosaur_render_glyph(dst,dstx,dsty,font,srcx,srcy,glyphw,glyphh,bgpixel,fgpixel);
  }
}

/* Put the longest line and count of lines in (dst->w,h).
 * Note that those are in glyphs, not pixels.
 */
 
static void fontosaur_measure_text(struct fontosaur_image *dst,const char *src,int srcc) {
  dst->w=dst->h=0;
  int srcp=0;
  while (srcp<srcc) {
    int linec=0;
    while ((srcp<srcc)&&(src[srcp++]!=0x0a)) linec++;
    if (linec>dst->w) dst->w=linec;
    dst->h++;
  }
}

/* Render text from raw image.
 */

int fontosaur_render_text_from_raw_image(
  struct fontosaur_image *dst,
  const struct fontosaur_image *font,
  int pixelsize,int bgpixel,int fgpixel,
  int align,
  const char *src,int srcc
) {
  if (!dst||!font) return -1;
  if (font->pixelsize!=8) return -1;
  
  /* Our public interface says the provided pixels are always big-endian, and that's sensible.
   * But it's more efficient in 16 and 32 bit cases to write full words.
   * So if we are little-endian, which we very likely are, swap the bytes.
   */
  switch (pixelsize) {
    // 1-bit would also make sense, and not too hard to do. Should we?
    case 8: break;
    case 16: {
        uint16_t detect=0x4321;
        if (((uint8_t*)&detect)[0]==0x21) {
          bgpixel=(bgpixel<<8)|((bgpixel&0xff00)>>8);
          fgpixel=(fgpixel<<8)|((fgpixel&0xff00)>>8);
        }
      } break;
    case 32: {
        uint32_t detect=0x87654321;
        if (((uint8_t*)&detect)[0]==0x21) {
          bgpixel=(bgpixel>>24)|((bgpixel&0xff0000)>>8)|((bgpixel&0xff00)<<8)|(bgpixel<<24);
          fgpixel=(fgpixel>>24)|((fgpixel&0xff0000)>>8)|((fgpixel&0xff00)<<8)|(fgpixel<<24);
        }
      } break;
    default: return -1;
  }
  
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int glyphw=font->w/16;
  int glyphh=font->h/6;
  if ((glyphw<1)||(glyphw>FONTOSAUR_GLYPHW_LIMIT)) return -1;
  if ((glyphh<1)||(glyphh>FONTOSAUR_GLYPHH_LIMIT)) return -1;
  fontosaur_image_cleanup(dst);
  fontosaur_measure_text(dst,src,srcc);
  if ((dst->w>FONTOSAUR_COLUMN_LIMIT)||(dst->h>FONTOSAUR_ROW_LIMIT)) return -1;
  dst->w*=glyphw;
  dst->h*=glyphh;
  int dststride=(dst->w*pixelsize+7)>>3;
  int dstsize=dststride*dst->h;
  if (!(dst->v=malloc(dstsize))) return -1;
  dst->pixelsize=pixelsize;
  int dsty=0,srcp=0;
  while (srcp<srcc) {
    const char *line=src+srcp;
    int linec=0;
    while ((srcp<srcc)&&(src[srcp++]!=0x0a)) linec++;
    fontosaur_render_line(dst,dsty,font,glyphw,glyphh,bgpixel,fgpixel,align,line,linec);
    dsty+=glyphh;
  }
  return 0;
}

#ifndef FONTOSAUR_INTERNAL_H
#define FONTOSAUR_INTERNAL_H

#include "fontosaur.h"
#include "opt/datafile/fmn_datafile.h"
#include "opt/png/png.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

// Sanity limits on output image size, in glyphs.
#define FONTOSAUR_COLUMN_LIMIT 100
#define FONTOSAUR_ROW_LIMIT 50

// Sanity limits on input glyph size, in pixels.
#define FONTOSAUR_GLYPHW_LIMIT 32
#define FONTOSAUR_GLYPHH_LIMIT 32

extern struct fontosaur {
  struct fontosaur_cache_entry {
    int imageid;
    struct fontosaur_image image;
  } *entryv;
  int entryc,entrya;
} fontosaur;

// WEAK
struct fontosaur_image *fontosaur_get_image(struct fmn_datafile *file,int imageid);

#endif

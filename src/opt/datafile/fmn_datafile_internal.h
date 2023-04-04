#ifndef FMN_DATAFILE_INTERNAL_H
#define FMN_DATAFILE_INTERNAL_H

#include "fmn_datafile.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

struct fmn_datafile {
  char *path;
  uint8_t *serial; // verbatim
  int serialc;
  // (entryv) is sorted by (type,id,qualifier). (p,c) points into (serial).
  struct fmn_datafile_toc_entry {
    uint16_t type;
    uint16_t qualifier;
    uint32_t id;
    uint32_t p;
    uint32_t c;
  } *entryv;
  int entryc,entrya;
};

int fmn_datafile_decode_toc(struct fmn_datafile *file);

#endif

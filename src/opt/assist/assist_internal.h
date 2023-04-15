#ifndef ASSIST_INTERNAL_H
#define ASSIST_INTERNAL_H

#include "assist.h"
#include "opt/datafile/fmn_datafile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

struct assist_timestamp {
  double real;
  double cpu;
};
void assist_now(struct assist_timestamp *t);

// Returns exit status.
int assist_analyze_data(const char *path);

/* From src/app/fmn_platform.h.
 * We read and cache all first time somebody asks for one.
 * Returned strings are WEAK and constant.
 */
int assist_get_sound_name(void *dstpp,int id);

#endif

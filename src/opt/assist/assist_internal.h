#ifndef ASSIST_INTERNAL_H
#define ASSIST_INTERNAL_H

#include "assist.h"
#include "opt/datafile/fmn_datafile.h"
#include "app/fmn_platform.h"
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

/* From source files.
 * We read and cache all first time somebody asks for one.
 * Returned strings are WEAK and constant.
 */
int assist_get_sound_name(void *dstpp,int id);
int assist_get_sound_id_by_name(const char *name,int namec);
int assist_get_sprite_name(void *dstpp,int id); // FMN_SPRCTL_*, not "sprite" resources
int assist_get_sprite_id_by_name(const char *name,int namec); // ''
int assist_get_sprite_style_by_name(const char *name,int namec);
int assist_get_sprite_physics_by_name(const char *name,int namec);
int assist_get_xform_by_name(const char *name,int namec);
int assist_get_item_by_name(const char *name,int namec);
int assist_get_map_event_by_name(const char *name,int namec);
int assist_get_map_callback_by_name(const char *name,int namec);
int assist_get_resource_id_by_name(const char *tname,const char *rname,int rnamec);
int assist_get_gsbit_by_name(const char *name,int namec);

#endif

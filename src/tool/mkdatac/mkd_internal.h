#ifndef MKD_INTERNAL_H
#define MKD_INTERNAL_H

#include "opt/datafile/fmn_datafile.h"
#include "tool/common/serial/serial.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#define MKD_CMD_UNSET 0
#define MKD_CMD_single 1
#define MKD_CMD_archive 2
#define MKD_CMD_showtoc 3
#define MKD_CMD_extract 4

#define MKD_FOR_EACH_CMD \
  _(single) \
  _(archive) \
  _(showtoc) \
  _(extract)
  
#define FMN_RESTYPE_KNOWN_UNKNOWN -1

struct mkd_config {
  const char *exename;
  char *dstpath;
  char **srcpathv;
  int srcpathc,srcpatha;
  int cmd;
  struct mkd_arg {
    char *k; int kc;
    char *v; int vc;
    int vn; // zero if evaluation fails
  } *argv;
  int argc,arga;
};

extern struct mkd {
  struct mkd_config config;
  char *src;
  int srcc;
  struct sr_encoder dst;
} mkd;

void mkd_config_cleanup(struct mkd_config *config);
int mkd_config_argv(struct mkd_config *config,int argc,char **argv);
int mkd_config_ready(struct mkd_config *config);

/* Reading as int, real zeroes are indistinguishable from missing arguments.
 */
int mkd_config_get_arg_string(void *dstpp,const struct mkd_config *config,const char *k,int kc);
int mkd_config_get_arg_int(const struct mkd_config *config,const char *k,int kc);

struct mkd_respath {
  const char *path; // input, verbatim
  int pathc;
  const char *tname; // The last directory, conventionally the resource type name.
  int tnamec;
  const char *base;
  int basec;
  const char *qname; // Qualifier name, if we can determine.
  int qnamec;
  int restype; // FMN_RESTYPE_* from opt/datafile/fmn_datafile.h, or FMN_RESTYPE_KNOWN_UNKNOWN, or zero.
  int resid; // From start of basename, or zero.
  int resq; // Qualifier or zero.
};
int mkd_respath_eval(struct mkd_respath *respath,const char *src);

const char *mkd_restype_repr(int restype);
int mkd_restype_eval(const char *src,int srcc);
int mkd_qualifier_eval(int type,const char *src,int srcc);

/* Generate a friendly human-readable form of any qualified resource ID.
 */
int mkd_resid_repr(char *dst,int dsta,int type,int qualifier,int id);

/* Verify that a single input or output path was provided, then read or write it.
 * Reads into (mkd.src,mkd.srcc) or writes from (mkd.dst).
 */
int mkd_read_single_input();
int mkd_write_single_output();

/* From src/opt/assist/assist_de_re_nominarii.c
 */
int assist_get_sound_name(void *dstpp,int id);
int assist_get_sound_id_by_name(const char *name,int namec);
int assist_get_sprite_name(void *dstpp,int id);
int assist_get_sprite_id_by_name(const char *name,int namec);
int assist_get_sprite_style_by_name(const char *name,int namec);
int assist_get_sprite_physics_by_name(const char *name,int namec);
int assist_get_xform_by_name(const char *name,int namec);
int assist_get_item_by_name(const char *name,int namec);
int assist_get_map_event_by_name(const char *name,int namec);
int assist_get_map_callback_by_name(const char *name,int namec);
int assist_get_resource_id_by_name(const char *tname,const char *rname,int rnamec);
int assist_get_gsbit_by_name(const char *name,int namec);
int assist_get_spell_id_by_name(const char *name,int namec);

#endif

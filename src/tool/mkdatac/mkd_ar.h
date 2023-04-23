/* mkd_ar.h
 * Our internal archive object.
 * Unlike the similar fmn_datafile, mkd_ar is designed to be sourced from loose files, and to handle modification gracefully.
 */

#ifndef MKD_AR_H
#define MKD_AR_H

struct mkd_ar;

void mkd_ar_del(struct mkd_ar *ar);

struct mkd_ar *mkd_ar_new();

/* Decode an archive file.
 * Fails if any resources already exist. Use a fresh mkd_ar.
 * If (path) not null, we log errors to stderr.
 */
int mkd_ar_decode(struct mkd_ar *ar,const void *src,int srcc,const char *path);

int mkd_ar_add_handoff(struct mkd_ar *ar,int type,int qualifier,int id,const char *path,void *v,int c);
int mkd_ar_add_copy(struct mkd_ar *ar,int type,int qualifier,int id,const char *path,const void *v,int c);
int mkd_ar_set_path(struct mkd_ar *ar,int type,int qualifier,int id,const char *path);
int mkd_ar_set_serial(struct mkd_ar *ar,int type,int qualifier,int id,const void *v,int c);
int mkd_ar_remove(struct mkd_ar *ar,int type,int qualifier,int id);
int mkd_ar_count(const struct mkd_ar *ar);
int mkd_ar_exists(const struct mkd_ar *ar,int type,int qualifier,int id);
const char *mkd_ar_get_path(const struct mkd_ar *ar,int type,int qualifier,int id);
int mkd_ar_get_serial(void *dstpp,const struct mkd_ar *ar,int type,int qualifier,int id);

/* Do not modify the archive during iteration!
 * Modifying path or serial of an entry is safe. But adding or removing entries is not.
 */
int mkd_ar_for_each(
  struct mkd_ar *ar,
  int (*cb)(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata),
  void *userdata
);
int mkd_ar_for_each_of_type(
  struct mkd_ar *ar,int type,
  int (*cb)(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata),
  void *userdata
);
int mkd_ar_for_each_of_type_qualifier(
  struct mkd_ar *ar,int type,int qualifier,
  int (*cb)(int type,int qualifier,int id,const char *path,const void *v,int c,void *userdata),
  void *userdata
);

int mkd_ar_encode(struct sr_encoder *dst,const struct mkd_ar *ar);

#endif

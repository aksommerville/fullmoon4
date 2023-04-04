/* fmn_datafile.h
 * Coordinates loading resources from our data archive.
 * Native builds should always use this, but it's optional,
 * in case we do one with the data embedded or something.
 * POSIX file ops must be available (open, read, write, ...).
 *
 * datafile's responsibility ends with the resources' serial data.
 * No decoding or specific resource-type knowledge here, that's somebody else's problem.
 * So when you're loading something, consider first whether there is an intermediate service handling that.
 */
 
#ifndef FMN_DATAFILE_H
#define FMN_DATAFILE_H

#include <stdint.h>

struct fmn_datafile;

#define FMN_RESTYPE_IMAGE      0x01 /* qualifier TODO */
#define FMN_RESTYPE_SONG       0x02
#define FMN_RESTYPE_MAP        0x03
#define FMN_RESTYPE_TILEPROPS  0x04
#define FMN_RESTYPE_SPRITE     0x05
#define FMN_RESTYPE_STRING     0x06 /* qualifier=language */
#define FMN_RESTYPE_INSTRUMENT 0x07 /* qualifier=synthesizer */
#define FMN_RESTYPE_SOUND      0x08 /* qualifier=synthesizer */

void fmn_datafile_del(struct fmn_datafile *file);

struct fmn_datafile *fmn_datafile_open(const char *path);

/* Direct access to encoded resources.
 * This can only be done via iteration.
 */
int fmn_datafile_for_each(
  struct fmn_datafile *file,
  int (*cb)(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata),
  void *userdata
);
int fmn_datafile_for_each_of_type(
  struct fmn_datafile *file,
  uint16_t type,
  int (*cb)(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata),
  void *userdata
);
int fmn_datafile_for_each_of_qualified_type(
  struct fmn_datafile *file,
  uint16_t type,uint16_t qualifier,
  int (*cb)(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata),
  void *userdata
);
int fmn_datafile_for_each_of_id(
  struct fmn_datafile *file,
  uint16_t type,uint32_t id,
  int (*cb)(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata),
  void *userdata
);

int fmn_file_read(void *dstpp,const char *path);
int fmn_file_write(const char *path,const void *src,int srcc);

#endif

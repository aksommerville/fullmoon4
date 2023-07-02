/* fmn_datafile.h
 * Coordinates loading resources from our data archive.
 * Native builds should always use this, but it's optional,
 * in case we do one with the data embedded or something.
 * POSIX file ops must be available (open, read, write, ...).
 *
 * datafile's responsibility ends with the resources' serial data.
 * No decoding or specific resource-type knowledge here, that's somebody else's problem.
 * So when you're loading something, consider first whether there is an intermediate service handling that.
 * ...update: OK, we are going to provide analysis services that run directly on encoded resources.
 * Maps for example.
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
int fmn_datafile_reopen(struct fmn_datafile *file);

/* Direct access to encoded resources.
 * This can only be done via iteration.
 */
int fmn_datafile_for_each_qualifier(
  struct fmn_datafile *file,
  uint16_t type,
  int (*cb)(uint16_t qualifier,void *userdata),
  void *userdata
);
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

// Was planning for "iteration only", but that's already getting old...
int fmn_datafile_get_any(void *dstpp,struct fmn_datafile *file,uint16_t type,uint32_t id);
int fmn_datafile_get_qualified(void *dstpp,struct fmn_datafile *file,uint16_t type,uint16_t qualifier,uint32_t id);

int fmn_file_read(void *dstpp,const char *path);
int fmn_file_write(const char *path,const void *src,int srcc);
int fmn_dir_read(const char *path,int (*cb)(const char *path,const char *base,char type,void *userdata),void *userdata);
char fmn_file_get_type(const char *path);

/* Knowledge of specific resource type formats.
 ************************************************************/

// (argc) is usually knowable from (opcode), but 0xc0..0xff are variable-length.
int fmn_map_for_each_command(
  const void *v,int c,
  int (*cb)(uint8_t opcode,const uint8_t *argv,int argc,void *userdata),
  void *userdata
);

// If it has a concept of location, populate (xy), usually in (0,0..FMN_COLC-1,FMN_ROWC-1), and return nonzero.
// You get a sensible result in (xy) regardless of the return value; it defaults to the center of the screen.
int fmn_map_location_for_command(int16_t *xy,uint8_t opcode,const uint8_t *argv,int argc);

#endif

#include "fmn_datafile_internal.h"

/* Cleanup.
 */
 
void fmn_datafile_del(struct fmn_datafile *file) {
  if (!file) return;
  if (file->path) free(file->path);
  if (file->serial) free(file->serial);
  if (file->entryv) free(file->entryv);
  free(file);
}

/* Open.
 */

struct fmn_datafile *fmn_datafile_open(const char *path) {
  if (!path||!path[0]) return 0;
  struct fmn_datafile *file=calloc(1,sizeof(struct fmn_datafile));
  if (!file) return 0;
  
  if (!(file->path=strdup(path))) {
    free(file);
    return 0;
  }
  
  if ((file->serialc=fmn_file_read(&file->serial,path))<0) {
    fmn_datafile_del(file);
    return 0;
  }
  
  if (fmn_datafile_decode_toc(file)<0) {
    fmn_datafile_del(file);
    return 0;
  }
  
  return file;
}

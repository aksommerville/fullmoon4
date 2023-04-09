#include "fmstore_internal.h"

/* Del.
 */
 
void fmstore_del(struct fmstore *fmstore) {
  if (!fmstore) return;
  
  if (fmstore->plantv) free(fmstore->plantv);
  if (fmstore->sketchv) free(fmstore->sketchv);
  
  free(fmstore);
}

/* New.
 */

struct fmstore *fmstore_new() {
  struct fmstore *fmstore=calloc(1,sizeof(struct fmstore));
  if (!fmstore) return 0;
  return fmstore;
}

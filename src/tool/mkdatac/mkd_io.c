#include "mkd_internal.h"

/* Assert and read single input file.
 */
 
int mkd_read_single_input() {
  if (mkd.config.srcpathc!=1) {
    fprintf(stderr,"%s: Expected 1 input file, found %d.\n",mkd.config.exename,mkd.config.srcpathc);
    return -2;
  }
  if (mkd.src) free(mkd.src);
  mkd.src=0;
  mkd.srcc=0;
  if ((mkd.srcc=fmn_file_read(&mkd.src,mkd.config.srcpathv[0]))<0) {
    fprintf(stderr,"%s: Failed to read file.\n",mkd.config.srcpathv[0]);
    mkd.srcc=0;
    return -2;
  }
  return 0;
}

/* Assert and write single output file.
 */
 
int mkd_write_single_output() {
  if (!mkd.config.dstpath||!mkd.config.dstpath[0]) {
    fprintf(stderr,"%s: Output path required.\n",mkd.config.exename);
    return -2;
  }
  if (fmn_file_write(mkd.config.dstpath,mkd.dst.v,mkd.dst.c)<0) {
    fprintf(stderr,"%s: Failed to write file, %d bytes.\n",mkd.config.dstpath,mkd.dst.c);
    return -2;
  }
  return 0;
}

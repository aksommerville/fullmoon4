#include "mkd_internal.h"

int mkd_compile_map(struct mkd_respath *respath);
int mkd_compile_tileprops(struct mkd_respath *respath);
int mkd_compile_sprite(struct mkd_respath *respath);

/* Verbatim compile: Copy input to output.
 */
 
static int mkd_compile_verbatim(struct mkd_respath *respath) {
  mkd.dst.c=0;
  if (sr_encode_raw(&mkd.dst,mkd.src,mkd.srcc)<0) return -1;
  return 0;
}

/* Single file conversion, main entry point.
 */
 
int mkd_main_single() {
  int err=mkd_read_single_input();
  if (err<0) return err;
  struct mkd_respath respath={0};
  if ((err=mkd_respath_eval(&respath,mkd.config.srcpathv[0]))<0) return err;
  
  if (0) fprintf(stderr,
    "path: %.*s\n"
    "  tname: %.*s\n"
    "  base: %.*s\n"
    "  qname: %.*s\n"
    "  type: %d\n"
    "  id: %d\n"
    "  qualifier: %d\n",
    respath.pathc,respath.path,
    respath.tnamec,respath.tname,
    respath.basec,respath.base,
    respath.qnamec,respath.qname,
    respath.restype,respath.resid,respath.resq
  );
  
  // Check for unknown types.
  if (respath.restype==FMN_RESTYPE_KNOWN_UNKNOWN) return 0;
  if (!respath.restype) {
    fprintf(stderr,"%s: Unknown resource type.\n",respath.path);
    return -2;
  }
  
  //TODO Filter by qualifier.
  
  // Types that get compiled individually, do it now.
  err=-1;
  switch (respath.restype) {
  
    // Individual resource compilation:
    case FMN_RESTYPE_IMAGE: err=mkd_compile_verbatim(&respath); break; //TODO
    case FMN_RESTYPE_MAP: err=mkd_compile_map(&respath); break;
    case FMN_RESTYPE_TILEPROPS: err=mkd_compile_tileprops(&respath); break;
    case FMN_RESTYPE_SPRITE: err=mkd_compile_sprite(&respath); break;
    
    // Individual verbatim resources:
    case FMN_RESTYPE_SONG: err=mkd_compile_verbatim(&respath); break;
    
    // Packed sources that get compiled during the archive stage. Copy verbatim:
    case FMN_RESTYPE_STRING: err=mkd_compile_verbatim(&respath); break;
    case FMN_RESTYPE_INSTRUMENT: err=mkd_compile_verbatim(&respath); break;
    case FMN_RESTYPE_SOUND: err=mkd_compile_verbatim(&respath); break;
    
    default: {
        fprintf(stderr,"%s: Resource type %d not known at %s:%d\n",respath.path,respath.restype,__FILE__,__LINE__);
        return -2;
      }
  }
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error compiling resource.\n",respath.path);
    return -2;
  }
  
  if ((err=mkd_write_single_output())<0) return err;
  return 0;
}

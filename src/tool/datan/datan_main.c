#include "datan_internal.h"

struct datan datan={0};

/* --src
 */
 
static int datan_set_src(const char *path) {
  int err;
  if (datan.srcpath) {
    fprintf(stderr,"%s: Multiple source paths.\n",datan.exename);
    return -2;
  }
  datan.srcpath=path;
  
  /* Schedule of acquisition and validation against source files.
   * This is only for things that don't exist in the archive.
   */
  #define TEST(fnname) if ((err=fnname())<0) { \
    if (err!=-2) fprintf(stderr,"%s: Unspecified error at step '%s'\n",path,#fnname); \
    return -2; \
  }
  TEST(datan_acquire_gsbit)
  TEST(datan_acquire_chalk)
  
  #undef TEST
  return 0;
}

/* --archive
 */
 
static int datan_process_archive(const char *path) {
  int err;
  datan.arpath=path;
  if (datan.datafile) fmn_datafile_del(datan.datafile);
  if (!(datan.datafile=fmn_datafile_open(path))) {
    fprintf(stderr,"%s: Failed to read or decode archive.\n",path);
    return -2;
  }
  
  /* If we're in --tileusage mode, do that instead.
   * We still need to run "individual_resources", in order to populate (datan.resv).
   */
  if (datan.tileusage) {
    if ((err=datan_validate_individual_resources())<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error reading resources.\n",path);
      return -2;
    }
    if ((err=datan_tileusage())<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error gathering tile usage data.\n",path);
      return -2;
    }
    return 0;
  }
  
  /* Schedule of tests against one archive.
   */
  #define TEST(fnname) if ((err=fnname())<0) { \
    if (err!=-2) fprintf(stderr,"%s: Unspecified error at step '%s'\n",path,#fnname); \
    return -2; \
  }
  TEST(datan_validate_individual_resources) // Must come first; this populates our live-resource list.
  TEST(datan_validate_spawn_points)
  //TEST(datan_validate_res_id_continuity) // This is expected to fail, now that we tree-shake. No big deal. Can reenable if curious.
  TEST(datan_validate_cross_qualifier)
  TEST(datan_validate_save_points)
  TEST(datan_validate_song_choice)
  TEST(datan_validate_blowback)
  TEST(datan_validate_indoor_outdoor_boundaries)
  TEST(datan_validate_tileprops_against_image)
  TEST(datan_validate_buried_things)
  TEST(datan_validate_map_refs)
  TEST(datan_validate_reachability) // Recommend last because it tends to generate a large warning.
  
  #undef TEST
  return 0;
}

/* --help
 */
 
static void datan_print_usage(const char *exename) {
  if (!exename||!exename[0]) exename="datan";
  fprintf(stderr,"Usage: %s [--tileusage=HTMLPATH] [--src=DIR] --archive=FILE [--archive=FILE...]\n",exename);
  fprintf(stderr,"Each archive is processed independently.\n");
  fprintf(stderr,"--tileusage is a special mode to generate a report of which image tiles are used by maps. Normal validation doesn't run.\n");
}

/* Main.
 */
 
int main(int argc,char **argv) {
  int err;
  int archivec=0;
  if (argc>=1) datan.exename=argv[0];
  int i=1; for (;i<argc;i++) {
    const char *arg=argv[i];
    if (!strcmp(arg,"--help")) {
      datan_print_usage(argv[0]);
      return 0;
    }
    if (!memcmp(arg,"--src=",6)) {
      if ((err=datan_set_src(arg+6))<0) {
        if (err!=-2) fprintf(stderr,"%s: Unspecified error setting source directory.\n",arg+6);
        return 1;
      }
      continue;
    }
    if (!memcmp(arg,"--archive=",10)) {
      if ((err=datan_process_archive(arg+10))<0) {
        if (err!=-2) fprintf(stderr,"%s: Unspecified error processing archive.\n",arg+10);
        return 1;
      }
      archivec++;
      continue;
    }
    if (!memcmp(arg,"--tileusage=",12)) {
      datan.tileusage=arg+12;
      continue;
    }
    fprintf(stderr,"%s: Unexpected argument '%s'\n",datan.exename,arg);
    return 1;
  }
  if (!archivec) {
    datan_print_usage(argv[0]);
    return 1;
  }
  if (datan.tileusage) {
    if ((err=datan_tileusage_finish())<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error generating tile usage report.\n",datan.exename);
      return 1;
    }
  }
  return 0;
}

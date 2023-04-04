#ifndef FMN_STDSYN_INTERNAL_H
#define FMN_STDSYN_INTERNAL_H

#include "opt/bigpc/bigpc_synth.h"
#include "opt/bigpc/bigpc_audio.h"
#include <string.h>
#include <stdlib.h>

struct bigpc_synth_driver_stdsyn {
  struct bigpc_synth_driver hdr;
  float *qbuf;
  int qbufa;
  float qlevel; // 0..32767
};

#define DRIVER ((struct bigpc_synth_driver_stdsyn*)driver)

#endif

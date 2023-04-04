#ifndef FMN_MINSYN_INTERNAL_H
#define FMN_MINSYN_INTERNAL_H

#include "opt/bigpc/bigpc_synth.h"
#include "opt/bigpc/bigpc_audio.h"
#include <string.h>

struct bigpc_synth_driver_minsyn {
  struct bigpc_synth_driver hdr;
};

#define DRIVER ((struct bigpc_synth_driver_minsyn*)driver)

#endif

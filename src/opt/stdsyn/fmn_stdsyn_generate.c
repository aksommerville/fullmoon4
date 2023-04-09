#include "fmn_stdsyn_internal.h"

/* Generate signal, main entry point.
 */
 
void stdsyn_generate_signal(float *v,int c,struct bigpc_synth_driver *driver) {
  struct stdsyn_voice *voice=DRIVER->voicev;
  int i=DRIVER->voicec;
  for (;i-->0;voice++) {
    stdsyn_voice_update(v,c,voice);
  }
}

/* Release all voices.
 */

void stdsyn_release_all(struct bigpc_synth_driver *driver) {
  struct stdsyn_voice *voice=DRIVER->voicev;
  int i=DRIVER->voicec;
  for (;i-->0;voice++) stdsyn_voice_release(voice);
}

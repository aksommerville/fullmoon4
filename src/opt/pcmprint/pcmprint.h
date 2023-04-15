/* pcmprint.h
 * Shared sound effects generator.
 * I'm writing this in support of minsyn, and I expect stdsyn will also use it.
 */
 
#ifndef PCMPRINT_H
#define PCMPRINT_H

#include <stdint.h>

struct pcmprint;

void pcmprint_del(struct pcmprint *pcmprint);

/* (rate) in Hertz, the main output rate.
 * We decode during construction and don't need (src) again after.
 */
struct pcmprint *pcmprint_new(int rate,const void *src,int srcc);

/* Returns the total duration in samples. (which are also frames, we're mono only).
 */
int pcmprint_get_length(const struct pcmprint *pcmprint);

/* Peak level for integer output. Default 32000.
 */
int16_t pcmprint_get_quantization_level(const struct pcmprint *pcmprint);
void pcmprint_set_quantization_level(struct pcmprint *pcmprint,int16_t q);

/* Advance the printer by (c) frames.
 * You can alternate willy-nilly between int and float. But why on earth would you do that?
 * Float is more natural for us.
 * Returns >0 if more remains or 0 if complete. Errors only for invalid params.
 * You must provide for at least one sample of output.
 * It's safe to call forever after completion.
 */
int pcmprint_updatef(float *v,int c,struct pcmprint *pcmprint);
int pcmprint_updatei(int16_t *v,int c,struct pcmprint *pcmprint);

#endif

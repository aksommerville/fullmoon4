#ifndef PCMPRINT_INTERNAL_H
#define PCMPRINT_INTERNAL_H

#include "pcmprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#define PCMPRINT_WAVE_SERIAL_LIMIT 255
#define PCMPRINT_WAVE_SIZE_BITS 10
#define PCMPRINT_WAVE_SIZE_SAMPLES (1<<PCMPRINT_WAVE_SIZE_BITS)
#define PCMPRINT_WAVE_SHIFT (32-PCMPRINT_WAVE_SIZE_BITS)

struct pcmprint_op;
struct pcmprint_op_type;

struct pcmprint_op_type {
  uint8_t command; // leading byte in the serial input
  const char *name;
  int objlen;
  void (*del)(struct pcmprint_op *op);
  int (*init)(struct pcmprint_op *op);
  int (*decode)(struct pcmprint_op *op,const uint8_t *src,int srcc);
};

struct pcmprint_op {
  const struct pcmprint_op_type *type;
  struct pcmprint *pcmprint; // WEAK
  void (*update)(float *v,int c,struct pcmprint_op *op); // REQUIRED
};

struct pcmprint_wave {
  uint8_t serial[PCMPRINT_WAVE_SERIAL_LIMIT];
  uint8_t serialc;
  float v[PCMPRINT_WAVE_SIZE_SAMPLES];
};

struct pcmprint {
  int rate;
  int duration;
  int finishc;
  int16_t qlevel;
  float *qbuf;
  int qbufa;
  struct pcmprint_op **opv;
  int opc,opa;
  struct pcmprint_wave **wavev;
  int wavec,wavea;
};

int pcmprint_decode_program(struct pcmprint *pcmprint,const uint8_t *src,int srcc);

void pcmprint_op_del(struct pcmprint_op *op);
struct pcmprint_op *pcmprint_op_new(struct pcmprint *pcmprint,const struct pcmprint_op_type *type);
int pcmprint_op_decode_all(struct pcmprint_op *op,const uint8_t *src,int srcc);

struct pcmprint_env {
  int c;
  float v;
  float dv;
  int pointp;
  struct pcmprint_env_point {
    int t;
    float v;
  } *pointv;
  int pointc,pointa;
};

void pcmprint_env_cleanup(struct pcmprint_env *env);
int pcmprint_env_decode(struct pcmprint_env *env,const uint8_t *src,int srcc,int rate);
void pcmprint_env_advance(struct pcmprint_env *env);

// By default, output values are in 0..1
void pcmprint_env_scale(struct pcmprint_env *env,float scale);

static inline float pcmprint_env_next(struct pcmprint_env *env) {
  if (env->c>0) {
    env->c--;
    env->v+=env->dv;
    return env->v;
  }
  pcmprint_env_advance(env);
  return env->v;
}

struct pcmprint_wave *pcmprint_wave_decode(struct pcmprint *pcmprint,const uint8_t *serial,int serialc);

#define PCMPRINT_OSCILLATOR_IV_SIZE 8
#define PCMPRINT_OSCILLATOR_FV_SIZE 8

struct pcmprint_oscillator {
  void (*update)(float *v,int c,struct pcmprint_oscillator *osc,uint32_t arate,uint32_t zrate);
  struct pcmprint_wave *wave; // WEAK
  uint32_t p;
  int iv[PCMPRINT_OSCILLATOR_IV_SIZE];
  float fv[PCMPRINT_OSCILLATOR_FV_SIZE];
};

int pcmprint_oscillator_decode(struct pcmprint_oscillator *osc,const uint8_t *src,int srcc,struct pcmprint *pcmprint);

#endif

#include "mkd_internal.h"
#include "mkd_instrument_format.h"

/* Instrument context.
 */
 
struct mkd_stdsyn_instrument {
  const char *path;
  int lineno;
  int id;
};

#define INS ((struct mkd_stdsyn_instrument*)instrument)

static void _instrument_del(void *instrument) {
  free(instrument);
}

static void *_instrument_new(int id,const char *path,int lineno) {
  void *instrument=calloc(1,sizeof(struct mkd_stdsyn_instrument));
  if (!instrument) return 0;
  INS->path=path;
  INS->lineno=lineno;
  INS->id=id;
  return instrument;
}

/* Instrument input.
 */

static int _instrument_line(void *instrument,const char *kw,int kwc,const char *src,int srcc,const char *path,int lineno) {
  //TODO
  return 0;
}

/* Instrument output.
 */

static int _instrument_encode(struct sr_encoder *dst,void *instrument) {
  //TODO
  return 0;
}

/* Sound context.
 */
 
struct mkd_stdsyn_sound {
  const char *path;
  int lineno;
  int id;
};

#define SND ((struct mkd_stdsyn_sound*)sound)

static void _sound_del(void *sound) {
  free(sound);
}

static void *_sound_new(int id,const char *path,int lineno) {
  void *sound=calloc(1,sizeof(struct mkd_stdsyn_sound));
  if (!sound) return 0;
  SND->path=path;
  SND->lineno=lineno;
  SND->id=id;
  return sound;
}

/* Sound input.
 */

static int _sound_line(void *sound,const char *kw,int kwc,const char *src,int srcc,const char *path,int lineno) {
  //TODO
  return 0;
}

/* Sound output.
 */

static int _sound_encode(struct sr_encoder *dst,void *sound) {
  //TODO
  return 0;
}

/* Type definition.
 */
 
const struct mkd_instrument_format mkd_instrument_format_stdsyn={
  .name="stdsyn",
  .instrument_del=_instrument_del,
  .instrument_new=_instrument_new,
  .instrument_line=_instrument_line,
  .instrument_encode=_instrument_encode,
  .sound_del=_sound_del,
  .sound_new=_sound_new,
  .sound_line=_sound_line,
  .sound_encode=_sound_encode,
};


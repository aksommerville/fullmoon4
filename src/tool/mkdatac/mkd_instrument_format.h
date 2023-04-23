/* mkd_instrument_format.h
 * Generic interface for instrument and sound processors.
 * An upper layer manages the framing, and format manages individual instructions.
 */
 
#ifndef MKD_INSTRUMENT_FORMAT_H
#define MKD_INSTRUMENT_FORMAT_H

struct sr_encoder;

struct mkd_instrument_format {
  const char *name;
  
  void (*instrument_del)(void *instrument);
  void *(*instrument_new)(int id,const char *path,int lineno);
  int (*instrument_line)(void *instrument,const char *kw,int kwc,const char *src,int srcc,const char *path,int lineno);
  int (*instrument_encode)(struct sr_encoder *dst,void *instrument);
  
  void (*sound_del)(void *sound);
  void *(*sound_new)(int id,const char *path,int lineno);
  int (*sound_line)(void *sound,const char *kw,int kwc,const char *src,int srcc,const char *path,int lineno);
  int (*sound_encode)(struct sr_encoder *dst,void *sound);
};

extern const struct mkd_instrument_format mkd_instrument_format_WebAudio;
extern const struct mkd_instrument_format mkd_instrument_format_minsyn;
extern const struct mkd_instrument_format mkd_instrument_format_stdsyn;

#endif

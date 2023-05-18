#include "http_internal.h"

/* Method.
 */
 
const char *http_method_repr(int method) {
  switch (method) {
    #define _(tag) case HTTP_METHOD_##tag: return #tag;
    _(GET)
    _(POST)
    _(PUT)
    _(PATCH)
    _(DELETE)
    _(HEAD)
    #undef _
  }
  return "?";
}

/* URL encoding.
 */
 
int http_url_encode(char *dst,int dsta,const char *src,int srcc) {
  return -1;//TODO
}

int http_url_decode(char *dst,int dsta,const char *src,int srcc) {
  return -1;//TODO
}

/* Integers.
 */
 
int http_int_eval(int *dst,const char *src,int srcc) {
  return -1;//TODO
}

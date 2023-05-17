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

#include "datan_internal.h"

/* Validate string.
 */
 
int datan_string_validate_serial(uint16_t qualifier,uint32_t id,const void *v,int c) {
  //TODO Assert UTF-8? Or actually, are we going to encode to a private 8-bit format at compile time?
  return 0;
}

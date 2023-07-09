#include "datan_internal.h"

/* Confirm that every map is associated with exactly one save point.
 *
 * TODO An exception might be made for Full 65, where the church's basement connects to the castle.
 * If we make such an exception, I guess use a new map command or flag, to tell us it's a known overlap case.
 */
 
int datan_validate_save_points() {
  fprintf(stderr,"TODO %s\n",__func__);
  return 0;
}

#include "fmn_soft_internal.h"

//TODO

/* Cleanup.
 */
 
static void _soft_del(struct bigpc_render_driver *driver) {
}

/* Init.
 */
 
static int _soft_init(struct bigpc_render_driver *driver,struct bigpc_video_driver *video) {
  return 0;
}

/* Update.
 */
 
static int _soft_update(struct bigpc_image *fb,struct bigpc_render_driver *driver) {
  return 0;
}

/* Type.
 */
 
const struct bigpc_render_type bigpc_render_type_soft={
  .name="soft",
  .desc="Pure software rendering, no acceleration.",
  .objlen=sizeof(struct bigpc_render_driver_soft),
  .del=_soft_del,
  .init=_soft_init,
  .update=_soft_update,
};

#include "fmn_gl2_internal.h"

/* Cleanup.
 */
 
static void _gl2_del(struct bigpc_render_driver *driver) {
}

/* Init.
 */
 
static int _gl2_init(struct bigpc_render_driver *driver,struct bigpc_video_driver *video) {
  return 0;
}

/* Update.
 */
 
static int _gl2_update(struct bigpc_image *fb,struct bigpc_render_driver *driver) {
  return 0;
}

/* Type.
 */
 
const struct bigpc_render_type bigpc_render_type_gl2={
  .name="gl2",
  .desc="OpenGL 2+. Recommended.",
  .objlen=sizeof(struct bigpc_render_driver_gl2),
  .del=_gl2_del,
  .init=_gl2_init,
  .update=_gl2_update,
};

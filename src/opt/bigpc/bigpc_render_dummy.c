/* bigpc_render_dummy.c
 * Fake renderer that does nothing.
 */
 
#include "bigpc_render.h"

struct bigpc_render_driver_dummy {
  struct bigpc_render_driver hdr;
};

#define DRIVER ((struct bigpc_render_driver_dummy*)driver)

static int _dummy_update(struct bigpc_image *fb,struct bigpc_render_driver *driver) {
  return 0;
}

const struct bigpc_render_type bigpc_render_type_dummy={
  .name="dummy",
  .desc="Fake renderer that does nothing.",
  .objlen=sizeof(struct bigpc_render_driver_dummy),
  .appointment_only=1,
  .update=_dummy_update,
};

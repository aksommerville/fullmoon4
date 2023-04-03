/* bigpc_video_dummy.c
 * Fake video driver that creates a software-only context and discards content.
 * For automation and such.
 */
 
#include "bigpc_video.h"
#include "bigpc_image.h"

/* Instance definition.
 */
 
struct bigpc_video_driver_dummy {
  struct bigpc_video_driver hdr;
  struct bigpc_image *fb;
};

#define DRIVER ((struct bigpc_video_driver_dummy*)driver)

/* Cleanup.
 */
 
static void _dummy_del(struct bigpc_video_driver *driver) {
  bigpc_image_del(DRIVER->fb);
}

/* Init.
 */
 
static int _dummy_init(struct bigpc_video_driver *driver,const struct bigpc_video_config *config) {
  if (!config) return -1;
  
  int storage=-1,pixfmt=-1;
  switch (config->renderer) {
    case BIGPC_RENDERER_any: driver->renderer=BIGPC_RENDERER_rgb32; storage=BIGPC_IMAGE_STORAGE_32; pixfmt=BIGPC_IMAGE_PIXFMT_XRGB; break;
    case BIGPC_RENDERER_soft: driver->renderer=BIGPC_RENDERER_rgb32; storage=BIGPC_IMAGE_STORAGE_32; pixfmt=BIGPC_IMAGE_PIXFMT_XRGB; break;
    case BIGPC_RENDERER_rgb32: driver->renderer=BIGPC_RENDERER_rgb32; storage=BIGPC_IMAGE_STORAGE_32; pixfmt=BIGPC_IMAGE_PIXFMT_XRGB; break;
    // No problem to add new Soft formats as we define them, as long as it corresponds to some image format.
    default: return -1;
  }
  if (!(DRIVER->fb=bigpc_image_new_alloc(config->fbw,config->fbh,storage,pixfmt))) return -1;
  
  driver->w=DRIVER->fb->w;
  driver->h=DRIVER->fb->h;
  driver->fullscreen=1;
  
  return 0;
}

/* Begin frame.
 */
 
static struct bigpc_image *_dummy_begin_soft(struct bigpc_video_driver *driver) {
  return DRIVER->fb;
}

/* Type definition.
 */
 
const struct bigpc_video_type bigpc_video_type_dummy={
  .name="dummy",
  .desc="Fake driver for automated testing.",
  .objlen=sizeof(struct bigpc_video_driver_dummy),
  .appointment_only=1,
  .del=_dummy_del,
  .init=_dummy_init,
  .begin_soft=_dummy_begin_soft,
};

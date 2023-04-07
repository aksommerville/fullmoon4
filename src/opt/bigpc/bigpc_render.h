/* bigpc_render.h
 * Pluggable rendering units.
 * I expect there will be two implementations: Software and OpenGL 2.
 * Maybe we'll do OpenGL 1? Metal? Vulkan? And maybe specialized software renderers for low-performance systems.
 */
 
#ifndef BIGPC_RENDER_H
#define BIGPC_RENDER_H

struct bigpc_render_driver;
struct bigpc_render_type;
struct bigpc_video_driver;
struct bigpc_image;
struct fmn_datafile;

/* Instance.
 *************************************************************/
 
struct bigpc_render_driver {
  const struct bigpc_render_type *type;
  int refc;
  int w,h; // Provider should update before each render.
  struct fmn_datafile *datafile; // WEAK
};

void bigpc_render_del(struct bigpc_render_driver *driver);
int bigpc_render_ref(struct bigpc_render_driver *driver);

struct bigpc_render_driver *bigpc_render_new(
  const struct bigpc_render_type *type,
  struct bigpc_video_driver *video
);

/* Draw the scene.
 * Driver is expected to read globals (see fmn_platform.h) and render accordingly.
 * See the original implementation, src/www/js/game/Renderer.js.
 * If the video driver is configured for software rendering, (fb) is provided.
 * Otherwise (fb) is null, and we are in the GX context.
 */
int bigpc_render_update(struct bigpc_image *fb,struct bigpc_render_driver *driver);

/* Full Moon specific stuff.
 */
void bigpc_render_map_dirty(struct bigpc_render_driver *driver);

/* Type.
 ********************************************************/
 
struct bigpc_render_type {
  const char *name;
  const char *desc;
  int objlen;
  int appointment_only;
  void (*del)(struct bigpc_render_driver *driver);
  int (*init)(struct bigpc_render_driver *driver,struct bigpc_video_driver *video);
  int (*update)(struct bigpc_image *fb,struct bigpc_render_driver *driver);
  void (*map_dirty)(struct bigpc_render_driver *driver);
};

const struct bigpc_render_type *bigpc_render_type_by_index(int p);
const struct bigpc_render_type *bigpc_render_type_by_name(const char *name,int namec);

#endif

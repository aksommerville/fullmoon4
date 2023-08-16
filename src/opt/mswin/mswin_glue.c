#include "opt/genioc/genioc.h"

/* Preprocess argv.
 * We don't need anything here.
 */
 
int genioc_client_preprocess_argv(int argc,char **argv) {
  return argc;
}

/* XXX TEMP dummy drivers, just to get it booting.
 */
 
#include "opt/bigpc/bigpc_input.h"

struct bigpc_input_driver_mshid {
  struct bigpc_input_driver hdr;
};

static void _mshid_del(struct bigpc_input_driver *driver) {
}

static int _mshid_init(struct bigpc_input_driver *driver) {
  return 0;
}

static int _mshid_update(struct bigpc_input_driver *driver) {
  return 0;
}

static const char *_mshid_get_ids(uint16_t *vid,uint16_t *pid,struct bigpc_input_driver *driver,int devid) {
  return 0;
}

static int _mshid_for_each_button(
  struct bigpc_input_driver *driver,
  int devid,
  int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
) {
  return 0;
}

static int _mshid_for_each_device(
  struct bigpc_input_driver *driver,
  int (*cb)(struct bigpc_input_driver *driver,int devid,void *userdata),
  void *userdata
) {
  return 0;
}

const struct bigpc_input_type bigpc_input_type_mshid={
  .name="mshid",
  .desc="Joystick input for Windows",
  .objlen=sizeof(struct bigpc_input_driver_mshid),
  .del=_mshid_del,
  .init=_mshid_init,
  .update=_mshid_update,
  .get_ids=_mshid_get_ids,
  .for_each_button=_mshid_for_each_button,
  .for_each_device=_mshid_for_each_device,
};

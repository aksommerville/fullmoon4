/* bigpc_input.h
 * Generic interface for input drivers.
 */
 
#ifndef BIGPC_INPUT_H
#define BIGPC_INPUT_H

#include <stdint.h>

struct bigpc_input_driver;
struct bigpc_input_type;
struct bigpc_input_delegate;

struct bigpc_input_delegate {
  void *userdata;
  void (*cb_connect)(struct bigpc_input_driver *driver,int devid);
  void (*cb_disconnect)(struct bigpc_input_driver *driver,int devid);
  void (*cb_event)(struct bigpc_input_driver *driver,int devid,int btnid,int value);
};

/* Driver instance.
 *********************************************************/
 
struct bigpc_input_driver {
  const struct bigpc_input_type *type;
  struct bigpc_input_delegate delegate;
  int refc;
};

void bigpc_input_driver_del(struct bigpc_input_driver *driver);

struct bigpc_input_driver *bigpc_input_driver_new(
  const struct bigpc_input_type *type,
  const struct bigpc_input_delegate *delegate
);

/* Delegate callbacks should only fire during update.
 */
int bigpc_input_driver_update(struct bigpc_input_driver *driver);

const char *bigpc_input_device_get_ids(
  uint16_t *vid,uint16_t *pid,
  struct bigpc_input_driver *driver,
  int devid
);

int bigpc_input_device_for_each_button(
  struct bigpc_input_driver *driver,
  int devid,
  int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
);

/* Type.
 *********************************************************/
 
struct bigpc_input_type {
  const char *name;
  const char *desc;
  int objlen;
  int appointment_only;
  void (*del)(struct bigpc_input_driver *driver);
  int (*init)(struct bigpc_input_driver *driver);
  int (*update)(struct bigpc_input_driver *driver);
  const char *(*get_ids)(uint16_t *vid,uint16_t *pid,struct bigpc_input_driver *driver,int devid);
  int (*for_each_button)(
    struct bigpc_input_driver *driver,
    int devid,
    int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),
    void *userdata
  );
};

const struct bigpc_input_type *bigpc_input_type_by_index(int p);
const struct bigpc_input_type *bigpc_input_type_by_name(const char *name,int namec);

int bigpc_input_devid_next();

#endif

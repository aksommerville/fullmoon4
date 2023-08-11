/* inmgr.h
 * Generic input manager.
 * All devices are first canonicalized as a set of two-state buttons.
 * Those buttons are then mappable to logical buttons and actions.
 * Since Full Moon is a strictly 1-player game, we're not bothering with multiple players.
 * (though logically that would be our domain).
 *
 * There is a global 16-bit state of player input, and a 16-bit set of stateless actions.
 * (btnid) is a single bit.
 * (actionid) is a nonzero integer.
 *
 * Source devices are identified by name or vid/pid for config purposes; and devid for live ones.
 *
 * Mapping decisions, in order of preference:
 *  - First match from the config file (order matters).
 *  - If no declared capabilities and "keyboard" in the name, assume HID page 7 is available.
 *  - Use declared capabilities.
 */
 
#ifndef INMGR_H
#define INMGR_H

#include <stdint.h>

struct inmgr;

struct inmgr_delegate {
  void *userdata;
  uint16_t all_btnid; // Mask of all valid btnid.
  void (*state)(struct inmgr *inmgr,uint16_t btnid,uint8_t value,uint16_t state);
  void (*action)(struct inmgr *inmgr,uint16_t actionid);
  int (*btnid_repr)(char *dst,int dsta,uint16_t btnid);
  uint16_t (*btnid_eval)(const char *src,int srcc);
  int (*actionid_repr)(char *dst,int dsta,uint16_t actionid);
  uint16_t (*actionid_eval)(const char *src,int srcc);
};

/* Global context.
 ******************************************************************************/

void inmgr_del(struct inmgr *inmgr);

struct inmgr *inmgr_new(const struct inmgr_delegate *delegate);

void *inmgr_get_userdata(const struct inmgr *inmgr);
uint16_t inmgr_get_state(const struct inmgr *inmgr);

/* Pretend (state) is the current logical state, until anything changes.
 * This will not fire any events. It causes a possible discontinuity in the states reported to the delegate.
 */
void inmgr_force_state(struct inmgr *inmgr,uint16_t state);

/* You already know about the devices, I mean, you provided them to us at some point.
 * But it might be more convenient to retrieve the list from inmgr than to re-gather it from the drivers.
 */
int inmgr_for_each_device(
  struct inmgr *inmgr,
  int (*cb)(int devid,const char *name,int namec,uint16_t vid,uint16_t pid,void *userdata),
  void *userdata
);

int inmgr_device_index_by_devid(const struct inmgr *inmgr,int devid);
int inmgr_device_devid_by_index(const struct inmgr *inmgr,int index);

uint8_t inmgr_device_name_by_index(char *dst,int dsta,const struct inmgr *inmgr,int p);

/* Input processing.
 *******************************************************************************/

/* Pass all input events here, just as the drivers provide to you.
 */
void inmgr_event(struct inmgr *inmgr,int devid,int btnid,int value);

/* Notify of a new device available.
 * Call 'connect', then multiple 'add_capability', then 'ready'.
 * Mapping is finalized during 'ready'.
 * inmgr_ready() returns >0 if it added new rules; you might like to serialize and save the config then.
 */
void inmgr_connect(struct inmgr *inmgr,int devid,const char *name,int namec,uint16_t vid,uint16_t pid);
void inmgr_add_capability(struct inmgr *inmgr,int btnid,int usage,int lo,int hi,int rest);
int inmgr_ready(struct inmgr *inmgr);

/* Drop any state and mappings associated with this device.
 */
void inmgr_disconnect(struct inmgr *inmgr,int devid);

/* Config file.
 ******************************************************************************/

/* Drop configuration for all rules matching this ID.
 * If you provide (0,0,0,0) we drop everything.
 */
void inmgr_drop_configuration(struct inmgr *inmgr,const char *name,int namec,uint16_t vid,uint16_t pid);

/* Provide the config file.
 * Optionally we can report errors via (cb_error).
 * You should decorate the message, eg: fprintf(stderr,"%s:%d: %.*s\n",my_file_name,lineno,msgc,msg);
 * New rules from the file are appended to existing rules -- you probably want to drop first.
 */
int inmgr_receive_config(
  struct inmgr *inmgr,
  const char *src,int srcc,
  void (*cb_error)(const char *msg,int msgc,int lineno,void *userdata),
  void *userdata
);

/* Generate a replacement for the config file.
 * Any comments and formatting are lost.
 */
int inmgr_generate_config(void *dstpp,const struct inmgr *inmgr);

/* Live configuration.
 * This is a special state where you are prompting the user to configure each button on one device.
 * Any existing maps for the device are dropped at begin.
 * We generate new permanent rules. You should inmgr_generate_config and save that after ending.
 * Other devices continue to operate as usual.
 ************************************************************************/
 
void inmgr_live_config_begin(struct inmgr *inmgr,int devid);
void inmgr_live_config_end(struct inmgr *inmgr);

/* Live-config state changes happen during the usual event processing.
 * You should poll us each cycle to determine whether live-config is running.
 */
int inmgr_live_config_status(uint16_t *btnid,const struct inmgr *inmgr);
#define INMGR_LIVE_CONFIG_NONE 0 /* Not running. */
#define INMGR_LIVE_CONFIG_FAULT 1 /* Notify that we got something unexpected, otherwise same as PRESS state. */
#define INMGR_LIVE_CONFIG_PRESS 2 /* Press the given button, first time. */
#define INMGR_LIVE_CONFIG_RELEASE 3 /* Got a button, waiting to release it. */
#define INMGR_LIVE_CONFIG_PRESS_AGAIN 4 /* Please press it again to confirm. */
#define INMGR_LIVE_CONFIG_RELEASE_AGAIN 5 /* And release that... */
#define INMGR_LIVE_CONFIG_SUCCESS 6 /* All inputs collected. You should end and save it. */

/* Call while live config is running, normally when it enters SUCCESS state.
 * This drops any rules for the configging device and generates a new rules set based on live mappings.
 */
int inmgr_rewrite_rules_for_live_config(struct inmgr *inmgr);

#endif

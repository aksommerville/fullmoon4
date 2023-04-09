/* inmgr.h
 * Generic input mapping.
 * The key concepts:
 *   - You supply raw input events and we call back with digested states and actions.
 *   - Inputs are associated with a nonzero "playerid".
 *   - playerid zero is the aggregate state of all players.
 *   - In addition to the per-player and aggregate states, there are stateless "actions".
 *   - States are 16 independent bits.
 *   - Actions are a 16-bit identifier.
 * This is fully generic. I'm writing it for Full Moon, but <-- that is the only time you'll hear "Full Moon". (and also <-- that one).
 */
 
#ifndef INMGR_H
#define INMGR_H

#include <stdint.h>

struct inmgr;

#define INMGR_PLAYER_LIMIT 15 /* We'll never use a playerid greater than this. (this doesn't count player zero). */

struct inmgr_delegate {
  void *userdata;
  
  /* Tell us which bits are the dpad, so we can handle axes and hats appropriately.
   * These may all be zero, otherwise they must each be a unique bit.
   */
  uint16_t btnid_left,btnid_right,btnid_up,btnid_down;
  
  /* If a device is missing any of these bits after mapping, it won't participate.
   */
  uint16_t btnid_required;
  
  /* Notification of a state change on some player.
   * You will not get redundant notifications.
   * State changes on a nonzero playerid will typically be followed by the same state change on player zero.
   * (but again, not always, if player zero was already in that state).
   */
  void (*state_change)(void *userdata,uint8_t playerid,uint16_t btnid,uint8_t value,uint16_t state);
  
  /* Notification of a stateless action.
   * (playerid) is nonzero if it came from a device mapped to just one player.
   */
  void (*action)(void *userdata,uint8_t playerid,uint16_t actionid);
  
  /* Hooks to manage your buttons and actions as text, for config files.
   * (btnid) generally should be a single bit, but (left|right), (up|down), (left|right|up|down) are also permitted.
   */
  uint16_t (*btnid_eval)(void *userdata,const char *src,int srcc);
  uint16_t (*actionid_eval)(void *userdata,const char *src,int srcc);
  int (*btnid_repr)(char *dst,int dsta,void *userdata,uint16_t btnid);
  int (*actionid_repr)(char *dst,int dsta,void *userdata,uint16_t actionid);
};

/* Global context.
 **********************************************************************/

void inmgr_del(struct inmgr *inmgr);

/* New unconfigured inmgr.
 * Both delegate hooks (state_change,action) are required.
 */
struct inmgr *inmgr_new(const struct inmgr_delegate *delegate);

/* Apply a config file.
 * If (refname) set, we will log errors to stderr. (even if it's empty).
 */
int inmgr_configure_text(struct inmgr *inmgr,const char *src,int srcc,const char *refname);

/* Call once after applying all config, and before any events or connections.
 */
int inmgr_ready(struct inmgr *inmgr);

/* Call for all state changes on source devices.
 * Returns nonzero if the event is mapped. Zero, you might try other fallback behavior?
 */
int inmgr_event(struct inmgr *inmgr,int devid,int btnid,int value);

/* Force the given bits of the given player on or off, without firing callbacks or syncing player zero.
 * There are situations where you'll want to do this to ignore the next event.
 */
void inmgr_force_input_state(struct inmgr *inmgr,uint8_t playerid,uint16_t mask,uint8_t value);

/* Device handshake and farewell.
 **************************************************************/

/* You must identify source devices by a unique positive "devid".
 * To inform inmgr of a new device, call inmgr_device_connect.
 * Immediately after that, you should call inmgr_device_capability for each button on it, then inmgr_device_ready.
 * If you don't "ready" a device, it will not participate in mapping.
 *
 * Currently, only one device at a time is permitted to be in connected-but-not-ready state.
 */
void inmgr_device_disconnect(struct inmgr *inmgr,int devid);
int inmgr_device_connect(struct inmgr *inmgr,int devid,uint16_t vid,uint16_t pid,const char *name,int namec);
void inmgr_device_capability(struct inmgr *inmgr,int devid,int btnid,uint32_t usage,int lo,int hi,int value);
int inmgr_device_ready(struct inmgr *inmgr,int devid);

/* Equivalent to "connect ... dozens of capabilities ... ready", but we can guess the capabilities.
 * A device connected this way is mapped like any other, with vid=0 pid=0 name="System Keyboard".
 * (btnid) are presumed to be fully qualified USB-HID, page 7, eg 0x00070004 is A.
 */
int inmgr_connect_system_keyboard(struct inmgr *inmgr,int devid);

/* Live reconfiguration.
 **************************************************************/
 
//TODO inmgr support for live reconfiguration.

#endif

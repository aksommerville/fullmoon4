/* !!! mswm must be initialized first !!!
 */

#ifndef MSHID_INTERNAL_H
#define MSHID_INTERNAL_H

#include "opt/bigpc/bigpc_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <hidsdi.h>

#define PS_MSHID_HIDP_HEADER_SIZE 36
#define PS_MSHID_HIDP_USAGE_SIZE 104

#define PS_MSHID_HIDP_REPORT_COUNT_SANITY_LIMIT 256
#define PS_MSHID_HIDP_REPORT_SIZE_SANITY_LIMIT 32

#define RD16(src,p) ((((uint8_t*)(src))[p])|(((uint8_t*)(src))[p+1]<<8))
#define RD32(src,p) (RD16(src,p)|(RD16(src,p+2)<<16))

struct bigpc_input_driver_mshid {
  struct bigpc_input_driver hdr;
  int poll_soon;
};

#define DRIVER ((struct bigpc_input_driver_mshid*)driver)

extern struct bigpc_input_driver *mshid_global;

/* These two are called by mswm.
 */
void mshid_event(int wparam,int lparam);
void mshid_poll_connections();
void mshid_poll_connections_later();

/* Borrowed from mswm.
 */
HWND mswm_get_window_handle();

#endif

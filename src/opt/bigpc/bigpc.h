/* bigpc.h
 * Generic platform for reasonably capable targets, should work for all 21st-century PCs.
 * We implement all the Full Moon stuff, and connect to the target via generic driver interfaces defined here.
 * The only reasons *not* to use bigpc:
 *  - Severely limited platform (thinking PowerPC Mac...)
 *  - Highly specific platform, no need for generic interfaces (Thumby?)
 *  - Non-native components in platform logic (Web, Java...)
 * If this works out to plan, writing new platforms should be only a matter of highly-focused platform API glue.
 */
 
#ifndef BIGPC_H
#define BIGPC_H

#include <stdint.h>

struct bigpc_video_driver;
struct bigpc_audio_driver;
struct bigpc_input_driver;
struct bigpc_synth_driver;
struct bigpc_render_driver;
struct bigpc_menu;

void bigpc_quit();
int bigpc_init(int argc,char **argv);

/* Returns zero to request termination, <0 on real errors, or >0 to proceed.
 */
int bigpc_update();

/* Return the active menu if there is one.
 * For now I'm assuming we will not render downstack menus. If that changes, we could add an index here.
 */
struct bigpc_menu *bigpc_get_menu();

uint32_t bigpc_get_game_time_ms();

#endif

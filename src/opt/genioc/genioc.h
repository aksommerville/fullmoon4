/* genioc.h
 * Generic Inversion of Control API.
 * For typical platforms using bigpc.
 * We supply main().
 * MacOS should not use this, nor other platforms with their own IoC APIs.
 */
 
#ifndef GENIOC_H
#define GENIOC_H

/* Client must implement.
 * You have an opportunity to rewrite argv before genioc or bigpc examine it.
 * For special platform-specific config that you couldn't express generically.
 * Return (argc).
 */
int genioc_client_preprocess_argv(int argc,char **argv);

/* Request that the main loop break when convenient.
 * This call *does* return; it does not terminate the program immediately.
 */
void genioc_quit_soon();

/* Default is 60, client may change at any time.
 * <=0 to disable genioc's clock and run at maximum speed.
 */
void genioc_set_frame_rate(int hz);

#endif

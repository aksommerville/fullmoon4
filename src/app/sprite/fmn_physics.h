/* fmn_physics.h
 */
 
#ifndef FMN_PHYSICS_H
#define FMN_PHYSICS_H

struct fmn_sprite;

/* All behave the same way.
 * Return nonzero if a collision exists, and populate (cx,cy) with a correction you could apply to (a) to escape it.
 * Caller may supply null for (cx,cy) if not interested. Might be more efficient without.
 */
uint8_t fmn_physics_check_edges(float *cx,float *cy,const struct fmn_sprite *a);
uint8_t fmn_physics_check_grid(float *cx,float *cy,const struct fmn_sprite *a,uint8_t features);
uint8_t fmn_physics_check_sprites(float *cx,float *cy,const struct fmn_sprite *a,const struct fmn_sprite *b);

#endif

#include "fmn_gl2_internal.h"

/* GLSL.
 */
 
static const char fmn_gl2_vsrc_mintile[]=
"uniform vec2 screensize;\n"
"uniform vec2 texsize;\n"
"attribute vec2 apos;\n"
"attribute float atileid;\n"
"varying vec2 vtexcoord;\n" // upper-left corner, normalized
"void main() {\n"
"  gl_Position=vec4((apos.x*2.0)/screensize.x-1.0,1.0-(apos.y*2.0)/screensize.y,0.0,1.0);\n"
"  gl_PointSize=texsize.x/16.0;\n"
"  vtexcoord.x=floor(mod(atileid,16.0))/16.0;\n"
"  vtexcoord.y=floor(atileid/16.0)/16.0;\n"
"}\n";

static const char fmn_gl2_fsrc_mintile[]=
"uniform vec2 texsize;\n"
"uniform sampler2D sampler;\n"
"varying vec2 vtexcoord;\n"
"void main() {\n"
"  gl_FragColor=texture2D(sampler,vtexcoord+gl_PointCoord/16.0);\n"
"}\n";

/* Init.
 */
 
int fmn_gl2_program_mintile_init(struct fmn_gl2_program *program,struct bigpc_render_driver *driver) {
  int err=fmn_gl2_program_init(program,"mintile",fmn_gl2_vsrc_mintile,sizeof(fmn_gl2_vsrc_mintile),fmn_gl2_fsrc_mintile,sizeof(fmn_gl2_fsrc_mintile));
  if (err<0) return err;
  return 0;
}

/* Draw.
 */
 
void fmn_gl2_draw_mintile(const struct fmn_gl2_vertex_mintile *vtxv,int vtxc) {
  if (vtxc<1) return;
  glEnable(GL_POINT_SPRITE);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glVertexAttribPointer(0,2,GL_SHORT,0,sizeof(struct fmn_gl2_vertex_mintile),&vtxv->x);
  glVertexAttribPointer(1,1,GL_UNSIGNED_BYTE,0,sizeof(struct fmn_gl2_vertex_mintile),&vtxv->tileid);
  glDrawArrays(GL_POINTS,0,vtxc);
}

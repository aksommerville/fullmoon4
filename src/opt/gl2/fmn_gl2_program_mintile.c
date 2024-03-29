#include "fmn_gl2_internal.h"

/* GLSL.
 */
 
static const char fmn_gl2_vsrc_mintile[]=
"uniform vec2 screensize;\n"
"uniform vec2 screenoffset;\n"
"uniform vec2 texsize;\n"
"attribute vec2 apos;\n"
"attribute float atileid;\n"
"attribute float axform;\n"
"varying vec2 vtexcoord;\n" // upper-left corner, normalized
"varying vec2 vxformx;\n"
"varying vec2 vxformy;\n"
"void main() {\n"
"  vec2 pos=apos+screenoffset;\n"
"  gl_Position=vec4((pos.x*2.0)/screensize.x-1.0,1.0-(pos.y*2.0)/screensize.y,0.0,1.0);\n"
"  gl_PointSize=texsize.x/16.0;\n"
"  vtexcoord.x=floor(mod(atileid+0.5,16.0))/16.0;\n"
"  vtexcoord.y=floor((atileid+0.5)/16.0)/16.0;\n"

"       if (axform<0.5) { vxformx=vec2( 1.0, 0.0); vxformy=vec2( 0.0, 1.0); }\n"
"  else if (axform<1.5) { vxformx=vec2(-1.0, 0.0); vxformy=vec2( 0.0, 1.0); }\n"
"  else if (axform<2.5) { vxformx=vec2( 1.0, 0.0); vxformy=vec2( 0.0,-1.0); }\n"
"  else if (axform<3.5) { vxformx=vec2(-1.0, 0.0); vxformy=vec2( 0.0,-1.0); }\n"
"  else if (axform<4.5) { vxformx=vec2( 0.0, 1.0); vxformy=vec2( 1.0, 0.0); }\n"
"  else if (axform<5.5) { vxformx=vec2( 0.0,-1.0); vxformy=vec2( 1.0, 0.0); }\n"
"  else if (axform<6.5) { vxformx=vec2( 0.0, 1.0); vxformy=vec2(-1.0, 0.0); }\n"
"  else                 { vxformx=vec2( 0.0,-1.0); vxformy=vec2(-1.0, 0.0); }\n"
"}\n";

static const char fmn_gl2_fsrc_mintile[]=
"uniform vec2 texsize;\n"
"uniform sampler2D sampler;\n"
"varying vec2 vtexcoord;\n"
"varying vec2 vxformx;\n"
"varying vec2 vxformy;\n"
"void main() {\n"

"  vec2 texcoord=gl_PointCoord-0.5;\n"
"  texcoord=vec2(texcoord.x*vxformx.x+texcoord.y*vxformx.y,texcoord.x*vxformy.x+texcoord.y*vxformy.y);\n"
"  texcoord+=0.5;\n"
"  if ((texcoord.x<0.0)||(texcoord.x>=1.0)||(texcoord.y<0.0)||(texcoord.y>=1.0)) discard;\n"
"  texcoord/=16.0;\n"
"  texcoord+=vtexcoord;\n"

"  gl_FragColor=texture2D(sampler,texcoord);\n"
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
  glVertexAttribPointer(2,1,GL_UNSIGNED_BYTE,0,sizeof(struct fmn_gl2_vertex_mintile),&vtxv->xform);
  glDrawArrays(GL_POINTS,0,vtxc);
}

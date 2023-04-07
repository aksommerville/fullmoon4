#include "fmn_gl2_internal.h"

/* GLSL.
 */
 
static const char fmn_gl2_raw_vsrc[]=
"uniform vec2 screensize;\n"
"attribute vec2 apos;\n"
"attribute vec4 acolor;\n"
"varying vec4 vcolor;\n"
"void main() {\n"
"  gl_Position=vec4((apos.x*2.0)/screensize.x-1.0,1.0-(apos.y*2.0)/screensize.y,0.0,1.0);\n"
"  vcolor=acolor;\n"
"}\n";

static const char fmn_gl2_raw_fsrc[]=
"varying vec4 vcolor;\n"
"void main() {\n"
"  gl_FragColor=vcolor;\n"
"}\n";

/* Init.
 */
 
int fmn_gl2_program_raw_init(struct fmn_gl2_program *program,struct bigpc_render_driver *driver) {
  int err;
  if ((err=fmn_gl2_program_init(program,"raw",fmn_gl2_raw_vsrc,sizeof(fmn_gl2_raw_vsrc),fmn_gl2_raw_fsrc,sizeof(fmn_gl2_raw_fsrc)))<0) return err;
  return 0;
}

/* Draw.
 */
 
void fmn_gl2_draw_raw(int mode,const struct fmn_gl2_vertex_raw *vtxv,int vtxc) {
  if (vtxc<1) return;
  glVertexAttribPointer(0,2,GL_SHORT,0,sizeof(struct fmn_gl2_vertex_raw),&vtxv->x);
  glVertexAttribPointer(1,4,GL_UNSIGNED_BYTE,1,sizeof(struct fmn_gl2_vertex_raw),&vtxv->r);
  glDrawArrays(mode,0,vtxc);
}

/* Conveniences.
 */
 
void fmn_gl2_draw_raw_rect(int x,int y,int w,int h,uint32_t rgba) {
  uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8,a=rgba;
  struct fmn_gl2_vertex_raw vtxv[]={
    {x  ,y  ,r,g,b,a},
    {x  ,y+h,r,g,b,a},
    {x+w,y  ,r,g,b,a},
    {x+w,y+h,r,g,b,a},
  };
  fmn_gl2_draw_raw(GL_TRIANGLE_STRIP,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
}

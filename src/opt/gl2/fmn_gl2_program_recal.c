/* fmn_gl2_program_recal.c
 * Same as "decal", but we discard the texture's RGB and replace with one provided by caller.
 */

#include "fmn_gl2_internal.h"

struct fmn_gl2_vertex_recal {
  GLshort x,y;
  GLshort tx,ty;
};

/* GLSL.
 */
 
static const char fmn_gl2_vsrc_recal[]=
"uniform vec2 screensize;\n"
"uniform vec2 texsize;\n"
"attribute vec2 apos;\n"
"attribute vec2 atexcoord;\n"
"varying vec2 vtexcoord;\n"
"void main() {\n"
"  gl_Position=vec4((apos.x*2.0)/screensize.x-1.0,1.0-(apos.y*2.0)/screensize.y,0.0,1.0);\n"
"  vtexcoord=atexcoord/texsize;\n"
"}\n";

static const char fmn_gl2_fsrc_recal[]=
"uniform sampler2D sampler;\n"
"uniform vec4 ucolor;\n"
"varying vec2 vtexcoord;\n"
"void main() {\n"
"  gl_FragColor=vec4(ucolor.rgb,ucolor.a*texture2D(sampler,vtexcoord).a);\n"
"}\n";

/* Init.
 */
 
int fmn_gl2_program_recal_init(struct fmn_gl2_program *program,struct bigpc_render_driver *driver) {
  int err=fmn_gl2_program_init(program,"recal",fmn_gl2_vsrc_recal,sizeof(fmn_gl2_vsrc_recal),fmn_gl2_fsrc_recal,sizeof(fmn_gl2_fsrc_recal));
  if (err<0) return err;
  program->locv[0]=glGetUniformLocation(program->programid,"ucolor");
  return 0;
}

/* Flip vertical input coordinates.
 */
 
static void fmn_gl2_recal_flip(struct fmn_gl2_vertex_recal *vtx,int h) {
  int i=4; for (;i-->0;vtx++) {
    vtx->ty=h-vtx->ty;
  }
}

/* Render.
 */
 
void fmn_gl2_draw_recal(
  struct bigpc_render_driver *driver,
  struct fmn_gl2_program *program,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch,
  uint32_t rgba
) {
  if (!DRIVER->texture) return;
  glUniform4f(program->locv[0],(rgba>>24)/255.0f,((rgba>>16)&0xff)/255.0f,((rgba>>8)&0xff)/255.0f,(rgba&0xff)/255.0f);
  struct fmn_gl2_vertex_recal vtxv[]={
    {dstx     ,dsty     ,srcx     ,srcy     },
    {dstx     ,dsty+dsth,srcx     ,srcy+srch},
    {dstx+dstw,dsty     ,srcx+srcw,srcy     },
    {dstx+dstw,dsty+dsth,srcx+srcw,srcy+srch},
  };
  if (DRIVER->texture->fbid&&DRIVER->framebuffer) {
    fmn_gl2_recal_flip(vtxv,DRIVER->texture->h);
  }
  glVertexAttribPointer(0,2,GL_SHORT,0,sizeof(struct fmn_gl2_vertex_recal),&vtxv[0].x);
  glVertexAttribPointer(1,2,GL_SHORT,0,sizeof(struct fmn_gl2_vertex_recal),&vtxv[0].tx);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

void fmn_gl2_draw_recal_swap(
  struct bigpc_render_driver *driver,
  struct fmn_gl2_program *program,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch,
  uint32_t rgba
) {
  if (!DRIVER->texture) return;
  glUniform4f(program->locv[0],(rgba>>24)/255.0f,((rgba>>16)&0xff)/255.0f,((rgba>>8)&0xff)/255.0f,(rgba&0xff)/255.0f);
  struct fmn_gl2_vertex_recal vtxv[]={
    {dstx     ,dsty     ,srcx     ,srcy     },
    {dstx     ,dsty+dsth,srcx+srcw,srcy     },
    {dstx+dstw,dsty     ,srcx     ,srcy+srch},
    {dstx+dstw,dsty+dsth,srcx+srcw,srcy+srch},
  };
  if (DRIVER->texture->fbid&&DRIVER->framebuffer) {
    fmn_gl2_recal_flip(vtxv,DRIVER->texture->h);
  }
  glVertexAttribPointer(0,2,GL_SHORT,0,sizeof(struct fmn_gl2_vertex_recal),&vtxv[0].x);
  glVertexAttribPointer(1,2,GL_SHORT,0,sizeof(struct fmn_gl2_vertex_recal),&vtxv[0].tx);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

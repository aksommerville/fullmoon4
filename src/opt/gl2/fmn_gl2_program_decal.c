#include "fmn_gl2_internal.h"

struct fmn_gl2_vertex_decal {
  GLshort x,y;
  GLshort tx,ty;
};

/* GLSL.
 */
 
static const char fmn_gl2_vsrc_decal[]=
"uniform vec2 screensize;\n"
"uniform vec2 texsize;\n"
"attribute vec2 apos;\n"
"attribute vec2 atexcoord;\n"
"varying vec2 vtexcoord;\n"
"void main() {\n"
"  gl_Position=vec4((apos.x*2.0)/screensize.x-1.0,1.0-(apos.y*2.0)/screensize.y,0.0,1.0);\n"
"  vtexcoord=atexcoord/texsize;\n"
"}\n";

static const char fmn_gl2_fsrc_decal[]=
"uniform sampler2D sampler;\n"
"varying vec2 vtexcoord;\n"
"void main() {\n"
"  gl_FragColor=texture2D(sampler,vtexcoord);\n"
"}\n";

/* Init.
 */
 
int fmn_gl2_program_decal_init(struct fmn_gl2_program *program,struct bigpc_render_driver *driver) {
  int err=fmn_gl2_program_init(program,"decal",fmn_gl2_vsrc_decal,sizeof(fmn_gl2_vsrc_decal),fmn_gl2_fsrc_decal,sizeof(fmn_gl2_fsrc_decal));
  return err;
}

/* Flip vertical input coordinates.
 */
 
static void fmn_gl2_decal_flip(struct fmn_gl2_vertex_decal *vtx,int h) {
  int i=4; for (;i-->0;vtx++) {
    vtx->ty=h-vtx->ty;
  }
}

/* Render.
 */
 
void fmn_gl2_draw_decal(
  struct bigpc_render_driver *driver,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch
) {
  if (!DRIVER->texture) return;
  struct fmn_gl2_vertex_decal vtxv[]={
    {dstx     ,dsty     ,srcx     ,srcy     },
    {dstx     ,dsty+dsth,srcx     ,srcy+srch},
    {dstx+dstw,dsty     ,srcx+srcw,srcy     },
    {dstx+dstw,dsty+dsth,srcx+srcw,srcy+srch},
  };
  if (DRIVER->texture->fbid&&DRIVER->framebuffer) {
    fmn_gl2_decal_flip(vtxv,DRIVER->texture->h);
  }
  glVertexAttribPointer(0,2,GL_SHORT,0,sizeof(struct fmn_gl2_vertex_decal),&vtxv[0].x);
  glVertexAttribPointer(1,2,GL_SHORT,0,sizeof(struct fmn_gl2_vertex_decal),&vtxv[0].tx);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

void fmn_gl2_draw_decal_swap(
  struct bigpc_render_driver *driver,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch
) {
  if (!DRIVER->texture) return;
  struct fmn_gl2_vertex_decal vtxv[]={
    {dstx     ,dsty     ,srcx     ,srcy     },
    {dstx     ,dsty+dsth,srcx+srcw,srcy     },
    {dstx+dstw,dsty     ,srcx     ,srcy+srch},
    {dstx+dstw,dsty+dsth,srcx+srcw,srcy+srch},
  };
  if (DRIVER->texture->fbid&&DRIVER->framebuffer) {
    fmn_gl2_decal_flip(vtxv,DRIVER->texture->h);
  }
  glVertexAttribPointer(0,2,GL_SHORT,0,sizeof(struct fmn_gl2_vertex_decal),&vtxv[0].x);
  glVertexAttribPointer(1,2,GL_SHORT,0,sizeof(struct fmn_gl2_vertex_decal),&vtxv[0].tx);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

#include "fmn_gl2_internal.h"

/* GLSL.
 */
 
static const char fmn_gl2_vsrc_maxtile[]=
"uniform vec2 screensize;\n"
"uniform vec2 texsize;\n"
"attribute vec2 apos;\n"
"attribute float atileid;\n"
"attribute float arotate;\n"
"attribute float asize;\n"
"attribute float axform;\n"
"attribute vec4 atint;\n"
"attribute vec4 aprimary;\n"
"varying vec2 vtexcoord;\n" // upper-left corner, normalized
"varying vec2 vxformx;\n"
"varying vec2 vxformy;\n"
"varying vec4 vtint;\n"
"varying vec4 vprimary;\n"
"void main() {\n"
"  gl_Position=vec4((apos.x*2.0)/screensize.x-1.0,1.0-(apos.y*2.0)/screensize.y,0.0,1.0);\n"
"  gl_PointSize=asize;\n"
"  vtexcoord.x=floor(mod(atileid,16.0))/16.0;\n"
"  vtexcoord.y=floor(atileid/16.0)/16.0;\n"

"       if (axform<0.5) { vxformx=vec2( 1.0, 0.0); vxformy=vec2( 0.0, 1.0); }\n"
"  else if (axform<1.5) { vxformx=vec2(-1.0, 0.0); vxformy=vec2( 0.0, 1.0); }\n"
"  else if (axform<2.5) { vxformx=vec2( 1.0, 0.0); vxformy=vec2( 0.0,-1.0); }\n"
"  else if (axform<3.5) { vxformx=vec2(-1.0, 0.0); vxformy=vec2( 0.0,-1.0); }\n"
"  else if (axform<4.5) { vxformx=vec2( 0.0, 1.0); vxformy=vec2( 1.0, 0.0); }\n"
"  else if (axform<5.5) { vxformx=vec2( 0.0,-1.0); vxformy=vec2( 1.0, 0.0); }\n"
"  else if (axform<6.5) { vxformx=vec2( 0.0, 1.0); vxformy=vec2(-1.0, 0.0); }\n"
"  else                 { vxformx=vec2( 0.0,-1.0); vxformy=vec2(-1.0, 0.0); }\n"

"  if (arotate>0.0) {\n"
"    float slopscale=1.4142135623730951;\n"
"    gl_PointSize*=slopscale;\n"
"    float t=arotate*-6.283185307179586;\n"
"    vec2 tmpx=vec2(cos(t)*vxformx.x+sin(t)*vxformx.y,-sin(t)*vxformx.x+cos(t)*vxformx.y);\n"
"    vec2 tmpy=vec2(cos(t)*vxformy.x+sin(t)*vxformy.y,-sin(t)*vxformy.x+cos(t)*vxformy.y);\n"
"    vxformx=tmpx*slopscale;\n"
"    vxformy=tmpy*slopscale;\n"
"  }\n"

"  vtint=atint;\n"
"  vprimary=aprimary;\n"
"}\n";

static const char fmn_gl2_fsrc_maxtile[]=
"uniform vec2 texsize;\n"
"uniform sampler2D sampler;\n"
"varying vec2 vtexcoord;\n"
"varying vec2 vxformx;\n"
"varying vec2 vxformy;\n"
"varying vec4 vtint;\n"
"varying vec4 vprimary;\n"
"void main() {\n"

"  vec2 texcoord=gl_PointCoord-0.5;\n"
"  texcoord=vec2(texcoord.x*vxformx.x+texcoord.y*vxformx.y,texcoord.x*vxformy.x+texcoord.y*vxformy.y);\n"
"  texcoord+=0.5;\n"
"  if ((texcoord.x<0.0)||(texcoord.x>=1.0)||(texcoord.y<0.0)||(texcoord.y>=1.0)) discard;\n"
"  texcoord/=16.0;\n"
"  texcoord+=vtexcoord;\n"

"  vec4 texcolor=texture2D(sampler,texcoord);\n"
"  if ((texcolor.r==texcolor.g)&&(texcolor.g==texcolor.b)) {\n"
"    if (texcolor.r>=0.5) {\n"
"      texcolor.rgb=mix(vprimary.rgb,vec3(1.0,1.0,1.0),texcolor.r*2.0-1.0);\n"
"    } else {\n"
"      texcolor.rgb=mix(vec3(0.0,0.0,0.0),vprimary.rgb,texcolor.r*2.0);\n"
"    }\n"
"  }\n"
"  gl_FragColor=vec4(mix(\n"
"    texcolor.rgb,\n"
"    vtint.rgb,\n"
"    vtint.a\n"
"  ),texcolor.a*vprimary.a);\n"

"}\n";

/* Init.
 */
 
int fmn_gl2_program_maxtile_init(struct fmn_gl2_program *program,struct bigpc_render_driver *driver) {
  int err=fmn_gl2_program_init(program,"maxtile",fmn_gl2_vsrc_maxtile,sizeof(fmn_gl2_vsrc_maxtile),fmn_gl2_fsrc_maxtile,sizeof(fmn_gl2_fsrc_maxtile));
  if (err<0) return err;
  return 0;
}

/* Draw.
 */
 
void fmn_gl2_draw_maxtile(const struct fmn_gl2_vertex_maxtile *vtxv,int vtxc) {
  if (vtxc<1) return;
  glEnable(GL_POINT_SPRITE);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glVertexAttribPointer(0,2,GL_SHORT,0,sizeof(struct fmn_gl2_vertex_maxtile),&vtxv->x);
  glVertexAttribPointer(1,1,GL_UNSIGNED_BYTE,0,sizeof(struct fmn_gl2_vertex_maxtile),&vtxv->tileid);
  glVertexAttribPointer(2,1,GL_UNSIGNED_BYTE,1,sizeof(struct fmn_gl2_vertex_maxtile),&vtxv->rotate);
  glVertexAttribPointer(3,1,GL_UNSIGNED_BYTE,0,sizeof(struct fmn_gl2_vertex_maxtile),&vtxv->size);
  glVertexAttribPointer(4,1,GL_UNSIGNED_BYTE,0,sizeof(struct fmn_gl2_vertex_maxtile),&vtxv->xform);
  glVertexAttribPointer(5,4,GL_UNSIGNED_BYTE,1,sizeof(struct fmn_gl2_vertex_maxtile),&vtxv->tr);
  glVertexAttribPointer(6,4,GL_UNSIGNED_BYTE,1,sizeof(struct fmn_gl2_vertex_maxtile),&vtxv->pr);
  glDrawArrays(GL_POINTS,0,vtxc);
}

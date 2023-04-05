#include "fmn_gl2_internal.h"

/* Cleanup.
 */
 
void fmn_gl2_program_cleanup(struct fmn_gl2_program *program) {
  if (program->programid) glDeleteProgram(program->programid);
}

/* Compile one shader and attach it.
 */
 
static int fmn_gl2_program_compile(
  struct fmn_gl2_program *program,
  const char *name,
  const char *src,int srcc,
  int type
) {

  char version[256];
  int versionc;
  versionc=snprintf(version,sizeof(version),"#version %d\n",FMN_GL2_GLSL_VERSION);
  if ((versionc<1)||(versionc>=sizeof(version))) return -1;
  const char *srcv[2]={version,src};
  GLint lenv[2]={versionc,srcc};

  GLuint id=glCreateShader(type);
  if (!id) return -1;
  glShaderSource(id,2,srcv,lenv);
  glCompileShader(id);

  GLint status=0;
  glGetShaderiv(id,GL_COMPILE_STATUS,&status);
  if (status) {
    glAttachShader(program->programid,id);
    glDeleteShader(id);
    return 0;
  }

  int err=-1;
  GLint loga=0;
  glGetShaderiv(id,GL_INFO_LOG_LENGTH,&loga);
  if ((loga>0)&&(loga<INT_MAX)) {
    char *log=malloc(loga);
    if (log) {
      GLint logc=0;
      glGetShaderInfoLog(id,loga,&logc,log);
      while (logc&&((unsigned char)log[logc-1]<=0x20)) logc--;
      if ((logc>0)&&(logc<=loga)) {
        fprintf(stderr,
          "Error compiling '%s' %s shader:\n%.*s\n",
          name,(type==GL_VERTEX_SHADER)?"vertex":"fragment",logc,log
        );
        err=-2;
      }
      free(log);
    }
  }
  glDeleteShader(id);
  return err;
}

/* Link program, after both shaders are attached.
 */
 
static int fmn_gl2_program_link(
  struct fmn_gl2_program *program,
  const char *name
) {

  glLinkProgram(program->programid);
  GLint status=0;
  glGetProgramiv(program->programid,GL_LINK_STATUS,&status);
  if (status) return 0;

  /* Link failed. */
  int err=-1;
  GLint loga=0;
  glGetProgramiv(program->programid,GL_INFO_LOG_LENGTH,&loga);
  if ((loga>0)&&(loga<INT_MAX)) {
    char *log=malloc(loga);
    if (log) {
      GLint logc=0;
      glGetProgramInfoLog(program->programid,loga,&logc,log);
      while (logc&&((unsigned char)log[logc-1]<=0x20)) logc--;
      if ((logc>0)&&(logc<=loga)) {
        fprintf(stderr,
          "Error linking '%s' shader:\n%.*s\n",
          name,logc,log
        );
        err=-2;
      }
      free(log);
    }
  }
  return err;
}

/* Bind attributes from program text.
 */
 
static int fmn_gl2_program_bind_attributes(
  struct fmn_gl2_program *program,
  const char *pname,
  const char *src,int srcc
) {
  int srcp=0,attrid=0;
  while (srcp<srcc) {
    const char *line=src+srcp;
    int linec=0;
    while ((srcp<srcc)&&(src[srcp++]!=0x0a)) linec++;
    if ((linec>=10)&&!memcmp(line,"attribute ",10)) {
      #define IDCH(ch) (((ch>=0x30)&&(ch<=0x39))||((ch>=0x41)&&(ch<=0x5a))||((ch>=0x61)&&(ch<=0x7a)))
      while (!IDCH(line[linec-1])) linec--;
      const char *name=line+linec;
      int namec=0;
      while ((name>line)&&IDCH(name[-1])) { name--; namec++; }
      #undef IDCH
      if (namec) {
        char zname[32];
        if (namec>sizeof(zname)) return -1;
        memcpy(zname,name,namec);
        zname[namec]=0;
        glBindAttribLocation(program->programid,attrid,zname);
        attrid++;
      }
    }
  }
  program->attrc=attrid;
  return 0;
}

/* Initialize.
 */
 
int fmn_gl2_program_init(
  struct fmn_gl2_program *program,
  const char *name,
  const char *vsrc,int vsrcc,
  const char *fsrc,int fsrcc
) {
  int err;
  if (!(program->programid=glCreateProgram())) {
    if (!(program->programid=glCreateProgram())) {
      return -1;
    }
  }
  if (!vsrc) vsrcc=0; else if (vsrcc<0) { vsrcc=0; while (vsrc[vsrcc]) vsrcc++; }
  if (!fsrc) fsrcc=0; else if (fsrcc<0) { fsrcc=0; while (fsrc[fsrcc]) fsrcc++; }
  
  if ((err=fmn_gl2_program_compile(program,name,vsrc,vsrcc,GL_VERTEX_SHADER))<0) return err;
  if ((err=fmn_gl2_program_compile(program,name,fsrc,fsrcc,GL_FRAGMENT_SHADER))<0) return err;
  
  if ((err=fmn_gl2_program_bind_attributes(program,name,vsrc,vsrcc))<0) return err;
  
  if ((err=fmn_gl2_program_link(program,name))<0) return err;
  
  program->loc_screensize=glGetUniformLocation(program->programid,"screensize");
  program->loc_texsize=glGetUniformLocation(program->programid,"texsize");
  program->loc_sampler=glGetUniformLocation(program->programid,"sampler");
  
  return 0;
}

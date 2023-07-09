#include "datan_internal.h"

/* Primitives for the global registry.
 */
 
int datan_chalk_codepoint_for_bits(int bits) {
  const struct datan_chalk *chalk=datan.chalkv;
  int i=datan.chalkc;
  for (;i-->0;chalk++) {
    if (chalk->bits!=bits) continue;
    return chalk->codepoint;
  }
  return 0;
}

int datan_chalk_bits_for_codepoint(int codepoint,int p) {
  const struct datan_chalk *chalk=datan.chalkv;
  int i=datan.chalkc;
  for (;i-->0;chalk++) {
    if (chalk->codepoint!=codepoint) continue;
    if (p-->0) continue;
    return chalk->bits;
  }
  return 0;
}

int datan_chalk_add(int bits,int codepoint) {
  if (datan.chalkc>=datan.chalka) {
    int na=datan.chalka+128;
    if (na>INT_MAX/sizeof(struct datan_chalk)) return -1;
    void *nv=realloc(datan.chalkv,sizeof(struct datan_chalk)*na);
    if (!nv) return -1;
    datan.chalkv=nv;
    datan.chalka=na;
  }
  struct datan_chalk *chalk=datan.chalkv+datan.chalkc++;
  chalk->bits=bits;
  chalk->codepoint=codepoint;
  return 0;
}

/* Decode and store chalk file content.
 */
 
static int read_chalk(const char *src,int srcc,const char *path) {
  struct sr_decoder decoder={.v=src,.c=srcc};
  const char *line=0;
  int linec=0,lineno=1,err;
  for (;linec=sr_decode_line(&line,&decoder);lineno++) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    
    int linep=0,codepoint=0,bits=0;
    while ((linep<linec)&&((unsigned char)line[linep]>0x20)) {
      if ((line[linep]<'0')||(line[linep]>'9')) {
        fprintf(stderr,"%s:%d: Expected decimal integers\n",path,lineno);
        return -2;
      }
      codepoint*=10;
      codepoint+=line[linep++]-'0';
    }
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    while ((linep<linec)&&((unsigned char)line[linep]>0x20)) {
      if ((line[linep]<'0')||(line[linep]>'9')) {
        fprintf(stderr,"%s:%d: Expected decimal integers\n",path,lineno);
        return -2;
      }
      bits*=10;
      bits+=line[linep++]-'0';
    }
    
    if ((codepoint<=0x20)||(codepoint>0x7e)) {
      fprintf(stderr,"%s:%d: Codepoint must be in 0x21..0x7e. Found 0x%x (%d).\n",path,lineno,codepoint,codepoint);
      return -2;
    }
    if ((codepoint>='a')&&(codepoint<='z')) {
      fprintf(stderr,"%s:%d: Codepoint 0x%02x (%d) = '%c'. Please use upper case letters only.\n",path,lineno,codepoint,codepoint,codepoint);
      return -2;
    }
    if ((bits<1)||(bits>0xfffff)) {
      fprintf(stderr,"%s:%d: Bits must be in 1..0x000fffff ie 20 bits. Found 0x%08x (%d).\n",path,lineno,bits,bits);
      return -2;
    }
    // Duplicate (codepoint) is fine and normal, but duplicate (bits) is a hard error.
    if (err=datan_chalk_codepoint_for_bits(bits)) {
      fprintf(stderr,"%s:%d: Bits 0x%08x (%d) duplicated for codepoints 0x%02x and 0x%02x.\n",path,lineno,bits,bits,err,codepoint);
      return -2;
    }
    if (datan_chalk_add(bits,codepoint)<0) return -1;
  }
  return 0;
}

/* Validate directory src/chalk: It must contain 1 file named "1".
 */
 
static int validate_chalk_dir_cb(const char *path,const char *base,char type,void *userdata) {
  if (strcmp(base,"1")) {
    fprintf(stderr,"%s: Unexpected file in chalk dir.\n",path);
    return -2;
  }
  *(int*)userdata=1;
  return 0;
}
 
static int validate_chalk_dir(const char *path) {
  int ok=0,err;
  if (err=fmn_dir_read(path,validate_chalk_dir_cb,&ok)) {
    if (err!=-2) fprintf(stderr,"%s: Unexpected file in chalk dir.\n",path);
    return -2;
  }
  if (!ok) {
    fprintf(stderr,"%s: Chalk dir must contain a file \"1\".\n",path);
    return -2;
  }
  return 0;
}

/* Acquire and validate chalk letter definitions.
 */
 
int datan_acquire_chalk() {
  
  int err;
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"%s/chalk",datan.srcpath);
  if ((pathc<1)||(pathc>=sizeof(path))) return -1;
  if ((err=validate_chalk_dir(path))<0) return err;
  
  pathc=snprintf(path,sizeof(path),"%s/chalk/1",datan.srcpath);
  if ((pathc<1)||(pathc>=sizeof(path))) return -1;
  void *src=0;
  int srcc=fmn_file_read(&src,path);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read chalk definitions file.\n",path);
    return -2;
  }
  if ((err=read_chalk(src,srcc,path))<0) {
    free(src);
    return err;
  }
  free(src);
  
  return 0;
}

#include "mkd_internal.h"

/* Failure logging.
 */
 
static int faileval(const char *path,int lineno,const char *token,int tokenc,const char *expected) {
  fprintf(stderr,"%s:%d: Failed to evaluate '%.*s' as %s.\n",path,lineno,tokenc,token,expected);
  return -2;
}

/* image ID
 * 0x20 Image ID.
 */
 
static int mkd_sprite_cmd_image(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if (sr_int_eval(&v,src,srcc)>=2) ;
  else if ((v=assist_get_resource_id_by_name("image",src,srcc))>0) ;
  else return faileval(respath->path,lineno,src,srcc,"image id or name");
  if ((v<0)||(v>0xff)) return faileval(respath->path,lineno,src,srcc,"integer in 0..255");
  if (sr_encode_u8(&mkd.dst,0x20)<0) return -1;
  if (sr_encode_u8(&mkd.dst,v)<0) return -1;
  return 0;
}

/* tile ID
 * 0x21 Tile ID.
 */
 
static int mkd_sprite_cmd_tile(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if (sr_int_eval(&v,src,srcc)>=2) ;
  else return faileval(respath->path,lineno,src,srcc,"tile id");
  if ((v<0)||(v>0xff)) return faileval(respath->path,lineno,src,srcc,"integer in 0..255");
  if (sr_encode_u8(&mkd.dst,0x21)<0) return -1;
  if (sr_encode_u8(&mkd.dst,v)<0) return -1;
  return 0;
}

/* xform [xrev] [yrev] [swap]
 * 0x22 Xform.
 */
 
static int mkd_sprite_cmd_xform(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int srcp=0,v=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  while (srcp<srcc) {
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    int bit=0;
    if (sr_int_eval(&bit,token,tokenc)>=2) ;
    else if ((bit=assist_get_xform_by_name(token,tokenc))>0) ;
    else return faileval(respath->path,lineno,src,srcc,"'xrev', 'yrev', or 'swap'");
    v|=bit;
  }
  if (sr_encode_u8(&mkd.dst,0x22)<0) return -1;
  if (sr_encode_u8(&mkd.dst,v)<0) return -1;
  return 0;
}

/* style hidden|tile|hero|fourframe|firenozzle|firewall|doublewide|pitchfork|twoframe|eightframe
 * 0x23 Style. Default 1 (FMN_SPRITE_STYLE_TILE)
 */
 
static int mkd_sprite_cmd_style(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if (sr_int_eval(&v,src,srcc)>=2) ;
  else if ((v=assist_get_sprite_style_by_name(src,srcc))>=0) ;
  else return faileval(respath->path,lineno,src,srcc,"integer or sprite style name");
  if (sr_encode_u8(&mkd.dst,0x23)<0) return -1;
  if (sr_encode_u8(&mkd.dst,v)<0) return -1;
  return 0;
}

/* physics [motion] [edge] [sprites] [solid] [hole]
 * 0x24 Physics. Bitfields, see below.
0x01 MOTION
0x02 EDGE
0x04 SPRITES
0x10 SOLID
0x20 HOLE
 */
 
static int mkd_sprite_cmd_physics(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int srcp=0,v=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  while (srcp<srcc) {
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    int bit=0;
    if (sr_int_eval(&bit,token,tokenc)>=2) ;
    else if ((bit=assist_get_sprite_physics_by_name(token,tokenc))>0) ;
    else return faileval(respath->path,lineno,src,srcc,"sprite physics token");
    v|=bit;
  }
  if (sr_encode_u8(&mkd.dst,0x24)<0) return -1;
  if (sr_encode_u8(&mkd.dst,v)<0) return -1;
  return 0;
}

/* decay (FLOAT 0..255)
 * 0x40 Velocity decay. u8.8 linear decay in m/s**2
 * NB on review 2023-04-22, this command is not used at all. I haven't been playing auto-motion for sprites much.
 */
 
static int mkd_sprite_cmd_decay(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  double vf;
  if ((sr_double_eval(&vf,src,srcc)<0)||(vf<0.0f)||(vf>=256.0f)) return faileval(respath->path,lineno,src,srcc,"float in 0..255");
  uint16_t vi=(uint16_t)(vf*256.0f);
  if (sr_encode_u8(&mkd.dst,0x40)<0) return -1;
  if (sr_encode_intbe(&mkd.dst,vi,2)<0) return -1;
  return 0;
}

/* radius (FLOAT 0..255)
 * 0x41 Radius. u8.8 m
 */
 
static int mkd_sprite_cmd_radius(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  double vf;
  if ((sr_double_eval(&vf,src,srcc)<0)||(vf<0.0f)||(vf>=256.0f)) return faileval(respath->path,lineno,src,srcc,"float in 0..255");
  uint16_t vi=(uint16_t)(vf*256.0f);
  if (sr_encode_u8(&mkd.dst,0x41)<0) return -1;
  if (sr_encode_intbe(&mkd.dst,vi,2)<0) return -1;
  return 0;
}

/* invmass (0..255)
 * 0x25 Inverse mass. 0=infinite, 1=heaviest, 255=lightest
 */
 
static int mkd_sprite_cmd_invmass(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if ((sr_int_eval(&v,src,srcc)<2)||(v<0)||(v>255)) return faileval(respath->path,lineno,src,srcc,"int in 0..255");
  if (sr_encode_u8(&mkd.dst,0x25)<0) return -1;
  if (sr_encode_u8(&mkd.dst,v)<0) return -1;
  return 0;
}

/* controller (0..65535|name)
 * 0x42 Controller. See FMN_SPRCTL_* in src/app/sprite/fmn_sprite.h
 */
 
static int mkd_sprite_cmd_controller(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if (sr_int_eval(&v,src,srcc)>=2) ;
  else if ((v=assist_get_sprite_id_by_name(src,srcc))>=0) ;
  else return faileval(respath->path,lineno,src,srcc,"integer or sprite controller name");
  if (sr_encode_u8(&mkd.dst,0x42)<0) return -1;
  if (sr_encode_intbe(&mkd.dst,v,2)<0) return -1;
  return 0;
}

/* layer (0..255)
 * 0x26 Layer.
 */
 
static int mkd_sprite_cmd_layer(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if ((sr_int_eval(&v,src,srcc)<2)||(v<0)||(v>255)) return faileval(respath->path,lineno,src,srcc,"int in 0..255");
  if (sr_encode_u8(&mkd.dst,0x26)<0) return -1;
  if (sr_encode_u8(&mkd.dst,v)<0) return -1;
  return 0;
}

/* bv[N] (0..255) # N in 0..7
 * 0x43 bv, controller-specific parameters. (u8 k,u8 v)
 */
 
static int mkd_sprite_cmd_bv(struct mkd_respath *respath,int bp,const char *src,int srcc,int lineno) {
  if ((bp<0)||(bp>7)) {
    fprintf(stderr,"%s:%d: bv index must be in 0..7, found %d\n",respath->path,lineno,bp);
    return -2;
  }
  int v;
  
  // I guess we should generalize and share this? Copied from mkd_compile_map.c.
  // For now, I think not many other sprite fields will need namespaces so whatever.
  if ((sr_int_eval(&v,src,srcc)>=2)&&(v>=0)&&(v<=255)) {
  } else if ((srcc>=3)&&!memcmp(src,"gs:",3)) {
    if ((v=assist_get_gsbit_by_name(src+3,srcc-3))<0) return faileval(respath->path,lineno,src,srcc,"gsbit name");
  } else if ((srcc>=5)&&!memcmp(src,"item:",5)) {
    if ((v=assist_get_item_by_name(src+5,srcc-5))<0) return faileval(respath->path,lineno,src,srcc,"item name");
  } else if ((srcc>=3)&&!memcmp(src,"ev:",3)) {
    if ((v=assist_get_map_event_by_name(src+3,srcc-3))<0) return faileval(respath->path,lineno,src,srcc,"map event name");
  } else if ((srcc>=3)&&!memcmp(src,"cb:",3)) {
    if ((v=assist_get_map_callback_by_name(src+3,srcc-3))<0) return faileval(respath->path,lineno,src,srcc,"map callback name");
  } else if ((srcc>=6)&&!memcmp(src,"spell:",6)) {
    if ((v=assist_get_spell_id_by_name(src+6,srcc-6))<0) return faileval(respath->path,lineno,src,srcc,"spell name");
  } else {
    faileval(respath->path,lineno,src,srcc,"integer in 0..255");
  }
  
  if (sr_encode_u8(&mkd.dst,0x43)<0) return -1;
  if (sr_encode_u8(&mkd.dst,bp)<0) return -1;
  if (sr_encode_u8(&mkd.dst,v)<0) return -1;
  return 0;
}

/* Compile one sprite.
 */
 
int mkd_compile_sprite(struct mkd_respath *respath) {
  struct sr_decoder decoder={.v=mkd.src,.c=mkd.srcc};
  const char *line;
  int linec,lineno=1;
  for (;linec=sr_decode_line(&line,&decoder);lineno++) {
    int i=0; for (;i<linec;i++) if (line[i]=='#') linec=i;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    //fprintf(stderr,"%s:%d: %.*s\n",respath->path,lineno,linec,line);
    
    const char *kw=line;
    int kwc=0,linep=0;
    while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) kwc++;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    
    int err=-1;
    #define _(tag) if ((kwc==sizeof(#tag)-1)&&!memcmp(kw,#tag,kwc)) err=mkd_sprite_cmd_##tag(respath,line+linep,linec-linep,lineno); else
    _(image)
    _(tile)
    _(xform)
    _(style)
    _(physics)
    _(decay)
    _(radius)
    _(invmass)
    _(controller)
    _(layer)
    #undef _
    if ((kwc==5)&&(kw[0]=='b')&&(kw[1]=='v')&&(kw[2]=='[')&&(kw[3]>='0')&&(kw[3]<='7')&&(kw[4]==']')) err=mkd_sprite_cmd_bv(respath,kw[3]-'0',line+linep,linec-linep,lineno);
    else if ((kwc==7)&&!memcmp(kw,"argtype",7)) err=0; // we can ignore this one; it's only for editors
    else {
      fprintf(stderr,"%s:%d: Unknown command '%.*s'\n",respath->path,lineno,kwc,kw);
      return -2;
    }
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error processing '%.*s' sprite command.\n",respath->path,lineno,kwc,kw);
      return -2;
    }
  }
  return 0;
}

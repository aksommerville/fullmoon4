#include "mkd_internal.h"
#include <stdarg.h>

// Don't include fmn_platform.h in the build tools.
#define FMN_COLC 20
#define FMN_ROWC 12

/* Failure logging.
 */
 
static int faileval(const char *path,int lineno,const char *token,int tokenc,const char *expected) {
  fprintf(stderr,"%s:%d: Failed to evaluate '%.*s' as %s.\n",path,lineno,tokenc,token,expected);
  return -2;
}

/* Uniform parameter evaluation.
 * We log errors too.
 * Variadic args, per 'dsta':
 *  - const char *fldname
 *  - int lo
 *  - int hi
 *  - const char *restype // empty string if not applicable (don't use zero; that's variadic "int", not "pointer")
 * If the token begins with one of our namespace identifiers eg "gs:", we do the right thing.
 */
 
static int evalparams(
  const char *path,int lineno,
  const char *src,int srcc,
  int *dst,int dsta,...
) {
  va_list vargs;
  va_start(vargs,dsta);
  int srcp=0;
  int dstc=0; for (;dstc<dsta;dstc++) {
  
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  
    const char *fldname=va_arg(vargs,const char*);
    if (!fldname||!fldname[0]) return -1;
    int lo=va_arg(vargs,int);
    int hi=va_arg(vargs,int);
    const char *restype=va_arg(vargs,const char*);
    
    if (!tokenc) {
      fprintf(stderr,"%s:%d: Unexpected end of line, expecting field '%s'\n",path,lineno,fldname);
      return -2;
    }
    
    // Plain integers are always ok.
    if (sr_int_eval(dst+dstc,token,tokenc)>=2) {
    
    // Check known namespace prefixes.
    } else if ((tokenc>=3)&&!memcmp(token,"gs:",3)) {
      if ((dst[dstc]=assist_get_gsbit_by_name(token+3,tokenc-3))<0) return faileval(path,lineno,token,tokenc,"gsbit name");
    } else if ((tokenc>=5)&&!memcmp(token,"item:",5)) {
      if ((dst[dstc]=assist_get_item_by_name(token+5,tokenc-5))<0) return faileval(path,lineno,token,tokenc,"item name");
    } else if ((tokenc>=3)&&!memcmp(token,"ev:",3)) {
      if ((dst[dstc]=assist_get_map_event_by_name(token+3,tokenc-3))<0) return faileval(path,lineno,token,tokenc,"map event name");
    } else if ((tokenc>=3)&&!memcmp(token,"cb:",3)) {
      if ((dst[dstc]=assist_get_map_callback_by_name(token+3,tokenc-3))<0) return faileval(path,lineno,token,tokenc,"map callback name");
      
    // If a resource type was suggested, try it.
    } else if (restype&&restype[0]&&((dst[dstc]=assist_get_resource_id_by_name(restype,token,tokenc))>0)) {
    
    // OK I give up.
    } else {
      fprintf(stderr,"%s:%d: Failed to parse '%.*s' for field '%s'.\n",path,lineno,tokenc,token,fldname);
      return -2;
    }
    
    // Now we have the value, assert the range.
    if ((dst[dstc]<lo)||(dst[dstc]>hi)) {
      fprintf(stderr,"%s:%d: Value %d out of range %d..%d for '%s'\n",path,lineno,dst[dstc],lo,hi,fldname);
      return -2;
    }
  }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp<srcc) {
    fprintf(stderr,"%s:%d: Unexpected extra tokens: '%.*s'\n",path,lineno,srcc-srcp,src+srcp);
    return -2;
  }
  return 0;
}

/* song SONGID
 * 0x20 (u8 songid) SONG
 */
 
static int mkd_map_cmd_song(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if (sr_int_eval(&v,src,srcc)>=2) ;
  else if ((v=assist_get_resource_id_by_name("song",src,srcc))>0) ;
  else return faileval(respath->path,lineno,src,srcc,"integer or song name");
  if ((v<0)||(v>0xff)) return faileval(respath->path,lineno,src,srcc,"integer in 0..255; maps can only use the first 255 songs");
  if (sr_encode_u8(&mkd.dst,0x20)<0) return -1;
  if (sr_encode_u8(&mkd.dst,v)<0) return -1;
  return 0;
}

/* tilesheet IMAGEID
 * 0x21 (u8 imageid) TILESHEET
 */
 
static int mkd_map_cmd_tilesheet(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if (sr_int_eval(&v,src,srcc)>=2) ;
  else if ((v=assist_get_resource_id_by_name("image",src,srcc))>0) ;
  else return faileval(respath->path,lineno,src,srcc,"integer or image name");
  if ((v<1)||(v>0xff)) return faileval(respath->path,lineno,src,srcc,"integer in 1..255; map tilesheets can only use the first 255 images");
  if (sr_encode_u8(&mkd.dst,0x21)<0) return -1;
  if (sr_encode_u8(&mkd.dst,v)<0) return -1;
  return 0;
}

/* neighborw MAPID
 * 0x40 (u16 mapid) NEIGHBORW
 * neighbore MAPID
 * 0x41 (u16 mapid) NEIGHBORE
 * neighborn MAPID
 * 0x42 (u16 mapid) NEIGHBORN
 * neighbors MAPID
 * 0x43 (u16 mapid) NEIGHBORS
 */
 
static int mkd_map_cmd_neighborw(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if (sr_int_eval(&v,src,srcc)>=2) ;
  else if ((v=assist_get_resource_id_by_name("map",src,srcc))>0) ;
  else return faileval(respath->path,lineno,src,srcc,"integer or map name");
  if ((v<0)||(v>0xffff)) return faileval(respath->path,lineno,src,srcc,"map id");
  if (sr_encode_u8(&mkd.dst,0x40)<0) return -1;
  if (sr_encode_intbe(&mkd.dst,v,2)<0) return -1;
  return 0;
}

static int mkd_map_cmd_neighbore(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if (sr_int_eval(&v,src,srcc)>=2) ;
  else if ((v=assist_get_resource_id_by_name("map",src,srcc))>0) ;
  else return faileval(respath->path,lineno,src,srcc,"integer or map name");
  if ((v<0)||(v>0xffff)) return faileval(respath->path,lineno,src,srcc,"map id");
  if (sr_encode_u8(&mkd.dst,0x41)<0) return -1;
  if (sr_encode_intbe(&mkd.dst,v,2)<0) return -1;
  return 0;
}

static int mkd_map_cmd_neighborn(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if (sr_int_eval(&v,src,srcc)>=2) ;
  else if ((v=assist_get_resource_id_by_name("map",src,srcc))>0) ;
  else return faileval(respath->path,lineno,src,srcc,"integer or map name");
  if ((v<0)||(v>0xffff)) return faileval(respath->path,lineno,src,srcc,"map id");
  if (sr_encode_u8(&mkd.dst,0x42)<0) return -1;
  if (sr_encode_intbe(&mkd.dst,v,2)<0) return -1;
  return 0;
}

static int mkd_map_cmd_neighbors(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int v;
  if (sr_int_eval(&v,src,srcc)>=2) ;
  else if ((v=assist_get_resource_id_by_name("map",src,srcc))>0) ;
  else return faileval(respath->path,lineno,src,srcc,"integer or map name");
  if ((v<0)||(v>0xffff)) return faileval(respath->path,lineno,src,srcc,"map id");
  if (sr_encode_u8(&mkd.dst,0x43)<0) return -1;
  if (sr_encode_intbe(&mkd.dst,v,2)<0) return -1;
  return 0;
}

/* door X Y MAPID DSTX DSTY
 * 0x60 (u8 cellp,u16 mapid,u8 dstcellp) DOOR
 */
 
static int mkd_map_cmd_door(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int argv[5];
  int err=evalparams(
    respath->path,lineno,src,srcc,argv,5,
    "x",0,FMN_COLC-1,"",
    "y",0,FMN_ROWC-1,"",
    "mapid",1,0xffff,"map",
    "dstx",0,FMN_COLC-1,"",
    "dsty",0,FMN_ROWC-1,""
  );
  if (err<0) return err;
  if (sr_encode_u8(&mkd.dst,0x60)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[1]*FMN_COLC+argv[0])<0) return -1;
  if (sr_encode_intbe(&mkd.dst,argv[2],2)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[4]*FMN_COLC+argv[3])<0) return -1;
  return 0;
}

/* sprite X Y SPRITEID ARG0 ARG1 ARG2
 * 0x80 (u8 cellp,u16 spriteid,u8 arg0,u8 arg1,u8 arg2) SPRITE
 */
 
static int mkd_map_cmd_sprite(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int argv[6];
  int err=evalparams(
    respath->path,lineno,src,srcc,argv,6,
    "x",0,FMN_COLC-1,"",
    "y",0,FMN_ROWC-1,"",
    "spriteid",1,0xffff,"sprite",
    "arg0",0,0xff,"",
    "arg1",0,0xff,"",
    "arg2",0,0xff,""
  );
  if (err<0) return err;
  if (sr_encode_u8(&mkd.dst,0x80)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[1]*FMN_COLC+argv[0])<0) return -1;
  if (sr_encode_intbe(&mkd.dst,argv[2],2)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[3])<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[4])<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[5])<0) return -1;
  return 0;
}

/* dark
 * 0x01 () DARK
 */
 
static int mkd_map_cmd_dark(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  if (sr_encode_u8(&mkd.dst,0x01)<0) return -1;
  return 0;
}

/* hero X Y
 * 0x22 (u8 cellp) HERO
 */
 
static int mkd_map_cmd_hero(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int argv[2];
  int err=evalparams(
    respath->path,lineno,src,srcc,argv,2,
    "x",0,FMN_COLC-1,"",
    "y",0,FMN_ROWC-1,""
  );
  if (err<0) return err;
  if (sr_encode_u8(&mkd.dst,0x22)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[1]*FMN_COLC+argv[0])<0) return -1;
  return 0;
}

/* transmogrify X Y MODE STATE # STATE=[1:pumpkin], MODE=[to,from,toggle]
 * 0x44 (u8 cellp,u8 0x80:to 0x40:from 0x3f:state) TRANSMOGRIFY
 */
 
static int mkd_map_cmd_transmogrify(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  // We have some unique parameters, not going to try shoe-horning those into the general evaluation.
  int x,y,mode,state;
  const char *token;
  int tokenc;
  int srcp=0;
  
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  if ((sr_int_eval(&x,token,tokenc)<2)||(x<0)||(x>=FMN_COLC)) return faileval(respath->path,lineno,token,tokenc,"horz in 0..19");
  
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  if ((sr_int_eval(&y,token,tokenc)<2)||(y<0)||(y>=FMN_ROWC)) return faileval(respath->path,lineno,token,tokenc,"vert in 0..11");
  
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  if ((tokenc==2)&&!memcmp(token,"to",2)) mode=0x80;
  else if ((tokenc==4)&&!memcmp(token,"from",4)) mode=0x40;
  else if ((tokenc==6)&&!memcmp(token,"toggle",6)) mode=0xc0;
  else return faileval(respath->path,lineno,token,tokenc,"'to', 'from', or 'toggle'");
  
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  if ((sr_int_eval(&state,token,tokenc)>=2)&&(state>=0)&&(state<=0x3f)) ;
  else if ((tokenc==7)&&!memcmp(token,"pumpkin",7)) state=1;
  else return faileval(respath->path,lineno,token,tokenc,"transmogrify state in 0..63");
  
  if (sr_encode_u8(&mkd.dst,0x44)<0) return -1;
  if (sr_encode_u8(&mkd.dst,y*FMN_COLC+x)<0) return -1;
  if (sr_encode_u8(&mkd.dst,mode|state)<0) return -1;
  return 0;
}

/* indoors
 * 0x02 () INDOORS
 */
 
static int mkd_map_cmd_indoors(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  if (sr_encode_u8(&mkd.dst,0x02)<0) return -1;
  return 0;
}

/* wind N|E|S|W # direction it blows *to*, opposite the meterological convention
 * 0x23 (u8 dir) WIND
 */
 
static int mkd_map_cmd_wind(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  uint8_t dir;
  switch ((srcc==1)?src[0]:'?') {
    case 'N': dir=0x40; break;
    case 'S': dir=0x02; break;
    case 'W': dir=0x10; break;
    case 'E': dir=0x08; break;
    default: return faileval(respath->path,lineno,src,srcc,"'N', 'E', 'S', or 'W'");
  }
  if (sr_encode_u8(&mkd.dst,0x23)<0) return -1;
  if (sr_encode_u8(&mkd.dst,dir)<0) return -1;
  return 0;
}

/* sketch X Y BITS # Creates sketch if there are none initially.
 * 0x61 (u8 cellp,u24 bits) SKETCH
 */
 
static int mkd_map_cmd_sketch(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int argv[3];
  int err=evalparams(
    respath->path,lineno,src,srcc,argv,3,
    "x",0,FMN_COLC-1,"",
    "y",0,FMN_ROWC-1,"",
    "bits",0,0xffffff,""
  );
  if (err<0) return err;
  if (sr_encode_u8(&mkd.dst,0x61)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[1]*FMN_COLC+argv[0])<0) return -1;
  if (sr_encode_intbe(&mkd.dst,argv[2],3)<0) return -1;
  return 0;
}

/* blowback
 * 0x03 () BLOWBACK
 */
 
static int mkd_map_cmd_blowback(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  if (sr_encode_u8(&mkd.dst,0x03)<0) return -1;
  return 0;
}

/* buried_treasure X Y GSBIT ITEMID
 * 0x62 (u8 cellp,u16 gsbit,u8 itemid) BURIED_TREASURE
 */
 
static int mkd_map_cmd_buried_treasure(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int argv[4];
  int err=evalparams(
    respath->path,lineno,src,srcc,argv,4,
    "x",0,FMN_COLC-1,"",
    "y",0,FMN_ROWC-1,"",
    "gsbit",0,0xffff,"",
    "itemid",0,0xff,""
  );
  if (err<0) return err;
  if (sr_encode_u8(&mkd.dst,0x62)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[1]*FMN_COLC+argv[0])<0) return -1;
  if (sr_encode_intbe(&mkd.dst,argv[2],2)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[3])<0) return -1;
  return 0;
}

/* buried_door X Y GSBIT MAPID DSTX DSTY
 * 0x81 (u8 cellp,u16 gsbit,u16 mapid,u8 dstp) BURIED_DOOR
 */
 
static int mkd_map_cmd_buried_door(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int argv[6];
  int err=evalparams(
    respath->path,lineno,src,srcc,argv,6,
    "x",0,FMN_COLC-1,"",
    "y",0,FMN_ROWC-1,"",
    "gsbit",0,0xffff,"",
    "mapid",0,0xffff,"map",
    "dstx",0,FMN_COLC-1,"",
    "dsty",0,FMN_ROWC-1,""
  );
  if (err<0) return err;
  if (sr_encode_u8(&mkd.dst,0x81)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[1]*FMN_COLC+argv[0])<0) return -1;
  if (sr_encode_intbe(&mkd.dst,argv[2],2)<0) return -1;
  if (sr_encode_intbe(&mkd.dst,argv[3],2)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[5]*FMN_COLC+argv[4])<0) return -1;
  return 0;
}

/* callback EVID CBID PARAM
 * 0x63 (u8 evid,u16 cbid,u8 param) CALLBACK
 */
 
static int mkd_map_cmd_callback(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int argv[3];
  int err=evalparams(
    respath->path,lineno,src,srcc,argv,3,
    "evid",0,0xff,"",
    "cbid",0,0xffff,"",
    "param",0,0xff,""
  );
  if (err<0) return err;
  if (sr_encode_u8(&mkd.dst,0x63)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[0])<0) return -1;
  if (sr_encode_intbe(&mkd.dst,argv[1],2)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[2])<0) return -1;
  return 0;
}

/* ancillary # Will not be targetted by crow guidance, and not participate in travel coverage.
 * 0x04 () ANCILLARY
 */
 
static int mkd_map_cmd_ancillary(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  if (sr_encode_u8(&mkd.dst,0x04)<0) return -1;
  return 0;
}

/* event_trigger X Y EVENTID # for fmn_game.h:fmn_game_event_listen. Not the same events as "callback".
 * 0x64 (u8 cellp,u16 eventid,u8 unused) EVENT_TRIGGER
 */
 
static int mkd_map_cmd_event_trigger(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int argv[3];
  int err=evalparams(
    respath->path,lineno,src,srcc,argv,3,
    "x",0,FMN_COLC-1,"",
    "y",0,FMN_ROWC-1,"",
    "eventid",0,0xffff,""
  );
  if (err<0) return err;
  if (sr_encode_u8(&mkd.dst,0x64)<0) return -1;
  if (sr_encode_u8(&mkd.dst,argv[1]*FMN_COLC+argv[0])<0) return -1;
  if (sr_encode_intbe(&mkd.dst,argv[2],2)<0) return -1;
  if (sr_encode_u8(&mkd.dst,0)<0) return -1;
  return 0;
}

/* facedir GSBIT_COUNTERCLOCKWISE GSBIT_CLOCKWISE
 * 0x65 (u16 gsbit_counterclockwise,u16 gsbit_clockwise) FACEDIR
 */
 
static int mkd_map_cmd_facedir(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  int argv[2];
  int err=evalparams(
    respath->path,lineno,src,srcc,argv,2,
    "gsbit_counterclockwise",0,65535,"",
    "gsbit_clockwise",0,65535,""
  );
  if (err<0) return err;
  if (sr_encode_u8(&mkd.dst,0x65)<0) return -1;
  if (sr_encode_intbe(&mkd.dst,argv[0],2)<0) return -1;
  if (sr_encode_intbe(&mkd.dst,argv[1],2)<0) return -1;
  return 0;
}

/* Compile one row of the picture.
 */
 
static int mkd_compile_map_row(struct mkd_respath *respath,const char *src,int srcc,int lineno) {
  if (srcc!=FMN_COLC<<1) {
    fprintf(stderr,"%s:%d: Map rows must be exactly %d characters (%d cells). Found %d characters.\n",respath->path,lineno,FMN_COLC<<1,FMN_COLC,srcc);
    return -2;
  }
  int dstp=mkd.dst.c;
  if (sr_encoder_require(&mkd.dst,FMN_COLC)<0) return -1;
  mkd.dst.c+=FMN_COLC;
  uint8_t *dst=((uint8_t*)(mkd.dst.v))+dstp;
  int i=FMN_COLC;
  for (;i-->0;dst++,src+=2) {
    int hi=sr_digit_eval(src[0]);
    int lo=sr_digit_eval(src[1]);
    if ((hi<0)||(hi>0xf)||(lo<0)||(lo>0xf)) {
      fprintf(stderr,"%s:%d: Illegal cell '%.2s' in map picture, must be a hexadecimal byte.\n",respath->path,lineno,src);
      return -2;
    }
    *dst=(hi<<4)|lo;
  }
  return 0;
}

/* Compile one map.
 */
 
int mkd_compile_map(struct mkd_respath *respath) {
  struct sr_decoder decoder={.v=mkd.src,.c=mkd.srcc};
  const char *line;
  int linec,lineno=1;
  int y=0;
  for (;linec=sr_decode_line(&line,&decoder);lineno++) {
    int i=0; for (;i<linec;i++) if (line[i]=='#') linec=i;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    
    // First (ROWC) lines are the picture.
    if (y<FMN_ROWC) {
      int err=mkd_compile_map_row(respath,line,linec,lineno);
      if (err<0) {
        if (err!=-2) fprintf(stderr,"%s:%d: Error processing row %d of map.\n",respath->path,lineno,y);
        return -2;
      }
      y++;
      continue;
    }
    
    // Beyond the picture are text commands.
    const char *kw=line;
    int kwc=0,linep=0;
    while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) kwc++;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    
    int err=-1;
    #define _(tag) if ((kwc==sizeof(#tag)-1)&&!memcmp(kw,#tag,kwc)) err=mkd_map_cmd_##tag(respath,line+linep,linec-linep,lineno); else
    _(song)
    _(tilesheet)
    _(neighborw)
    _(neighbore)
    _(neighborn)
    _(neighbors)
    _(door)
    _(sprite)
    _(dark)
    _(hero)
    _(transmogrify)
    _(indoors)
    _(wind)
    _(sketch)
    _(blowback)
    _(buried_treasure)
    _(buried_door)
    _(callback)
    _(ancillary)
    _(event_trigger)
    _(facedir)
    #undef _
    {
      fprintf(stderr,"%s:%d: Unknown map command '%.*s'\n",respath->path,lineno,kwc,kw);
      return -2;
    }
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error processing '%.*s' map command.\n",respath->path,lineno,kwc,kw);
      return -2;
    }
  }
  if (y<FMN_ROWC) {
    fprintf(stderr,"%s: Incomplete map picture. %d rows, expected %d\n",respath->path,y,FMN_ROWC);
    return -2;
  }
  return 0;
}

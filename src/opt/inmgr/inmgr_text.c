#include "inmgr_internal.h"
#include <stdarg.h>

/* Match pattern.
 */
 
static int inmgr_pattern_match_1(const char *pat,int patc,const char *src,int srcc) {
  int patp=0,srcp=0;
  while (1) {
  
    // Terminate if pattern depleted. Match if src also depleted.
    if (patp>=patc) {
      if (srcp<srcc) return 0;
      return 1;
    }
    
    // Wildcard. (can enter here if src depleted).
    if (pat[patp]=='*') {
      while ((patp<patc)&&(pat[patp]=='*')) patp++; // Adjacent wildcards are redundant.
      if (patp>=patc) return 1; // Terminal wildcard => match, regardless of what's left in src.
      while (srcp<srcc) {
        if (inmgr_pattern_match_1(pat+patp,patc-patp,src+srcp,srcc-srcp)) return 1;
        srcp++;
      }
      return 0;
    }
    
    // Mismatch if (src) depleted.
    if (srcp>=srcc) return 0;
    
    // Condense whitespace.
    if ((unsigned char)pat[patp]<=0x20) {
      if ((unsigned char)src[srcp]>0x20) return 0;
      while ((patp<patc)&&((unsigned char)pat[patp]<=0x20)) patp++;
      while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
      continue;
    }
    if ((unsigned char)src[srcp]<=0x20) return 0;
    
    // Letters case-insensitive, all else verbatim.
    char cha=pat[patp++];
    char chb=src[srcp++];
    if ((cha>='a')&&(cha<='z')) cha-=0x20;
    if ((chb>='a')&&(chb<='z')) chb-=0x20;
    if (cha!=chb) return 0;
  }
}
 
int inmgr_pattern_match(const char *pat,int patc,const char *src,int srcc) {
  if (!pat) patc=0; else if (patc<0) { patc=0; while (pat[patc]) patc++; }
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  while (patc&&((unsigned char)pat[patc-1]<=0x20)) patc--;
  while (patc&&((unsigned char)pat[0]<=0x20)) { patc--; pat++; }
  while (srcc&&((unsigned char)src[srcc-1]<=0x20)) srcc--;
  while (srcc&&((unsigned char)src[0]<=0x20)) { srcc--; src++; }
  return inmgr_pattern_match_1(pat,patc,src,srcc);
}

/* Context for parsing config file.
 */
 
struct inmgr_config_read_context {
  struct inmgr *inmgr;
  struct inmgr_rules *rules;
  int lineno;
  void (*cb_error)(const char *msg,int msgc,int lineno,void *userdata);
  void *userdata;
};

static int inmgr_config_read_error(struct inmgr_config_read_context *ctx,const char *fmt,...) {
  if (!ctx->cb_error) return -1;
  va_list vargs;
  va_start(vargs,fmt);
  char tmp[256];
  int tmpc=vsnprintf(tmp,sizeof(tmp),fmt,vargs);
  if ((tmpc>=0)&&(tmpc<sizeof(tmp))) {
    ctx->cb_error(tmp,tmpc,ctx->lineno,ctx->userdata);
  } else {
    int fmtc=0; while (fmt[fmtc]) fmtc++; // better than nothing?
    ctx->cb_error(fmt,fmtc,ctx->lineno,ctx->userdata);
  }
  return -1;
}

static int inmgr_hexdigit_eval(char src) {
  if ((src>='0')&&(src<='9')) return src-'0';
  if ((src>='a')&&(src<='f')) return src-'a'+10;
  if ((src>='A')&&(src<='F')) return src-'A'+10;
  return -1;
}

static int memcasecmp(const char *a,const char *b,int c) {
  for (;c-->0;a++,b++) {
    char cha=*a; if ((cha>='A')&&(cha<='Z')) cha+=0x20;
    char chb=*b; if ((chb>='A')&&(chb<='Z')) chb+=0x20;
    if (cha<chb) return -1;
    if (cha>chb) return 1;
  }
  return 0;
}

static const char *inmgr_repr_srcpart(uint8_t srcpart) {
  switch (srcpart) {
    case INMGR_SRCPART_BUTTON_ON: return "btn";
    case INMGR_SRCPART_AXIS_LOW: return "lo";
    case INMGR_SRCPART_AXIS_HIGH: return "hi";
    case INMGR_SRCPART_HAT_N: return "n";
    case INMGR_SRCPART_HAT_E: return "e";
    case INMGR_SRCPART_HAT_S: return "s";
    case INMGR_SRCPART_HAT_W: return "w";
  }
  return "btn";
}

static int inmgr_config_eval_srcpart(const char *src,int srcc) {
  if ((srcc==3)&&!memcasecmp(src,"btn",3)) return INMGR_SRCPART_BUTTON_ON;
  if ((srcc==2)&&!memcasecmp(src,"lo",2)) return INMGR_SRCPART_AXIS_LOW;
  if ((srcc==2)&&!memcasecmp(src,"hi",2)) return INMGR_SRCPART_AXIS_HIGH;
  if (srcc==1) switch (src[0]) {
    case 'n': case 'N': return INMGR_SRCPART_HAT_N;
    case 'e': case 'E': return INMGR_SRCPART_HAT_E;
    case 's': case 'S': return INMGR_SRCPART_HAT_S;
    case 'w': case 'W': return INMGR_SRCPART_HAT_W;
  }
  return 0;
}

static int inmgr_config_eval_dsttype(const char *src,int srcc) {
  if ((srcc==6)&&!memcasecmp(src,"button",6)) return INMGR_DSTTYPE_BUTTON;
  if ((srcc==6)&&!memcasecmp(src,"action",6)) return INMGR_DSTTYPE_ACTION;
  return 0;
}

/* Read config file: Block start.
 * Caller must trim the keyword "device" and space after.
 */
 
static int inmgr_config_read_block_start(struct inmgr_config_read_context *ctx,const char *src,int srcc) {
  
  // "VID PID NAME"
  int srcp=0,vid=0,pid=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp>=srcc) return inmgr_config_read_error(ctx,"Expected VID");
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) {
    int digit=inmgr_hexdigit_eval(src[srcp]);
    if (digit<0) return inmgr_config_read_error(ctx,"Expected hexadecimal digit, found '%c'",src[srcp]);
    vid<<=4;
    vid|=digit;
    srcp++;
    if (vid>0xffff) return inmgr_config_read_error(ctx,"Invalid VID.");
  }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp>=srcc) return inmgr_config_read_error(ctx,"Expected PID");
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) {
    int digit=inmgr_hexdigit_eval(src[srcp]);
    if (digit<0) return inmgr_config_read_error(ctx,"Expected hexadecimal digit, found '%c'",src[srcp]);
    pid<<=4;
    pid|=digit;
    srcp++;
    if (pid>0xffff) return inmgr_config_read_error(ctx,"Invalid PID.");
  }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *name=src+srcp;
  int namec=srcc-srcp; // ok if empty
  
  if (!(ctx->rules=inmgr_rulesv_new(ctx->inmgr,-1))) return inmgr_config_read_error(ctx,"Failed to create rules record.");
  if (inmgr_rules_set_name(ctx->rules,name,namec)<0) return inmgr_config_read_error(ctx,"Error setting rules name.");
  ctx->rules->vid=vid;
  ctx->rules->pid=pid;
  
  return 0;
}

/* Read config file: Rule.
 * Provide the entire input line, except edge space.
 */
 
static int inmgr_config_read_rule(struct inmgr_config_read_context *ctx,const char *src,int srcc) {
  
  if (!ctx->rules) return inmgr_config_read_error(ctx,"Expected introducer \"device VID PID NAME\" before rule.");
  
  // 2..4 tokens: SRCBTNID [SRCPART] [DSTTYPE] DSTBTNID
  struct token { const char *v; int c; } tokenv[4];
  int srcp=0,tokenc=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (tokenc>=4) return inmgr_config_read_error(ctx,"Unexpected tokens after rule: '%.*s'",srcc-srcp,src+srcp);
    tokenv[tokenc].v=src+srcp;
    tokenv[tokenc].c=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenv[tokenc].c++;
    tokenc++;
  }
  if (!tokenc) return 0; // we shouldn't be given blank lines but whatever.
  if (tokenc==1) return inmgr_config_read_error(ctx,"Expected \"SRCBTNID [SRCPART] [DSTTYPE] DSTBTNID\" or \"device VID PID NAME\"");
  
  // tokenv[0] must be SRCBTNID.
  int srcbtnid=0,i;
  for (i=0;i<tokenv[0].c;i++) {
    int digit=inmgr_hexdigit_eval(tokenv[0].v[i]);
    if (digit<0) return inmgr_config_read_error(ctx,"Expected hexadecimal digit, found '%c'",tokenv[0].v[i]);
    if (srcbtnid&0x78000000) return inmgr_config_read_error(ctx,"Invalid srcbtnid");
    srcbtnid<<=4;
    srcbtnid|=digit;
  }
  
  // 4 tokens, SRCPART and DSTTYPE are both supplied. 3 tokens, unclear which. 2 tokens, neither.
  uint8_t srcpart=INMGR_SRCPART_BUTTON_ON;
  uint8_t dsttype=0;
  if (tokenc==4) {
    if (!(srcpart=inmgr_config_eval_srcpart(tokenv[1].v,tokenv[1].c))) {
      return inmgr_config_read_error(ctx,"Expected one of (btn,lo,hi,n,e,s,w), found '%.*s'",tokenv[1].c,tokenv[1].v);
    }
    if (!(dsttype=inmgr_config_eval_dsttype(tokenv[2].v,tokenv[2].c))) {
      return inmgr_config_read_error(ctx,"Expected one of (button,action), found '%.*s'",tokenv[2].c,tokenv[2].v);
    }
  } else if (tokenc==3) {
    int v=inmgr_config_eval_srcpart(tokenv[1].v,tokenv[1].c);
    if (v) {
      srcpart=v;
    } else {
      v=inmgr_config_eval_dsttype(tokenv[1].v,tokenv[1].c);
      if (!v) return inmgr_config_read_error(ctx,"Expected one of (btn,lo,hi,n,e,s,w,button,action), found '%.*s'",tokenv[1].c,tokenv[1].v);
      dsttype=v;
    }
  }
  
  // Last token is DSTBTNID.
  const char *tok=tokenv[tokenc-1].v;
  int tokc=tokenv[tokenc-1].c;
  uint16_t dstbtnid;
  switch (dsttype) {
    case INMGR_DSTTYPE_BUTTON: {
        if (!ctx->inmgr->delegate.btnid_eval||!(dstbtnid=ctx->inmgr->delegate.btnid_eval(tok,tokc))) {
          return inmgr_config_read_error(ctx,"Failed to evaluate '%.*s' as button name.",tokc,tok);
        }
      } break;
    case INMGR_DSTTYPE_ACTION: {
        if (!ctx->inmgr->delegate.actionid_eval||!(dstbtnid=ctx->inmgr->delegate.actionid_eval(tok,tokc))) {
          return inmgr_config_read_error(ctx,"Failed to evaluate '%.*s' as action name.",tokc,tok);
        }
      } break;
    default: {
        if (ctx->inmgr->delegate.btnid_eval&&(dstbtnid=ctx->inmgr->delegate.btnid_eval(tok,tokc))) {
          dsttype=INMGR_DSTTYPE_BUTTON;
        } else if (ctx->inmgr->delegate.actionid_eval&&(dstbtnid=ctx->inmgr->delegate.actionid_eval(tok,tokc))) {
          dsttype=INMGR_DSTTYPE_ACTION;
        } else {
          return inmgr_config_read_error(ctx,"Failed to evaluate '%.*s' as button or action name.",tokc,tok);
        }
      }
  }
  
  struct inmgr_rules_button *button=inmgr_rules_add_button(ctx->rules,srcbtnid,srcpart);
  if (!button) return inmgr_config_read_error(ctx,"Error adding button 0x%08x",srcbtnid);
  button->dsttype=dsttype;
  button->dstbtnid=dstbtnid;
  
  return 0;
}

/* Receive config file.
 */
 
int inmgr_receive_config(
  struct inmgr *inmgr,
  const char *src,int srcc,
  void (*cb_error)(const char *msg,int msgc,int lineno,void *userdata),
  void *userdata
) {
  struct inmgr_config_read_context ctx={
    .inmgr=inmgr,
    .rules=0,
    .lineno=0,
    .cb_error=cb_error,
    .userdata=userdata,
  };
  int srcp=0;
  while (srcp<srcc) {
    ctx.lineno++;
    const char *line=src+srcp;
    int linec=0,comment=0;
    while ((srcp<srcc)&&(src[srcp++]!=0x0a)) {
      if (src[srcp-1]=='#') comment=1;
      else if (!comment) linec++;
    }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    
    int linep=0;
    const char *word0=line;
    int word0c=0;
    while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) word0c++;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    
    if ((word0c==6)&&!memcmp(word0,"device",6)) {
      if (inmgr_config_read_block_start(&ctx,line+linep,linec-linep)<0) return -1;
    } else {
      if (inmgr_config_read_rule(&ctx,line,linec)<0) return -1;
    }
  }
  return 0;
}

/* Context for generating config file.
 */
 
struct inmgr_config_write_context {
  const struct inmgr *inmgr;
  char *dst;
  int dstc,dsta;
};

static int inmgr_config_write_context_cleanup(struct inmgr_config_write_context *ctx) {
  if (ctx->dst) free(ctx->dst);
  return -1;
}

static int inmgr_config_write_context_require(struct inmgr_config_write_context *ctx,int addc) {
  if (addc<1) return 0;
  if (ctx->dstc>INT_MAX-addc) return -1;
  int na=ctx->dstc+addc;
  if (na<=ctx->dsta) return 0;
  if (na<INT_MAX-1024) na=(na+1024)&~1023;
  void *nv=realloc(ctx->dst,na);
  if (!nv) return -1;
  ctx->dst=nv;
  ctx->dsta=na;
  return 0;
}

/* Append formatted text.
 */
 
static int inmgr_config_writef(struct inmgr_config_write_context *ctx,const char *fmt,...) {
  while (1) {
    va_list vargs;
    va_start(vargs,fmt);
    int err=vsnprintf(ctx->dst+ctx->dstc,ctx->dsta-ctx->dstc,fmt,vargs);
    if ((err<0)||(err==INT_MAX)) return -1;
    if (ctx->dstc<ctx->dsta-err) {
      ctx->dstc+=err;
      return 0;
    }
    if (inmgr_config_write_context_require(ctx,err+1)<0) return -1;
  }
}

/* Generate config file.
 */
 
int inmgr_generate_config(void *dstpp,const struct inmgr *inmgr) {
  struct inmgr_config_write_context ctx={
    .inmgr=inmgr,
  };
  #define APPEND(fmt,...) if (inmgr_config_writef(&ctx,fmt,##__VA_ARGS__)<0) return inmgr_config_write_context_cleanup(&ctx);

  APPEND("# WARNING: Comments and formatting will be dropped when the app auto-saves.\n")
  
  int rulesi=0;
  for (;rulesi<inmgr->rulesc;rulesi++) {
    const struct inmgr_rules *rules=inmgr->rulesv[rulesi];
    
    APPEND("\ndevice %04x %04x %.*s\n",rules->vid,rules->pid,rules->namec,rules->name)
    
    const struct inmgr_rules_button *button=rules->buttonv;
    int buttoni=rules->buttonc;
    for (;buttoni-->0;button++) {
    
      APPEND("  %08x %s",button->srcbtnid,inmgr_repr_srcpart(button->srcpart))
      switch (button->dsttype) {
      
        case INMGR_DSTTYPE_BUTTON: {
            if (!inmgr->delegate.btnid_repr) return inmgr_config_write_context_cleanup(&ctx);
            APPEND(" button ")
            while (1) {
              int err=inmgr->delegate.btnid_repr(ctx.dst+ctx.dstc,ctx.dsta-ctx.dstc,button->dstbtnid);
              if (err<=0) return inmgr_config_write_context_cleanup(&ctx);
              if (ctx.dstc<=ctx.dsta-err) {
                ctx.dstc+=err;
                break;
              }
              if (inmgr_config_write_context_require(&ctx,err)<0) return inmgr_config_write_context_cleanup(&ctx);
            }
          } break;
          
        case INMGR_DSTTYPE_ACTION: {
            if (!inmgr->delegate.actionid_repr) return inmgr_config_write_context_cleanup(&ctx);
            APPEND(" action ")
            while (1) {
              int err=inmgr->delegate.actionid_repr(ctx.dst+ctx.dstc,ctx.dsta-ctx.dstc,button->dstbtnid);
              if (err<=0) return inmgr_config_write_context_cleanup(&ctx);
              if (ctx.dstc<=ctx.dsta-err) {
                ctx.dstc+=err;
                break;
              }
              if (inmgr_config_write_context_require(&ctx,err)<0) return inmgr_config_write_context_cleanup(&ctx);
            }
          } break;
          
        default: return inmgr_config_write_context_cleanup(&ctx);
      }
      APPEND("\n")
    }
  }
  
  #undef APPEND
  *(void**)dstpp=ctx.dst;
  ctx.dst=0;
  inmgr_config_write_context_cleanup(&ctx);
  return ctx.dstc;
}

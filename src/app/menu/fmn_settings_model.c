#include "app/fmn_platform.h"
#include "fmn_settings_model.h"

/* Field value primitives.
 */
 
static void fmn_settings_field_write_enabled(char *dst,int dsta,struct fmn_settings_model *model,int enable) {
  const char *src;
  int srcc;
  if (enable) {
    src=model->enabletext;
    srcc=model->enabletextc;
  } else {
    src=model->disabletext;
    srcc=model->disabletextc;
  }
  if (srcc>dsta) srcc=dsta;
  memcpy(dst,src,srcc);
}

static void fmn_settings_field_write_iso631(char *dst,int dsta,struct fmn_settings_model *model,uint16_t lang) {
  char src[32];
  int srcc=fmn_get_string(src,sizeof(src),62); // string 62 should be the language's native name
  if ((srcc>0)&&(srcc<=sizeof(src))) {
    if (srcc>dsta) srcc=dsta;
    memcpy(dst,src,srcc);
    return;
  }
  // No name, that's fine, the code itself should be printable.
  if (dsta>=2) {
    dst[0]=lang>>8;
    dst[1]=lang;
    // Just in case we get an invalid language code:
    if ((dst[0]<'a')||(dst[0]>'z')||(dst[1]<'a')||(dst[1]>'z')) {
      dst[0]=dst[1]='?';
    }
  }
}

/* Overwrite text with field's current value.
 * (dst) null, we'll find it on our own.
 */
 
static void fmn_settings_field_write_value(
  struct fmn_settings_model *model,
  struct fmn_settings_field *field,
  char *dst,int dsta
) {
  if (!dst) {
    dst=model->text+field->displayrow*FMN_SETTINGS_COLC+21;
    dsta=FMN_SETTINGS_COLC-21;
  }
  if (dsta<1) return;
  memset(dst,0,dsta);
  switch (field->id) {
    case FMN_SETTINGS_FIELD_FULLSCREEN: fmn_settings_field_write_enabled(dst,dsta,model,model->platform.fullscreen_enable); break;
    case FMN_SETTINGS_FIELD_MUSIC_ENABLE: fmn_settings_field_write_enabled(dst,dsta,model,model->platform.music_enable); break;
    case FMN_SETTINGS_FIELD_LANGUAGE: fmn_settings_field_write_iso631(dst,dsta,model,model->platform.language); break;
    case FMN_SETTINGS_FIELD_INPUT: memset(dst,'.',3); break;
  }
}

/* Add a field to the list.
 * Does not establish output row yet.
 */
 
static void fmn_settings_model_add_field(struct fmn_settings_model *model,uint16_t id) {
  if (model->fieldc>=FMN_SETTINGS_FIELD_LIMIT) return;
  struct fmn_settings_field *field=model->fieldv+model->fieldc++;
  field->id=id;
}

/* Finalize field list.
 * Mostly this means assign (displayrow) in each.
 */
 
static void fmn_settings_model_finalize_fields(struct fmn_settings_model *model) {
  //TODO If it's possible to not have any fields selected, add some placeholder text.
  int8_t y=(FMN_SETTINGS_ROWC>>1)-(model->fieldc>>1);
  char *text=model->text+y*FMN_SETTINGS_COLC;
  const int keycolw=FMN_SETTINGS_COLC>>1;
  struct fmn_settings_field *field=model->fieldv;
  int i=model->fieldc;
  for (;i-->0;field++,y++,text+=FMN_SETTINGS_COLC) {
    field->displayrow=y;
    int labelc=fmn_get_string(text,keycolw,field->id);
    if ((labelc>0)&&(labelc<=keycolw)) {
      int j=labelc;
      char *label=text;
      for (;j-->0;label++) if (*label==0x20) *label=0;
      memmove(text+keycolw-labelc,text,labelc); // align right in left column
      memset(text,0,keycolw-labelc);
      fmn_settings_field_write_value(model,field,text+keycolw+1,FMN_SETTINGS_COLC-keycolw-1);
    }
  }
}

/* Helper strings.
 */
 
static void fmn_settings_model_reload_helper_strings(struct fmn_settings_model *model) {
  if ((model->enabletextc=fmn_get_string(model->enabletext,sizeof(model->enabletext),60))>sizeof(model->enabletext)) {
    model->enabletext[0]='1';
    model->enabletextc=1;
  }
  if ((model->disabletextc=fmn_get_string(model->disabletext,sizeof(model->disabletext),61))>sizeof(model->disabletext)) {
    model->disabletext[0]='0';
    model->disabletextc=1;
  }
}

/* Init.
 */
 
void fmn_settings_model_init(struct fmn_settings_model *model) {
  memset(model->text,0,sizeof(model->text));
  model->selrow=0;
  model->fieldc=0;
  model->fieldp=0;
  
  fmn_platform_get_settings(&model->platform);
  fmn_settings_model_reload_helper_strings(model);
  
  fmn_settings_model_add_field(model,FMN_SETTINGS_FIELD_LANGUAGE);
  if (model->platform.fullscreen_available) fmn_settings_model_add_field(model,FMN_SETTINGS_FIELD_FULLSCREEN);
  if (model->platform.music_available) fmn_settings_model_add_field(model,FMN_SETTINGS_FIELD_MUSIC_ENABLE);
  fmn_settings_model_add_field(model,FMN_SETTINGS_FIELD_INPUT);
  
  fmn_settings_model_finalize_fields(model);
  
  fmn_settings_model_move(model,0); // forces selrow valid, possibly other volatile state
}

/* Move focus to another row.
 */
 
void fmn_settings_model_move(struct fmn_settings_model *model,int d) {
  if (model->fieldc<1) return;
  model->fieldp+=d;
  if (model->fieldp<0) model->fieldp=model->fieldc-1;
  else if (model->fieldp>=model->fieldc) model->fieldp=0;
  model->selrow=model->fieldv[model->fieldp].displayrow;
}

/* Adjust value at current focus.
 */
 
void fmn_settings_model_adjust(struct fmn_settings_model *model,int d) {
  if (model->fieldc<1) return;
  struct fmn_settings_field *field=model->fieldv+model->fieldp;
  switch (field->id) {
    case FMN_SETTINGS_FIELD_FULLSCREEN: model->platform.fullscreen_enable^=1; break;
    case FMN_SETTINGS_FIELD_MUSIC_ENABLE: model->platform.music_enable^=1; break;
    case FMN_SETTINGS_FIELD_LANGUAGE: model->platform.language=(d>0)?fmn_platform_get_next_language(model->platform.language):fmn_platform_get_prev_language(model->platform.language); break;
    case FMN_SETTINGS_FIELD_INPUT: return; // no value
    default: return;
  }
  fmn_settings_field_write_value(model,field,0,0);
  fmn_platform_set_settings(&model->platform);
}

/* Get selected field.
 */
 
uint16_t fmn_settings_model_get_field_id(const struct fmn_settings_model *model) {
  if (!model) return 0;
  if (model->fieldp<0) return 0;
  if (model->fieldp>=model->fieldc) return 0;
  return model->fieldv[model->fieldp].id;
}

/* Language changed: Rebuild all text.
 */
 
void fmn_settings_model_language_changed(struct fmn_settings_model *model) {
  fmn_settings_model_reload_helper_strings(model);
  fmn_settings_model_finalize_fields(model);
}

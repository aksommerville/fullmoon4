/* fmn_settings_model.h
 * Underlying content and layout for the Settings menu.
 * We don't interact directly with fmn_menu or the renderer; fmn_menu_settings.c coordinates all that.
 */
 
#ifndef FMN_SETTINGS_MODEL_H
#define FMN_SETTINGS_MODEL_H

#define FMN_SETTINGS_COLC 40
#define FMN_SETTINGS_ROWC 24
#define FMN_SETTINGS_FIELD_LIMIT FMN_SETTINGS_ROWC

/* Field IDs are string resource IDs.
 */
#define FMN_SETTINGS_FIELD_FULLSCREEN 51 /* boolean */
#define FMN_SETTINGS_FIELD_MUSIC_ENABLE 52 /* boolean */
#define FMN_SETTINGS_FIELD_LANGUAGE 53 /* enum */
#define FMN_SETTINGS_FIELD_INPUT 54 /* interactive */
#define FMN_SETTINGS_FIELD_ZAP_SAVE 55 /* interactive */
//TODO video driver settings?
//TODO audio driver settings?
//TODO driver selection?

struct fmn_settings_model {

  /* Our output is a text terminal.
   * Zeroes should not be rendered.
   * Everything else, including space and C0, should (though those glyphs are probably empty).
   */
  char text[FMN_SETTINGS_COLC*FMN_SETTINGS_ROWC];
  
  int8_t selrow; // 0..FMN_SETTINGS_ROWC-1; owner should highlight
  
  struct fmn_settings_field {
    int8_t displayrow; // 0..FMN_SETTINGS_ROWC-1; must be sorted
    uint16_t id;
  } fieldv[FMN_SETTINGS_FIELD_LIMIT];
  uint8_t fieldc;
  int8_t fieldp;
  
  char enabletext[20],disabletext[20];
  uint8_t enabletextc,disabletextc;
  
  struct fmn_platform_settings platform;
};

void fmn_settings_model_init(struct fmn_settings_model *model);
void fmn_settings_model_move(struct fmn_settings_model *model,int d); // vertical
void fmn_settings_model_adjust(struct fmn_settings_model *model,int d); // horizontal
uint16_t fmn_settings_model_get_field_id(const struct fmn_settings_model *model);
void fmn_settings_model_language_changed(struct fmn_settings_model *model);

#endif

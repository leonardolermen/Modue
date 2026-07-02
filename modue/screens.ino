// ============================================================
//  screens.ino — telas Discord/Spotify, widgets e callbacks de toque
//  (globais e includes vivem no modue.ino; txln vive no protocol.ino)
// ============================================================

// ---- Evento comum: fader na tela ----
void slider_event_cb(lv_event_t *e) {
  int v = lv_slider_get_value(slider);
  lv_label_set_text_fmt(volLabel, "%d", v);
  if (suppressEv) return;
  static uint32_t lastTx = 0; static int lastV = -1;
  bool released = (lv_event_get_code(e) == LV_EVENT_RELEASED);
  uint32_t now = millis();
  if (v != lastV && (released || now - lastTx > 80)) {
    char b[16]; snprintf(b, sizeof(b), "EV VOL %d", v); txln(b); lastTx = now; lastV = v;
  }
}

// ---- Eventos Discord ----
void mic_event_cb(lv_event_t *e) {
  bool m = lv_obj_has_state(micBtn, LV_STATE_CHECKED);
  lv_img_set_src(micImg, m ? &img_mic_muted : &img_mic);
  if (!suppressEv) { char b[20]; snprintf(b, sizeof(b), "EV BTN mic %d", m); txln(b); }
}
void fone_event_cb(lv_event_t *e) {
  bool m = lv_obj_has_state(foneBtn, LV_STATE_CHECKED);
  lv_label_set_text(foneLbl, m ? LV_SYMBOL_MUTE : LV_SYMBOL_VOLUME_MAX);
  lv_obj_set_style_text_color(foneLbl, lv_color_hex(m ? C_RED : C_LIGHT), 0);
  if (!suppressEv) { char b[20]; snprintf(b, sizeof(b), "EV BTN deaf %d", m); txln(b); }
}

// ---- Eventos Spotify ----
void transport_cb(lv_event_t *e) {
  const char *id = (const char *)lv_event_get_user_data(e);
  lv_obj_t *btn = lv_event_get_target(e);
  if (!strcmp(id, "play")) {
    bool playing = lv_obj_has_state(btn, LV_STATE_CHECKED);
    lv_label_set_text(playLbl, playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    if (!suppressEv) { char b[20]; snprintf(b, sizeof(b), "EV BTN play %d", playing); txln(b); }
  } else if (!suppressEv) {
    char b[20]; snprintf(b, sizeof(b), "EV BTN %s 1", id); txln(b);
  }
}

// ---- Slider comum ----
lv_obj_t *common_slider(lv_obj_t *scr, int y) {
  lv_obj_t *s = lv_slider_create(scr);
  lv_obj_set_size(s, 140, 22);
  lv_obj_align(s, LV_ALIGN_TOP_MID, 0, y);
  lv_slider_set_range(s, 0, 100);
  lv_slider_set_value(s, 50, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(s, lv_color_hex(C_TRACK), LV_PART_MAIN);
  lv_obj_set_style_radius(s, 11, LV_PART_MAIN);
  lv_obj_set_style_bg_color(s, lv_color_hex(accent), LV_PART_INDICATOR);
  lv_obj_set_style_radius(s, 11, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(s, lv_color_hex(0xffffff), LV_PART_KNOB);
  lv_obj_set_style_pad_all(s, 4, LV_PART_KNOB);
  lv_obj_add_event_cb(s, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(s, slider_event_cb, LV_EVENT_RELEASED, NULL);
  return s;
}

// ---- Tela Discord ----
void disc_btn_style(lv_obj_t *b, int x) {
  lv_obj_set_size(b, 64, 64);
  lv_obj_align(b, LV_ALIGN_BOTTOM_MID, x, -12);
  lv_obj_add_flag(b, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_style_radius(b, 16, 0);
  lv_obj_set_style_bg_color(b, lv_color_hex(C_BTN), 0);
  lv_obj_set_style_border_width(b, 0, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_set_style_bg_color(b, lv_color_hex(C_BTN_MUTE), LV_STATE_CHECKED);
  lv_obj_set_style_border_width(b, 1, LV_STATE_CHECKED);
  lv_obj_set_style_border_color(b, lv_color_hex(C_RED), LV_STATE_CHECKED);
}

void build_discord(lv_obj_t *scr) {
  lv_obj_t *logo = lv_obj_create(scr);
  lv_obj_set_size(logo, 132, 132);
  lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 8);
  lv_obj_set_style_radius(logo, 30, 0);
  lv_obj_set_style_clip_corner(logo, true, 0);
  lv_obj_set_style_bg_color(logo, lv_color_hex(0x5865F2), 0);
  lv_obj_set_style_border_width(logo, 0, 0);
  lv_obj_set_style_pad_all(logo, 0, 0);
  lv_obj_clear_flag(logo, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *li = lv_img_create(logo);
  lv_img_set_src(li, &img_discord);
  lv_obj_center(li);

  slider = common_slider(scr, 152);
  lv_slider_set_value(slider, 72, LV_ANIM_OFF);

  volLabel = lv_label_create(scr);
  lv_label_set_text(volLabel, "72");
  lv_obj_set_style_text_color(volLabel, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(volLabel, &lv_font_montserrat_40, 0);
  lv_obj_align(volLabel, LV_ALIGN_TOP_MID, 0, 182);

  micBtn = lv_btn_create(scr);
  disc_btn_style(micBtn, -36);
  micImg = lv_img_create(micBtn);
  lv_img_set_src(micImg, &img_mic);
  lv_obj_center(micImg);
  lv_obj_add_event_cb(micBtn, mic_event_cb, LV_EVENT_CLICKED, NULL);

  foneBtn = lv_btn_create(scr);
  disc_btn_style(foneBtn, 36);
  foneLbl = lv_label_create(foneBtn);
  lv_label_set_text(foneLbl, LV_SYMBOL_VOLUME_MAX);
  lv_obj_set_style_text_color(foneLbl, lv_color_hex(C_LIGHT), 0);
  lv_obj_set_style_text_font(foneLbl, &lv_font_montserrat_20, 0);
  lv_obj_center(foneLbl);
  lv_obj_add_event_cb(foneBtn, fone_event_cb, LV_EVENT_CLICKED, NULL);
}

// ---- Tela Spotify ----
lv_obj_t *transport_btn(lv_obj_t *scr, const char *sym, int x, int sz,
                        bool checkable, const char *id) {
  lv_obj_t *b = lv_btn_create(scr);
  lv_obj_set_size(b, sz, sz);
  lv_obj_align(b, LV_ALIGN_BOTTOM_MID, x, -14);
  lv_obj_set_style_radius(b, sz / 2, 0);  // redondo
  lv_obj_set_style_bg_color(b, lv_color_hex(checkable ? accent : C_BTN), 0);
  lv_obj_set_style_border_width(b, 0, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_pad_all(b, 0, 0);
  if (checkable) {
    lv_obj_add_flag(b, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(b, lv_color_hex(accent), LV_STATE_CHECKED);
  }
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, sym);
  lv_obj_set_style_text_color(l, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
  lv_obj_center(l);
  lv_obj_add_event_cb(b, transport_cb, LV_EVENT_CLICKED, (void *)id);
  if (!strcmp(id, "play")) playLbl = l;
  return b;
}

void build_spotify(lv_obj_t *scr) {
  // Capa do album: container arredondado (clip) + imagem preenchida via IMG
  lv_obj_t *art = lv_obj_create(scr);
  lv_obj_set_size(art, 140, 140);
  lv_obj_align(art, LV_ALIGN_TOP_MID, 0, 8);
  lv_obj_set_style_radius(art, 16, 0);
  lv_obj_set_style_clip_corner(art, true, 0);
  lv_obj_set_style_bg_color(art, lv_color_hex(0x1f2430), 0);
  lv_obj_set_style_border_width(art, 0, 0);
  lv_obj_set_style_pad_all(art, 0, 0);
  lv_obj_clear_flag(art, LV_OBJ_FLAG_SCROLLABLE);
  artImg = lv_img_create(art);
  lv_obj_center(artImg);
  if (artBuf && artDsc.data) lv_img_set_src(artImg, &artDsc);  // reusa ultima capa

  // Nome da faixa (rola se for grande)
  trackLabel = lv_label_create(scr);
  lv_label_set_long_mode(trackLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(trackLabel, 164);
  lv_label_set_text(trackLabel, "—");
  lv_obj_set_style_text_align(trackLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(trackLabel, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(trackLabel, &lv_font_montserrat_14, 0);
  lv_obj_align(trackLabel, LV_ALIGN_TOP_MID, 0, 156);

  slider = common_slider(scr, 192);  // fader mais pra baixo
  volLabel = lv_label_create(scr);
  lv_obj_add_flag(volLabel, LV_OBJ_FLAG_HIDDEN);

  // Transporte: anterior · play/pause · proxima
  prevBtn = transport_btn(scr, LV_SYMBOL_PREV, -56, 48, false, "prev");
  playBtn = transport_btn(scr, LV_SYMBOL_PLAY, 0, 58, true, "play");
  nextBtn = transport_btn(scr, LV_SYMBOL_NEXT, 56, 48, false, "next");
}

// ---- Troca de tela ----
void switch_screen(Screen s) {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_clean(scr);
  micBtn = foneBtn = micImg = foneLbl = NULL;
  trackLabel = prevBtn = playBtn = playLbl = nextBtn = artImg = NULL;
  lv_obj_set_style_bg_color(scr, lv_color_hex(C_BG), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  current = s;
  if (s == SCR_SPOTIFY) build_spotify(scr); else build_discord(scr);
}

// ---- Aplica STATE do PC (sem gerar EV) ----
void setChecked(lv_obj_t *btn, bool on) {
  if (!btn) return;
  suppressEv = true;
  if (on) lv_obj_add_state(btn, LV_STATE_CHECKED);
  else lv_obj_clear_state(btn, LV_STATE_CHECKED);
  lv_event_send(btn, LV_EVENT_CLICKED, NULL);
  suppressEv = false;
}

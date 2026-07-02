// ============================================================
//  protocol.ino — comunicação serial com o PC (ver PROTOCOL.md)
//  Comandos do PC: PING/SCREEN/VOL/STATE/LABEL/IMG/COLOR
//  Eventos pro PC: HELLO/PONG/EV VOL/EV BTN (emitidos nos outros arquivos)
// ============================================================

// Envia uma linha pro PC sem bloquear (se o serial nao estiver pronto, descarta).
void txln(const char *s) {
  if (Serial && Serial.availableForWrite() > (int)strlen(s) + 2) Serial.println(s);
}

// Monta o descritor da capa recebida e mostra no display.
void finishImage() {
  artDsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  artDsc.header.always_zero = 0;
  artDsc.header.reserved = 0;
  artDsc.header.w = imgW;
  artDsc.header.h = imgH;
  artDsc.data_size = (size_t)imgW * imgH * 2;
  artDsc.data = artBuf;
  if (artImg) { lv_img_set_src(artImg, &artDsc); lv_obj_invalidate(artImg); }
}

// Interpreta um comando de texto vindo do PC.
void handleCommand(char *s) {
  if (!strcmp(s, "PING")) { txln("PONG"); return; }

  if (!strncmp(s, "SCREEN ", 7)) {
    switch_screen(!strcmp(s + 7, "spotify") ? SCR_SPOTIFY : SCR_DISCORD);
    return;
  }
  if (!strncmp(s, "VOL ", 4)) {
    int v = constrain(atoi(s + 4), 0, 100);
    suppressEv = true;
    if (slider) lv_slider_set_value(slider, v, LV_ANIM_OFF);
    if (volLabel) lv_label_set_text_fmt(volLabel, "%d", v);
    suppressEv = false;
    return;
  }
  if (!strncmp(s, "STATE ", 6)) {
    char w[8]; int on = 0;
    if (sscanf(s + 6, "%7s %d", w, &on) == 2) {
      if (!strcmp(w, "mic")) setChecked(micBtn, on);
      else if (!strcmp(w, "deaf")) setChecked(foneBtn, on);
      else if (!strcmp(w, "play")) setChecked(playBtn, on);
    }
    return;
  }
  if (!strncmp(s, "LABEL ", 6)) {
    if (trackLabel) lv_label_set_text(trackLabel, s + 6);
    return;
  }
  if (!strncmp(s, "IMG ", 4)) {
    char slot[8]; int w = 0, h = 0;
    if (sscanf(s + 4, "%7s %d %d", slot, &w, &h) == 3) {
      size_t n = (size_t)w * h * 2;
      if (!artBuf) artBuf = (uint8_t *)malloc(MAX_ART);
      if (artBuf && n > 0 && n <= MAX_ART) {
        // arma o modo recepcao; os bytes sao lidos no loop (sem bloquear)
        imgW = w; imgH = h; imgRemaining = n; imgPos = 0; imgLast = millis();
      }
    }
    return;
  }
  if (!strncmp(s, "COLOR ", 6)) {
    accent = (uint32_t)strtol(s + 6, NULL, 16);
    if (slider) lv_obj_set_style_bg_color(slider, lv_color_hex(accent), LV_PART_INDICATOR);
    return;
  }
}

// Le o serial: bytes de imagem (modo recepcao) ou linhas de comando.
void pollSerial() {
  // Modo recepcao de imagem: consome bytes crus, sem bloquear o loop
  if (imgRemaining > 0) {
    bool got = false;
    while (Serial.available() && imgRemaining > 0) {
      artBuf[imgPos++] = (uint8_t)Serial.read();
      imgRemaining--;
      got = true;
    }
    if (got) imgLast = millis();
    if (imgRemaining == 0) finishImage();
    else if (millis() - imgLast > 1500) { imgRemaining = 0; }  // timeout: nao trava
    return;
  }
  // Modo linha (comandos de texto)
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (lineLen > 0) { lineBuf[lineLen] = 0; handleCommand(lineBuf); lineLen = 0; }
      if (imgRemaining > 0) return;  // entrou em modo imagem; le os bytes na proxima passada
    } else if (lineLen < sizeof(lineBuf) - 1) lineBuf[lineLen++] = c;
  }
}

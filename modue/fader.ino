// ============================================================
//  fader.ino — leitura do fader físico (potenciômetro via ADC)
//  + curva de calibração que lineariza o taper do fader.
//  (config FADER_* e globais vivem no modue.ino)
// ============================================================
#if FADER_ENABLED

// Curva de calibracao DESTE fader (mV medido -> % de posicao fisica).
// Lineariza o taper torto interpolando entre os pontos medidos.
// Pra recalibrar: meça o mV (DBG) em cada posicao e atualize as tabelas.
int faderCurve(int mv) {
  static const int MV[]  = {8, 581, 1134, 1481, 2915};  // mV em cada posicao
  static const int POS[] = {0,  25,   50,   75,  100};  // % fisico correspondente
  const int N = 5;
  if (mv <= MV[0]) return 0;
  if (mv >= MV[N - 1]) return 100;
  for (int i = 1; i < N; i++) {
    if (mv <= MV[i]) {
      return POS[i - 1] + (int)((long)(mv - MV[i - 1]) * (POS[i] - POS[i - 1]) / (MV[i] - MV[i - 1]));
    }
  }
  return 100;
}

void readFader() {
  static uint32_t last = 0;
  if (millis() - last < 40) return;  // ~25 Hz
  last = millis();

  static float ema = -1;
  int raw = analogReadMilliVolts(FADER_PIN);          // mV calibrado (linear, ~0..3300)
  ema = (ema < 0) ? raw : ema * 0.75f + raw * 0.25f;  // suaviza ruido

  int v = faderCurve((int)ema);  // -> volume linear com a posicao fisica

#if FADER_DEBUG
  static uint32_t dbg = 0;
  if (millis() - dbg > 250) {
    dbg = millis();
    char b[48]; snprintf(b, sizeof(b), "DBG fader raw=%d ema=%d vol=%d", raw, (int)ema, v);
    txln(b);
  }
#endif

  static int lastV = -1;
  if (lastV < 0 || abs(v - lastV) >= 2) {  // deadband anti-jitter
    lastV = v;
    suppressEv = true;
    if (slider) lv_slider_set_value(slider, v, LV_ANIM_OFF);
    if (volLabel) lv_label_set_text_fmt(volLabel, "%d", v);
    suppressEv = false;
    char b[16]; snprintf(b, sizeof(b), "EV VOL %d", v); txln(b);
  }
}

#endif  // FADER_ENABLED

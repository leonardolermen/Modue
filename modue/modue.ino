// ============================================================
//  Mixer Modular Magnético — Firmware dinâmico (controlado pelo PC)
//  Placa: SpotPear/Waveshare ESP32-C6-Touch-LCD-1.47 (JD9853 + AXS5106L)
//
//  Renderizador genérico com telas trocáveis (Discord / Spotify).
//  PC manda comandos (SCREEN/VOL/STATE/COLOR/LABEL); módulo devolve
//  eventos (EV VOL / EV BTN). Ver PROTOCOL.md.
//
//  Libs: GFX Library for Arduino, lvgl 8.4, esp_lcd_touch_axs5106l
//  Board: ESP32C6 Dev Module | USB CDC On Boot: Enabled
// ============================================================

#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include "esp_lcd_touch_axs5106l.h"
#include "icons.h"  // img_discord, img_mic, img_mic_muted (defaults)

#define FW_VERSION "0.3.0"

// ---- Display JD9853 ----
#define GFX_BL 23
Arduino_DataBus *bus = new Arduino_HWSPI(15, 14, 1, 2);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 22, 0, false, 172, 320, 34, 0, 34, 0);

// ---- Touch AXS5106L ----
#define TP_SDA 18
#define TP_SCL 19
#define TP_RST 20
#define TP_INT 21

// ---- Fader fisico (potenciometro do ALPS, via ADC) ----
#define FADER_ENABLED 1   // 0 = desliga (use 0 enquanto NAO ligou o fader no GPIO3)
#define FADER_DEBUG   0   // 1 = imprime o valor cru do ADC no serial (debug)
#define FADER_PIN 3       // wiper do potenciometro -> ADC1_CH3

// ---- Cores ----
#define C_BG       0x0a0b10
#define C_TRACK    0x1c1f29
#define C_BTN      0x15171f
#define C_BTN_MUTE 0x2b1417
#define C_RED      0xe24b4a
#define C_LIGHT    0xcfd2da
static uint32_t accent = 0x5865F2;

// Tipos no topo (o Arduino gera protótipos antes das declarações abaixo)
enum Screen { SCR_DISCORD, SCR_SPOTIFY };

// ---- Init proprio do JD9853 ----
void lcd_reg_init(void) {
  static const uint8_t op[] = {
    BEGIN_WRITE, WRITE_COMMAND_8, 0x11, END_WRITE, DELAY, 120,
    BEGIN_WRITE,
    WRITE_C8_D16, 0xDF, 0x98, 0x53,
    WRITE_C8_D8, 0xB2, 0x23,
    WRITE_COMMAND_8, 0xB7, WRITE_BYTES, 4, 0x00, 0x47, 0x00, 0x6F,
    WRITE_COMMAND_8, 0xBB, WRITE_BYTES, 6, 0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0,
    WRITE_C8_D16, 0xC0, 0x44, 0xA4,
    WRITE_C8_D8, 0xC1, 0x16,
    WRITE_COMMAND_8, 0xC3, WRITE_BYTES, 8, 0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77,
    WRITE_COMMAND_8, 0xC4, WRITE_BYTES, 12, 0x00, 0x00, 0xA0, 0x79, 0x0B, 0x0A, 0x16, 0x79, 0x0B, 0x0A, 0x16, 0x82,
    WRITE_COMMAND_8, 0xC8, WRITE_BYTES, 32,
      0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,
      0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,
    WRITE_COMMAND_8, 0xD0, WRITE_BYTES, 5, 0x04, 0x06, 0x6B, 0x0F, 0x00,
    WRITE_C8_D16, 0xD7, 0x00, 0x30,
    WRITE_C8_D8, 0xE6, 0x14,
    WRITE_C8_D8, 0xDE, 0x01,
    WRITE_COMMAND_8, 0xB7, WRITE_BYTES, 5, 0x03, 0x13, 0xEF, 0x35, 0x35,
    WRITE_COMMAND_8, 0xC1, WRITE_BYTES, 3, 0x14, 0x15, 0xC0,
    WRITE_C8_D16, 0xC2, 0x06, 0x3A,
    WRITE_C8_D16, 0xC4, 0x72, 0x12,
    WRITE_C8_D8, 0xBE, 0x00,
    WRITE_C8_D8, 0xDE, 0x02,
    WRITE_COMMAND_8, 0xE5, WRITE_BYTES, 3, 0x00, 0x02, 0x00,
    WRITE_COMMAND_8, 0xE5, WRITE_BYTES, 3, 0x01, 0x02, 0x00,
    WRITE_C8_D8, 0xDE, 0x00,
    WRITE_C8_D8, 0x35, 0x00,
    WRITE_C8_D8, 0x3A, 0x05,
    WRITE_COMMAND_8, 0x2A, WRITE_BYTES, 4, 0x00, 0x22, 0x00, 0xCD,
    WRITE_COMMAND_8, 0x2B, WRITE_BYTES, 4, 0x00, 0x00, 0x01, 0x3F,
    WRITE_C8_D8, 0xDE, 0x02,
    WRITE_COMMAND_8, 0xE5, WRITE_BYTES, 3, 0x00, 0x02, 0x00,
    WRITE_C8_D8, 0xDE, 0x00,
    WRITE_C8_D8, 0x36, 0x00,
    WRITE_COMMAND_8, 0x21,
    END_WRITE, DELAY, 10,
    BEGIN_WRITE, WRITE_COMMAND_8, 0x29, END_WRITE
  };
  bus->batchOperation(op, sizeof(op));
}

// ---- LVGL glue ----
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;
static lv_disp_drv_t disp_drv;

void my_disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = area->x2 - area->x1 + 1, h = area->y2 - area->y1 + 1;
#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
  lv_disp_flush_ready(drv);
}

void touchpad_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  touch_data_t td;
  bsp_touch_read();
  if (bsp_touch_get_coordinates(&td)) {
    data->point.x = td.coords[0].x;
    data->point.y = td.coords[0].y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else data->state = LV_INDEV_STATE_RELEASED;
}

void txln(const char *s) {
  if (Serial && Serial.availableForWrite() > (int)strlen(s) + 2) Serial.println(s);
}

// ---- Estado / widgets ----
static Screen current = SCR_DISCORD;
static bool suppressEv = false;

static lv_obj_t *slider, *volLabel;          // comuns
static lv_obj_t *micBtn, *micImg, *foneBtn, *foneLbl;  // discord
static lv_obj_t *trackLabel, *prevBtn, *playBtn, *playLbl, *nextBtn, *artImg;  // spotify

// Buffer/descritor da capa do album (recebida via comando IMG)
#define MAX_ART 40000  // 140*140*2 = 39200
static uint8_t *artBuf = NULL;
static lv_img_dsc_t artDsc;
// Recepcao de imagem nao-bloqueante (le os bytes aos poucos no loop)
static int imgW = 0, imgH = 0;
static size_t imgRemaining = 0, imgPos = 0;
static uint32_t imgLast = 0;  // pra timeout

// ---- Eventos comuns ----
static void slider_event_cb(lv_event_t *e) {
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
static void mic_event_cb(lv_event_t *e) {
  bool m = lv_obj_has_state(micBtn, LV_STATE_CHECKED);
  lv_img_set_src(micImg, m ? &img_mic_muted : &img_mic);
  if (!suppressEv) { char b[20]; snprintf(b, sizeof(b), "EV BTN mic %d", m); txln(b); }
}
static void fone_event_cb(lv_event_t *e) {
  bool m = lv_obj_has_state(foneBtn, LV_STATE_CHECKED);
  lv_label_set_text(foneLbl, m ? LV_SYMBOL_MUTE : LV_SYMBOL_VOLUME_MAX);
  lv_obj_set_style_text_color(foneLbl, lv_color_hex(m ? C_RED : C_LIGHT), 0);
  if (!suppressEv) { char b[20]; snprintf(b, sizeof(b), "EV BTN deaf %d", m); txln(b); }
}

// ---- Eventos Spotify ----
static void transport_cb(lv_event_t *e) {
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

// ---- Helpers de estilo ----
static lv_obj_t *common_slider(lv_obj_t *scr, int y) {
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
static void disc_btn_style(lv_obj_t *b, int x) {
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
static lv_obj_t *transport_btn(lv_obj_t *scr, const char *sym, int x, int sz,
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
  txln(s == SCR_SPOTIFY ? "DBG screen=spotify" : "DBG screen=discord");
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

// ---- Parser de comandos ----
static char lineBuf[160];
static uint8_t lineLen = 0;

void handleCommand(char *s) {
  if (!strcmp(s, "PING")) { txln("PONG"); return; }

  if (!strncmp(s, "SCREEN ", 7)) {
    if (!strcmp(s + 7, "spotify")) switch_screen(SCR_SPOTIFY);
    else switch_screen(SCR_DISCORD);
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
    if (trackLabel) { lv_label_set_text(trackLabel, s + 6); txln("DBG label=set"); }
    else txln("DBG label=NULL-trackLabel");
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
        txln(artImg ? "DBG img=arm" : "DBG img=arm-NULL-artImg");
      } else txln("DBG img=rejeitado");
    }
    return;
  }
  if (!strncmp(s, "COLOR ", 6)) {
    accent = (uint32_t)strtol(s + 6, NULL, 16);
    if (slider) lv_obj_set_style_bg_color(slider, lv_color_hex(accent), LV_PART_INDICATOR);
    return;
  }
}

void finishImage() {
  artDsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  artDsc.header.always_zero = 0;
  artDsc.header.reserved = 0;
  artDsc.header.w = imgW;
  artDsc.header.h = imgH;
  artDsc.data_size = (size_t)imgW * imgH * 2;
  artDsc.data = artBuf;
  if (artImg) { lv_img_set_src(artImg, &artDsc); lv_obj_invalidate(artImg); }
  txln(artImg ? "DBG img=done" : "DBG img=done-NULL-artImg");
}

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
    else if (millis() - imgLast > 1500) { imgRemaining = 0; txln("DBG img=timeout"); }
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

void setup() {
  Serial.setRxBufferSize(MAX_ART + 2048);  // segura o jorro da imagem sem perder bytes
  Serial.begin(115200);
  delay(300);

#if FADER_ENABLED
  // ADC: por padrao o ESP32 satura em ~1.1V. Com 12dB le o range ~0..3.3V.
  analogReadResolution(12);
  analogSetPinAttenuation(FADER_PIN, ADC_11db);
#endif

  if (!gfx->begin()) Serial.println("gfx->begin() FALHOU");
  lcd_reg_init();
  gfx->setRotation(0);
  gfx->fillScreen(RGB565_BLACK);
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  Wire.begin(TP_SDA, TP_SCL);
  bsp_touch_init(&Wire, TP_RST, TP_INT, gfx->getRotation(), gfx->width(), gfx->height());

  lv_init();
  uint32_t bufSize = 172 * 40;
  buf1 = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!buf1) buf1 = (lv_color_t *)malloc(bufSize * 2);
  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, bufSize);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 172;
  disp_drv.ver_res = 320;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touchpad_read_cb;
  lv_indev_drv_register(&indev_drv);

  switch_screen(SCR_DISCORD);
  txln("HELLO modue " FW_VERSION);
}

#if FADER_ENABLED
// Curva de calibracao DESTE fader (mV medido -> % de posicao fisica).
// Linearia o taper torto: interpola entre os pontos medidos.
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
  int raw = analogReadMilliVolts(FADER_PIN);       // mV calibrado (linear, ~0..3300)
  ema = (ema < 0) ? raw : ema * 0.75f + raw * 0.25f;  // suaviza ruido

  // Aplica a curva de calibracao -> volume linear com a posicao fisica
  int v = faderCurve((int)ema);

#if FADER_DEBUG
  static uint32_t dbg = 0;
  if (millis() - dbg > 250) {
    dbg = millis();
    char b[48]; snprintf(b, sizeof(b), "DBG fader raw=%d ema=%d vol=%d", raw, (int)ema, v);
    txln(b);
  }
#endif
  static int lastV = -1;
  if (lastV < 0 || abs(v - lastV) >= 2) {          // deadband anti-jitter
    lastV = v;
    suppressEv = true;
    if (slider) lv_slider_set_value(slider, v, LV_ANIM_OFF);
    if (volLabel) lv_label_set_text_fmt(volLabel, "%d", v);
    suppressEv = false;
    char b[16]; snprintf(b, sizeof(b), "EV VOL %d", v); txln(b);
  }
}
#endif

void loop() {
  pollSerial();
  // Enquanto recebe a imagem, drena rapido (sem desenhar) pra nao perder bytes.
  if (imgRemaining > 0) return;
#if FADER_ENABLED
  readFader();
#endif
  lv_timer_handler();
  delay(5);
}

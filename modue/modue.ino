// ============================================================
//  Mixer Modular Magnético — Firmware dinâmico (controlado pelo PC)
//  Placa: SpotPear/Waveshare ESP32-C6-Touch-LCD-1.47 (JD9853 + AXS5106L)
//
//  Renderizador genérico: o PC manda comandos (VOL/STATE/COLOR) e o
//  módulo devolve eventos (EV VOL/EV BTN). Ver PROTOCOL.md.
//
//  Libs: GFX Library for Arduino, lvgl 8.4, esp_lcd_touch_axs5106l
//  Board: ESP32C6 Dev Module | USB CDC On Boot: Enabled
// ============================================================

#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include "esp_lcd_touch_axs5106l.h"
#include "icons.h"  // logo padrao (default ate o PC mandar imagem)

#define FW_VERSION "0.2.0"

// ---- Display JD9853 ----
#define GFX_BL 23
Arduino_DataBus *bus = new Arduino_HWSPI(15, 14, 1, 2);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 22, 0, false, 172, 320, 34, 0, 34, 0);

// ---- Touch AXS5106L ----
#define TP_SDA 18
#define TP_SCL 19
#define TP_RST 20
#define TP_INT 21

// ---- Cores ----
#define C_BG       0x0a0b10
#define C_TRACK    0x1c1f29
#define C_BTN      0x15171f
#define C_BTN_MUTE 0x2b1417
#define C_RED      0xe24b4a
#define C_LIGHT    0xcfd2da
static uint32_t accent = 0x5865F2;  // mutavel via COLOR

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
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
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
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// ---- Serial helper (nunca bloqueia) ----
void txln(const char *s) {
  if (Serial && Serial.availableForWrite() > (int)strlen(s) + 2) Serial.println(s);
}

// ---- Widgets globais ----
static lv_obj_t *slider, *volLabel, *micBtn, *foneBtn, *micImg, *foneLbl;
static bool suppressEv = false;  // true quando o PC altera (nao gerar EV)

// ---- Eventos do usuario -> PC ----
static void slider_event_cb(lv_event_t *e) {
  int v = lv_slider_get_value(slider);
  lv_label_set_text_fmt(volLabel, "%d", v);
  if (suppressEv) return;
  static uint32_t lastTx = 0;
  static int lastV = -1;
  bool released = (lv_event_get_code(e) == LV_EVENT_RELEASED);
  uint32_t now = millis();
  if (v != lastV && (released || now - lastTx > 80)) {
    char b[16]; snprintf(b, sizeof(b), "EV VOL %d", v);
    txln(b); lastTx = now; lastV = v;
  }
}

static void mic_event_cb(lv_event_t *e) {
  bool muted = lv_obj_has_state(micBtn, LV_STATE_CHECKED);
  lv_img_set_src(micImg, muted ? &img_mic_muted : &img_mic);
  if (!suppressEv) { char b[20]; snprintf(b, sizeof(b), "EV BTN mic %d", muted); txln(b); }
}

static void fone_event_cb(lv_event_t *e) {
  bool muted = lv_obj_has_state(foneBtn, LV_STATE_CHECKED);
  lv_label_set_text(foneLbl, muted ? LV_SYMBOL_MUTE : LV_SYMBOL_VOLUME_MAX);
  lv_obj_set_style_text_color(foneLbl, lv_color_hex(muted ? C_RED : C_LIGHT), 0);
  if (!suppressEv) { char b[20]; snprintf(b, sizeof(b), "EV BTN deaf %d", muted); txln(b); }
}

static void style_btn(lv_obj_t *btn, int x) {
  lv_obj_set_size(btn, 64, 64);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, x, -12);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_style_radius(btn, 16, 0);
  lv_obj_set_style_bg_color(btn, lv_color_hex(C_BTN), 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_set_style_pad_all(btn, 0, 0);
  lv_obj_set_style_bg_color(btn, lv_color_hex(C_BTN_MUTE), LV_STATE_CHECKED);
  lv_obj_set_style_border_width(btn, 1, LV_STATE_CHECKED);
  lv_obj_set_style_border_color(btn, lv_color_hex(C_RED), LV_STATE_CHECKED);
}

void build_ui() {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(C_BG), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  lv_obj_t *logo = lv_obj_create(scr);
  lv_obj_set_size(logo, 132, 132);
  lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 8);
  lv_obj_set_style_radius(logo, 30, 0);
  lv_obj_set_style_clip_corner(logo, true, 0);
  lv_obj_set_style_bg_color(logo, lv_color_hex(accent), 0);
  lv_obj_set_style_border_width(logo, 0, 0);
  lv_obj_set_style_pad_all(logo, 0, 0);
  lv_obj_clear_flag(logo, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *logoImg = lv_img_create(logo);
  lv_img_set_src(logoImg, &img_discord);
  lv_obj_center(logoImg);

  slider = lv_slider_create(scr);
  lv_obj_set_size(slider, 140, 24);
  lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 152);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, 72, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(slider, lv_color_hex(C_TRACK), LV_PART_MAIN);
  lv_obj_set_style_radius(slider, 12, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_hex(accent), LV_PART_INDICATOR);
  lv_obj_set_style_radius(slider, 12, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, lv_color_hex(0xffffff), LV_PART_KNOB);
  lv_obj_set_style_pad_all(slider, 4, LV_PART_KNOB);
  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_RELEASED, NULL);

  volLabel = lv_label_create(scr);
  lv_label_set_text(volLabel, "72");
  lv_obj_set_style_text_color(volLabel, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(volLabel, &lv_font_montserrat_40, 0);
  lv_obj_align(volLabel, LV_ALIGN_TOP_MID, 0, 184);

  micBtn = lv_btn_create(scr);
  style_btn(micBtn, -36);
  micImg = lv_img_create(micBtn);
  lv_img_set_src(micImg, &img_mic);
  lv_obj_center(micImg);
  lv_obj_add_event_cb(micBtn, mic_event_cb, LV_EVENT_CLICKED, NULL);

  foneBtn = lv_btn_create(scr);
  style_btn(foneBtn, 36);
  foneLbl = lv_label_create(foneBtn);
  lv_label_set_text(foneLbl, LV_SYMBOL_VOLUME_MAX);
  lv_obj_set_style_text_color(foneLbl, lv_color_hex(C_LIGHT), 0);
  lv_obj_set_style_text_font(foneLbl, &lv_font_montserrat_20, 0);
  lv_obj_center(foneLbl);
  lv_obj_add_event_cb(foneBtn, fone_event_cb, LV_EVENT_CLICKED, NULL);
}

// ---- Aplica comandos do PC (sem gerar EV) ----
void setStateBtn(lv_obj_t *btn, bool on) {
  suppressEv = true;
  if (on) lv_obj_add_state(btn, LV_STATE_CHECKED);
  else lv_obj_clear_state(btn, LV_STATE_CHECKED);
  lv_event_send(btn, LV_EVENT_CLICKED, NULL);  // atualiza icone/cor
  suppressEv = false;
}

void applyColor(uint32_t rgb) {
  accent = rgb;
  lv_obj_set_style_bg_color(slider, lv_color_hex(accent), LV_PART_INDICATOR);
}

// ---- Parser de comandos serial ----
static char lineBuf[96];
static uint8_t lineLen = 0;

void handleCommand(char *s) {
  if (!strcmp(s, "PING")) { txln("PONG"); return; }

  if (!strncmp(s, "VOL ", 4)) {
    int v = constrain(atoi(s + 4), 0, 100);
    suppressEv = true;
    lv_slider_set_value(slider, v, LV_ANIM_OFF);
    lv_label_set_text_fmt(volLabel, "%d", v);
    suppressEv = false;
    return;
  }
  if (!strncmp(s, "STATE ", 6)) {
    char which[8]; int on = 0;
    if (sscanf(s + 6, "%7s %d", which, &on) == 2) {
      if (!strcmp(which, "mic")) setStateBtn(micBtn, on);
      else if (!strcmp(which, "deaf")) setStateBtn(foneBtn, on);
    }
    return;
  }
  if (!strncmp(s, "COLOR ", 6)) {
    uint32_t rgb = (uint32_t)strtol(s + 6, NULL, 16);
    applyColor(rgb);
    return;
  }
  // comandos desconhecidos: ignora
}

void pollSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (lineLen > 0) { lineBuf[lineLen] = 0; handleCommand(lineBuf); lineLen = 0; }
    } else if (lineLen < sizeof(lineBuf) - 1) {
      lineBuf[lineLen++] = c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

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

  build_ui();
  txln("HELLO modue " FW_VERSION);
}

void loop() {
  pollSerial();
  lv_timer_handler();
  delay(5);
}

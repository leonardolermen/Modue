// ============================================================
//  Mixer Modular Magnético — Tela Discord (LVGL)
//  Placa: SpotPear/Waveshare ESP32-C6-Touch-LCD-1.47
//  Display JD9853 172x320 | Touch AXS5106L
//
//  UI: marca Discord + fader vertical touch + botoes mute mic/fone.
//  O slider e os botoes mandam eventos no Serial (VOL / MUTE_*),
//  prontos pra ligar no app do PC depois.
//
//  Libs: GFX Library for Arduino, lvgl 8.4, esp_lcd_touch_axs5106l
//  (lv_conf.h ja instalado em ~/Documents/Arduino/libraries/)
//  Board: ESP32C6 Dev Module | USB CDC On Boot: Enabled
// ============================================================

#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include "esp_lcd_touch_axs5106l.h"

// ---- Display JD9853 ----
#define GFX_BL 23
Arduino_DataBus *bus = new Arduino_HWSPI(15 /* DC */, 14 /* CS */, 1 /* SCK */, 2 /* MOSI */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 22 /* RST */, 0, false, 172, 320, 34, 0, 34, 0);

// ---- Touch AXS5106L ----
#define TP_SDA 18
#define TP_SCL 19
#define TP_RST 20
#define TP_INT 21

// ---- Cores ----
#define C_BG       0x0a0b10
#define C_BLURPLE  0x5865F2
#define C_TRACK    0x1c1f29
#define C_BTN      0x15171f
#define C_BTN_MUTE 0x2b1417
#define C_RED      0xe24b4a
#define C_LIGHT    0xcfd2da

// ---- Init proprio do JD9853 (demo oficial Waveshare) ----
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

// ---- UI ----
static lv_obj_t *volLabel;

static void slider_event_cb(lv_event_t *e) {
  lv_obj_t *s = lv_event_get_target(e);
  int v = lv_slider_get_value(s);
  lv_label_set_text_fmt(volLabel, "%d", v);
  Serial.printf("VOL %d\n", v);
}

static void btn_event_cb(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target(e);
  const char *name = (const char *)lv_event_get_user_data(e);
  bool muted = lv_obj_has_state(btn, LV_STATE_CHECKED);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  bool isMic = (strcmp(name, "mic") == 0);
  if (isMic) {
    lv_label_set_text(label, muted ? LV_SYMBOL_MUTE "\nMic" : LV_SYMBOL_AUDIO "\nMic");
    Serial.printf("MUTE_MIC %s\n", muted ? "on" : "off");
  } else {
    lv_label_set_text(label, muted ? LV_SYMBOL_MUTE "\nFone" : LV_SYMBOL_VOLUME_MAX "\nFone");
    Serial.printf("MUTE_FONE %s\n", muted ? "on" : "off");
  }
  lv_obj_set_style_text_color(label, lv_color_hex(muted ? C_RED : C_LIGHT), 0);
}

static lv_obj_t *make_mute_btn(lv_obj_t *parent, const char *icon_txt, const char *id, int x) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 58, 54);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, x, -14);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_style_radius(btn, 15, 0);
  lv_obj_set_style_bg_color(btn, lv_color_hex(C_BTN), 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  // estado mutado (checked): fundo vermelho-escuro + borda vermelha
  lv_obj_set_style_bg_color(btn, lv_color_hex(C_BTN_MUTE), LV_STATE_CHECKED);
  lv_obj_set_style_border_width(btn, 1, LV_STATE_CHECKED);
  lv_obj_set_style_border_color(btn, lv_color_hex(C_RED), LV_STATE_CHECKED);

  lv_obj_t *lbl = lv_label_create(btn);
  lv_label_set_text(lbl, icon_txt);
  lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lbl, lv_color_hex(C_LIGHT), 0);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
  lv_obj_center(lbl);

  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void *)id);
  return btn;
}

void build_discord_ui() {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(C_BG), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  // Marca: quadrado blurple GRANDE + nome
  // (quando voce mandar a logo do Discord, troco esse quadrado pela imagem real)
  lv_obj_t *logo = lv_obj_create(scr);
  lv_obj_set_size(logo, 88, 88);
  lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_set_style_radius(logo, 22, 0);
  lv_obj_set_style_bg_color(logo, lv_color_hex(C_BLURPLE), 0);
  lv_obj_set_style_border_width(logo, 0, 0);
  lv_obj_clear_flag(logo, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *logoTxt = lv_label_create(logo);
  lv_label_set_text(logoTxt, LV_SYMBOL_CALL);  // placeholder ate a logo real
  lv_obj_set_style_text_color(logoTxt, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(logoTxt, &lv_font_montserrat_40, 0);
  lv_obj_center(logoTxt);

  lv_obj_t *name = lv_label_create(scr);
  lv_label_set_text(name, "Discord");
  lv_obj_set_style_text_color(name, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
  lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 116);

  // Fader HORIZONTAL (slider nativo touch)
  lv_obj_t *slider = lv_slider_create(scr);
  lv_obj_set_size(slider, 140, 26);
  lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 150);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, 72, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(slider, lv_color_hex(C_TRACK), LV_PART_MAIN);
  lv_obj_set_style_radius(slider, 13, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_hex(C_BLURPLE), LV_PART_INDICATOR);
  lv_obj_set_style_radius(slider, 13, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, lv_color_hex(0xffffff), LV_PART_KNOB);
  lv_obj_set_style_pad_all(slider, 4, LV_PART_KNOB);
  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // Numero do volume (grandao)
  volLabel = lv_label_create(scr);
  lv_label_set_text(volLabel, "72");
  lv_obj_set_style_text_color(volLabel, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(volLabel, &lv_font_montserrat_40, 0);
  lv_obj_align(volLabel, LV_ALIGN_TOP_MID, 0, 188);

  // Botoes mute mic / fone
  make_mute_btn(scr, LV_SYMBOL_AUDIO "\nMic", "mic", -34);
  make_mute_btn(scr, LV_SYMBOL_VOLUME_MAX "\nFone", "fone", 34);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[Modue] Tela Discord (LVGL)");

  // Display
  if (!gfx->begin()) Serial.println("gfx->begin() FALHOU");
  lcd_reg_init();
  gfx->setRotation(0);
  gfx->fillScreen(RGB565_BLACK);
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  // Touch
  Wire.begin(TP_SDA, TP_SCL);
  bsp_touch_init(&Wire, TP_RST, TP_INT, gfx->getRotation(), gfx->width(), gfx->height());

  // LVGL
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

  build_discord_ui();
  Serial.println("UI pronta. Arraste o fader e toque nos botoes.");
}

void loop() {
  lv_timer_handler();
  delay(5);
}

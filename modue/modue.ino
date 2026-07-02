// ============================================================
//  Mixer Modular Magnético — Firmware dinâmico (controlado pelo PC)
//  Placa: SpotPear/Waveshare ESP32-C6-Touch-LCD-1.47 (JD9853 + AXS5106L)
//
//  Renderizador genérico com telas trocáveis (Discord / Spotify).
//  PC manda comandos (SCREEN/VOL/STATE/COLOR/LABEL/IMG); módulo devolve
//  eventos (EV VOL / EV BTN). Ver PROTOCOL.md.
//
//  Organização (abas .ino, concatenadas pelo Arduino):
//    modue.ino    -> includes, config, globais, setup(), loop()
//    board.ino    -> init JD9853 + glue do LVGL/touch
//    screens.ino  -> telas Discord/Spotify + callbacks de toque
//    protocol.ino -> serial: comandos do PC e recepção de imagem
//    fader.ino    -> leitura do fader físico (ADC) + calibração
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

// ---- Capa do album (recebida via comando IMG) ----
#define MAX_ART 40000  // 140*140*2 = 39200

// Tipos no topo (o Arduino gera protótipos antes das funções nas outras abas)
enum Screen { SCR_DISCORD, SCR_SPOTIFY };

// ============================================================
//  Globais (definidos aqui; visíveis em todas as abas .ino — é uma TU só)
//  static = linkagem interna, evita colisão de nome com as libs.
// ============================================================
static uint32_t accent = 0x5865F2;

// LVGL glue
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;
static lv_disp_drv_t disp_drv;

// Estado
static Screen current = SCR_DISCORD;
static bool suppressEv = false;

// Widgets
static lv_obj_t *slider, *volLabel;                                   // comuns
static lv_obj_t *micBtn, *micImg, *foneBtn, *foneLbl;                 // discord
static lv_obj_t *trackLabel, *prevBtn, *playBtn, *playLbl, *nextBtn, *artImg;  // spotify

// Recepcao de imagem (nao-bloqueante)
static uint8_t *artBuf = NULL;
static lv_img_dsc_t artDsc;
static int imgW = 0, imgH = 0;
static size_t imgRemaining = 0, imgPos = 0;
static uint32_t imgLast = 0;

// Parser de comandos
static char lineBuf[160];
static uint8_t lineLen = 0;

// ============================================================
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

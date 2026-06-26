// ============================================================
//  Mixer Modular Magnético — Teste 04: Fader TOUCH
//  Placa: SpotPear/Waveshare ESP32-C6-Touch-LCD-1.47
//  Display JD9853 172x320  |  Touch AXS5106L (I2C addr 0x63)
//
//  Arraste o dedo pra cima/baixo na barra: o volume acompanha.
//  Pinos display: SCK=1 MOSI=2 DC=15 CS=14 RST=22 BL=23
//  Pinos touch  : SDA=18 SCL=19 RST=20 INT=21
//
//  Lib: "GFX Library for Arduino" (Moon On Our Nation)
//  Board: ESP32C6 Dev Module | USB CDC On Boot: Enabled
// ============================================================

#include <Arduino_GFX_Library.h>
#include <Wire.h>

#define GFX_BL 23

// ---- Touch AXS5106L ----
#define TP_SDA 18
#define TP_SCL 19
#define TP_RST 20
#define TP_INT 21
#define TP_ADDR 0x63
#define TP_DATA_REG 0x01

Arduino_DataBus *bus = new Arduino_HWSPI(15 /* DC */, 14 /* CS */, 1 /* SCK */, 2 /* MOSI */);
Arduino_GFX *gfx = new Arduino_ST7789(
    bus, 22 /* RST */, 0 /* rotation */, false /* IPS */,
    172, 320, 34, 0, 34, 0);

// Para paisagem: troque o rotation acima pra 1 ou 3 e ajuste o layout.

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

// ---- Leitura do touch (polling, sem interrupcao) ----
// Retorna true se ha dedo na tela; preenche tx,ty em coords de tela.
bool lerTouch(int &tx, int &ty) {
  uint8_t d[14];
  Wire.beginTransmission(TP_ADDR);
  Wire.write(TP_DATA_REG);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom(TP_ADDR, (uint8_t)14) != 14) return false;
  for (int i = 0; i < 14; i++) d[i] = Wire.read();

  uint8_t n = d[1];               // numero de toques
  if (n == 0) return false;

  uint16_t rawX = ((uint16_t)(d[2] & 0x0F) << 8) | d[3];
  uint16_t rawY = ((uint16_t)(d[4] & 0x0F) << 8) | d[5];
  // rotation 0: x espelhado, y direto
  tx = 171 - rawX;
  ty = rawY;
  return true;
}

void resetTouch() {
  pinMode(TP_RST, OUTPUT);
  digitalWrite(TP_RST, LOW);  delay(200);
  digitalWrite(TP_RST, HIGH); delay(300);
  pinMode(TP_INT, INPUT_PULLUP);
}

// ---- Layout do fader ----
const int TRK_X = 46, TRK_W = 80;     // trilha
const int TRK_Y = 70, TRK_H = 200;    // topo e altura
#define COR_BARRA  0x07E0   // verde
#define COR_TRILHA 0x2104   // cinza bem escuro
#define COR_KNOB   0xFFFF   // branco

int volume = 50;
int volAnterior = -1;

void desenhaFader(int v) {
  // trilha
  gfx->fillRoundRect(TRK_X, TRK_Y, TRK_W, TRK_H, 10, COR_TRILHA);
  // preenchimento de baixo pra cima
  int fill = (TRK_H * v) / 100;
  gfx->fillRoundRect(TRK_X, TRK_Y + (TRK_H - fill), TRK_W, fill, 10, COR_BARRA);
  // "knob" (linha do nivel)
  int ky = TRK_Y + (TRK_H - fill);
  gfx->fillRoundRect(TRK_X - 6, ky - 4, TRK_W + 12, 8, 4, COR_KNOB);

  // numero
  gfx->fillRect(0, TRK_Y + TRK_H + 14, 172, 40, RGB565_BLACK);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setTextSize(4);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", v);
  int16_t x1, y1; uint16_t w, h;
  gfx->getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  gfx->setCursor((172 - w) / 2, TRK_Y + TRK_H + 18);
  gfx->print(buf);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[Modue] Teste 04 - Fader Touch");

  // Display
  if (!gfx->begin()) Serial.println("gfx->begin() FALHOU");
  lcd_reg_init();
  gfx->setRotation(0);
  gfx->fillScreen(RGB565_BLACK);
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  // Touch
  Wire.begin(TP_SDA, TP_SCL);
  resetTouch();

  // Titulo
  gfx->setTextColor(0x07FF);
  gfx->setTextSize(3);
  gfx->setCursor(20, 20);
  gfx->print("Discord");

  Serial.println("Pronto. Arraste o dedo na barra.");
}

void loop() {
  int tx, ty;
  if (lerTouch(tx, ty)) {
    // se o toque esta na faixa vertical do fader, define o volume
    if (ty >= TRK_Y - 10 && ty <= TRK_Y + TRK_H + 10) {
      int v = (int)((float)(TRK_Y + TRK_H - ty) / TRK_H * 100.0f + 0.5f);
      volume = constrain(v, 0, 100);
    }
  }

  if (volume != volAnterior) {
    desenhaFader(volume);
    volAnterior = volume;
    Serial.printf("Volume: %d%%\n", volume);
  }
  delay(15);
}

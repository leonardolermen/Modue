# Modue — Mixer Modular Magnético

Controlador físico modular para PC que controla individualmente o volume de aplicações (Discord, Spotify, Chrome, jogos…) através de módulos magnéticos, cada um com display touch e fader motorizado, comunicando via Bluetooth Low Energy.

> Inspirado em produtos como Monogram Creative Console e Intech Studio Grid, porém open-source, modular e a uma fração do custo.

## Hardware

- **MCU + Display:** ESP32-C6-Touch-LCD-1.47 (SpotPear/Waveshare) — display **JD9853** 172×320 + touch **AXS5106L**
- **Fader motorizado:** ALPS RS60N11M9A0
- **Driver de motor:** DRV8833 / TB6612FNG
- **Conexão entre módulos:** pogo pins magnéticos (energia) + BLE (dados)

## Pinagem (placa Touch — JD9853)

| Display | GPIO | | Touch | GPIO |
|---|---|---|---|---|
| SCK | 1 | | SDA | 18 |
| MOSI | 2 | | SCL | 19 |
| DC | 15 | | RST | 20 |
| CS | 14 | | INT | 21 |
| RST | 22 | | | |
| BL | 23 | | | |

> ⚠️ É a variante **Touch** (JD9853, não ST7789). O JD9853 precisa de sequência de init própria — ver `discordUI/`.

## Firmware (Arduino IDE)

| Sketch | Descrição |
|---|---|
| `displayHello/` | Teste do display JD9853 + fader touch (Arduino_GFX) |
| `discordUI/` | UI do módulo em **LVGL**: logo, fader e botões mute (mic/fone) |

**Libs:** GFX Library for Arduino, LVGL 8.4, esp_lcd_touch_axs5106l
**Board:** ESP32C6 Dev Module · USB CDC On Boot: Enabled

## Roadmap

- [x] Display JD9853 funcionando
- [x] Touch AXS5106L
- [x] UI Discord em LVGL (fader + botões mute)
- [ ] Ícones/logo reais embutidos
- [ ] Link PC ↔ módulo (Serial → BLE)
- [ ] App PC (Node.js/Electron) — volume via WASAPI, mute Discord
- [ ] Fader motorizado + sincronização com o volume do Windows

Documentação completa em [`claude.md`](claude.md).

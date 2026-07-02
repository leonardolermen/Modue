# Setup do ambiente (máquina nova)

Guia pra colocar o projeto pra rodar do zero. Três frentes: **firmware** (Arduino),
**app de PC** (Node) e **case** (OpenSCAD).

---

## 1. Firmware (Arduino IDE)

### Board
- Instale o core **esp32** (Espressif) pelo Boards Manager — versão **≥ 3.0** (testado na 3.3.10).
- Selecione **ESP32C6 Dev Module**.
- Em *Tools*: **USB CDC On Boot: Enabled**, Flash Size **8MB**.

### Bibliotecas
| Lib | Como instalar | Versão |
|---|---|---|
| **GFX Library for Arduino** (moononournation) | Library Manager | 1.6.x |
| **LVGL** | ⚠️ **fixar na 8.4.0** — **NÃO** usar 9.x (API incompatível, quebra tudo) | 8.4.0 |
| **esp_lcd_touch_axs5106l** | copiar do demo da Waveshare (não está no Library Manager) | — |

O jeito mais garantido de ter a LVGL 8.4 + o driver de touch é pegar do **demo oficial**:

```
https://files.waveshare.com/wiki/ESP32-C6-Touch-LCD-1.47/ESP32-C6-Touch-LCD-1.47-Demo.zip
```

Dentro dele, copie `Arduino/libraries/lvgl` e `Arduino/libraries/esp_lcd_touch_axs5106l`
para a pasta de bibliotecas do Arduino (`~/Documents/Arduino/libraries/` no Mac,
`Documentos\Arduino\libraries\` no Windows).

> Se o Library Manager sugerir atualizar a LVGL pra 9.x, **ignore** — quebra a compilação.

### lv_conf.h (crítico)
Copie o `lv_conf.h` do exemplo do demo (`Arduino/examples/04_lvgl_arduino_v8/lv_conf.h`)
para a **raiz** da pasta de bibliotecas (`libraries/lv_conf.h`), e ajuste:

- `LV_COLOR_16_SWAP` → **0**  (senão as cores saem trocadas)
- `LV_TICK_CUSTOM` → **1**  (usa `millis()` do Arduino)
- Habilite as fontes: `LV_FONT_MONTSERRAT_20`, `_28`, `_40` → **1**

### Pinagem (já no código, mas pra referência)
Placa **ESP32-C6-Touch-LCD-1.47** (JD9853 + AXS5106L):
- Display (HW SPI): SCK=1, MOSI=2, DC=15, CS=14, RST=22, BL=23
- Touch (I²C): SDA=18, SCL=19, RST=20, INT=21
- Fader (pot 10k): wiper→GPIO3, extremos no 3V3/GND

Abra `modue/modue.ino` (as abas `board/screens/protocol/fader` aparecem sozinhas) e grave.

---

## 2. App de PC (Node)

```bash
cd app
npm install
```

- **Spotify:** o `config.json` já tem o `clientId` (público, ok no git). Registre o
  Redirect URI `http://127.0.0.1:8888/callback` no [Spotify Dashboard](https://developer.spotify.com/dashboard)
  se ainda não estiver. Os tokens (`.spotify-tokens.json`) **não** vão pro git — na
  primeira execução ele abre o navegador pra você autorizar de novo.
- Rodar o Spotify: `node src/spotify-run.js`
- **Windows:** pro volume por-app do Discord, baixe o `svcl.exe` (NirSoft) — ver `app/README.md`.

---

## 3. Case (OpenSCAD)

- Instale o **OpenSCAD** (https://openscad.org ou `brew install --cask openscad`).
- Abra `case/modulo_case.scad`. Variáveis de medida no topo do arquivo.

---

## 4. Git (empurrar de uma máquina nova)

Clone com a URL padrão (usa a chave SSH da conta pessoal da própria máquina):

```bash
git clone git@github.com:leonardolermen/Modue.git
```

> Obs: no Mac de trabalho foi usado um alias `github-personal` no `~/.ssh/config`
> porque a chave padrão dali era da conta do trabalho. Numa máquina pessoal onde a
> conta do GitHub já é a `leonardolermen`, não precisa de alias — a URL padrão funciona.
> (Ou use HTTPS + token.)

---

## Estrutura do repo

| Pasta | O que é |
|---|---|
| `modue/` | firmware (abas .ino) — firmware principal |
| `app/` | app de PC (Node): serial + Spotify + imagens |
| `case/` | modelo 3D do case (OpenSCAD) |
| `docs/` | arquitetura + diagrama |
| `discordUI/`, `displayHello/` | sketches antigos (histórico) |
| `PROTOCOL.md` | contrato serial PC↔módulo |

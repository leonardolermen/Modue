# Mixer Modular Magnético

Controlador físico modular para PC que controla individualmente o volume de aplicações Windows (Discord, Spotify, Chrome, Jogos, etc.) através de faders físicos com displays touch dedicados.

---

## Visão Geral

Inspirado em produtos como Monogram Creative Console, Intech Studio Grid e Modue, porém open-source, modular e a uma fração do custo. Cada módulo é independente — tem seu próprio MCU, display touch e fader motorizado — e se comunica com o PC via Bluetooth Low Energy.

---

## Hardware

### Módulo (1 por aplicativo)

| Componente | Modelo | Função |
|---|---|---|
| MCU + Display | ESP32-C6-Touch-LCD-1.47 | Cérebro do módulo + display 172×320 touch |
| Fader motorizado | ALPS RS60N11M9A0 | Controle de volume físico com feedback automático |
| Driver de motor | DRV8833 ou TB6612FNG | Controla o motor do fader (H-bridge) |
| Conector | Pogo pins magnéticos 4P (2.54mm) | VCC + GND (energia vinda da base) |
| Fixação | Ímãs neodímio N35 6×3mm | Encaixe magnético na base |

### Base Principal

| Componente | Modelo | Função |
|---|---|---|
| Fonte | USB-C | Alimentação geral |
| Distribuição | Rails de pogo pins | Distribui 5V para todos os módulos encaixados |

> **Nota:** Cada módulo tem seu próprio ESP32-C6 — a base serve apenas como fonte de energia. A comunicação com o PC é via **Bluetooth**, não por fio.

---

## Arquitetura

```
PC Windows (Node.js)
        │
        │ BLE (Bluetooth Low Energy)
        │
   ┌────┴────┐
   │ Módulo 1│ ESP32-C6 + Display Touch
   │ Discord │ Fader ALPS motorizado
   └─────────┘
   ┌─────────┐
   │ Módulo 2│ ESP32-C6 + Display Touch
   │ Spotify │ Fader ALPS motorizado
   └─────────┘
   ┌─────────┐
   │Módulo N │ ESP32-C6 + Display Touch
   │  ...    │ Fader ALPS motorizado
   └─────────┘
        │
        │ Pogo Pins (VCC + GND)
        │
   ┌────┴────┐
   │  BASE   │ USB-C → distribuição de energia
   └─────────┘
```

### Fluxo de dados

```
Fader movido pelo usuário
  → ADC lê posição (GPIO0)
  → ESP32-C6 envia BLE notify: {"discord": 75}
  → Node.js recebe
  → WASAPI aplica volume no Windows

Volume mudado pelo Windows (outro app, tecla de mídia)
  → Node.js detecta via WASAPI
  → Node.js envia BLE write: SET 75
  → ESP32-C6 aciona motor do fader
  → Fader se move fisicamente até 75%
```

---

## Pinagem — ESP32-C6-Touch-LCD-1.47 (SpotPear/Waveshare)

> ⚠️ **Importante:** esta é a placa **TOUCH**, com controlador **JD9853** (NÃO ST7789)
> e touch **AXS5106L** (NÃO CST816T). A pinagem é **diferente** da versão não-touch.
> O JD9853 precisa de sequência de init própria (ver `displayHello/displayHello.ino`).
> Pinagem confirmada no demo oficial Waveshare (`01_gfx_helloworld.ino`).

### Display JD9853 (fixo na placa, 4-wire SPI por hardware)

| Sinal | GPIO |
|---|---|
| SCK | 1 |
| MOSI (SDA) | 2 |
| DC | 15 |
| CS | 14 |
| RST | 22 |
| Backlight (BL) | 23 |

### Touch AXS5106L (fixo na placa, I²C)

| Sinal | GPIO |
|---|---|
| SDA | 18 |
| SCL | 19 |
| RST | 20 |
| INT | 21 |

### Fader e Motor (você cabia)

> ⚠️ GPIO **18/19/20/21 estão OCUPADOS pelo touch** — não usar pro motor!
> Escolher GPIOs livres (ex.: 3, 4, 5, 6, 7) para o driver do motor.

| Sinal | GPIO | Observação |
|---|---|---|
| Fader wiper (feedback) | GPIO0 | ADC1 — lê posição atual (confirmar livre) |
| Motor IN1 | GPIO4 | Direção do motor (sugestão — pino livre) |
| Motor IN2 | GPIO5 | Direção do motor (sugestão — pino livre) |
| Motor PWM | GPIO6 | Velocidade via PWM (sugestão — pino livre) |

---

## Case 3D

Dimensões externas: **170 × 40 × 38 mm**

### Peças (imprimir em PLA/PETG preto fosco)

| Arquivo | Descrição | Qtd |
|---|---|---|
| `tray_v3.stl` | Corpo principal (bandeja) — aloja eletrônica, display, conector | 1 |
| `lid_v3.stl` | Tampa com fenda do fader | 1 |
| `knob_v3.stl` | Puxador do fader | 1 |

### Cavidades internas

- **Fader motorizado ALPS**: 110 × 21 × 24mm
- **Janela do display**: 36 × 28mm (rebaixo 41 × 33mm)
- **Pogo pins**: 4 furos Ø2.2mm na face direita
- **Ímãs**: bolsos 6.2 × 3.2mm nos 4 cantos, faces direita e esquerda

### Parâmetros ajustáveis (arquivo `.scad`)

Abrir `modulo_case_v3.scad` no **OpenSCAD** (gratuito) e editar as variáveis no topo:

```scad
fader_cav_l  = 110;   // comprimento do fader (medir o componente real)
fader_cav_w  = 21;    // largura
fader_cav_h  = 24;    // altura
display_window_w = 36; // largura da janela do display
display_window_h = 28; // altura da janela
```

---

## Software PC (Node.js)

### Dependências

```bash
npm install @abandonware/noble   # BLE central
npm install node-ffi-napi        # WASAPI (Windows Audio)
npm install electron             # Interface gráfica (opcional)
```

### Protocolo BLE

Cada módulo expõe um **GATT Service** com uma **Characteristic** de volume:

| Direção | Formato | Exemplo |
|---|---|---|
| Módulo → PC (Notify) | JSON string | `{"discord":75}` |
| PC → Módulo (Write) | String | `SET 75` |

### Estrutura básica do backend

```js
// index.js
const noble = require('@abandonware/noble');

noble.on('discover', (peripheral) => {
  // conecta em módulos com nome "MixMod-*"
  if (peripheral.advertisement.localName?.startsWith('MixMod')) {
    peripheral.connect(() => {
      peripheral.discoverAllServicesAndCharacteristics((err, services, chars) => {
        chars.forEach(c => {
          c.on('data', (data) => {
            const json = JSON.parse(data.toString());
            // aplica volume via WASAPI
            setWindowsVolume(json);
          });
          c.subscribe(); // habilita notify
        });
      });
    });
  }
});

noble.startScanning();
```

---

## Firmware (Arduino IDE)

### Configuração do Arduino IDE

| Setting | Valor |
|---|---|
| Board | ESP32C6 Dev Module |
| USB CDC On Boot | **Enabled** |
| Flash Size | 8MB |
| Partition Scheme | 8M with spiffs |

### Bibliotecas necessárias

| Biblioteca | Instalação |
|---|---|
| Arduino_GFX Library (by Moon On Our Nation) | Library Manager |
| ArduinoJson | Library Manager |
| NimBLE-Arduino | Library Manager (BLE mais leve que o padrão) |

### Arquivos de firmware

| Arquivo | Descrição |
|---|---|
| `teste1_hello_display.ino` | Teste básico do display — Hello World + animação de volume |
| `sketch_v3.ino` | Firmware completo (Wokwi) — display + fader + motor simulado + serial |

---

## Roadmap

### Fase 1 — MVP ✅ em andamento
- [x] Conceito e arquitetura definidos
- [x] Case 3D modelado (OpenSCAD)
- [x] Firmware base do display (Arduino_GFX)
- [ ] Teste do display físico na placa
- [ ] Leitura do fader via ADC
- [ ] Driver de motor (H-bridge)

### Fase 2 — BLE
- [ ] GATT Server no ESP32-C6
- [ ] Backend Node.js com `noble`
- [ ] Integração WASAPI (controle de volume Windows)
- [ ] Sincronização motor ↔ volume do Windows

### Fase 3 — Software completo
- [ ] Auto-detecção de aplicativos ativos
- [ ] Perfis (FPS, Trabalho, Streaming)
- [ ] Ícones dos apps no display
- [ ] Interface Electron (tray app)

### Fase 4 — Polimento
- [ ] PCB customizada (KiCad)
- [ ] Case v2 ajustado ao hardware real medido
- [ ] Documentação open-source no GitHub
- [ ] BOM completo com links de compra

---

## BOM (Bill of Materials)

### Comprado

| Componente | Qtd | Onde | Valor aprox. |
|---|---|---|---|
| ESP32-C6-Touch-LCD-1.47 | 2 | AliExpress | R$ ~100 |
| ALPS RS60N11M9A0 (fader motorizado) | 1 | AliExpress | R$ 115 |
| Pogo pins magnéticos 4P | 3 pares | AliExpress | R$ 65 |
| Display 1.69" ST7789V (painel nu) | 2 | AliExpress | R$ 87 |

### Faltando

| Componente | Qtd | Estimativa |
|---|---|---|
| DRV8833 / TB6612FNG (motor driver) | 1 por módulo | ~R$ 15 |
| Ímãs neodímio N35 6×3mm (pack 50) | 1 pack | ~R$ 15 |
| PLA/PETG preto fosco (filamento) | ~50g por módulo | ~R$ 10 |

---

## Referências

- [Waveshare ESP32-C6-Touch-LCD-1.47 Wiki](https://www.waveshare.com/wiki/ESP32-C6-Touch-LCD-1.47)
- [Arduino_GFX Library](https://github.com/moononournation/Arduino_GFX)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
- [ALPS RS60N11M9A0 Datasheet](https://tech.alpsalpine.com)
- [noble (Node.js BLE)](https://github.com/abandonware/noble)

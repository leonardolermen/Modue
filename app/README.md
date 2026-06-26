# Modue PC bridge

Ponte entre o módulo físico e o PC: lê os eventos do fader/botões pela porta serial e aplica no Discord.

```
modulo (ESP32) --USB serial--> este app --> Discord (mute / deafen / volume)
```

Eventos que o firmware envia: `VOL <0-100>`, `MUTE_MIC on|off`, `MUTE_FONE on|off`.

## Instalar

```bash
cd pc
npm install
```

## Configurar o Discord (atalhos globais)

O app simula os **atalhos globais** do Discord. Configure em
**Discord → Configurações → Atalhos de Teclado** e adicione:

| Ação | Atalho (macOS) | Atalho (Windows) |
|---|---|---|
| Alternar mudo (mute) | `Ctrl+Alt+Cmd+M` | `Ctrl+Alt+Win+M` |
| Alternar surdo (deafen) | `Ctrl+Alt+Cmd+D` | `Ctrl+Alt+Win+D` |

> Os atalhos precisam bater com o `config.json` (`keybinds`). Pode trocar à vontade,
> só mantenha os dois lados iguais.

## Rodar

```bash
npm start
```

A porta serial é **auto-detectada** (`usbmodem` no Mac, `COM` no Windows). Para fixar,
edite `serialPort` no `config.json`.

> ⚠️ Feche o **Serial Monitor do Arduino** antes — a porta não pode ser aberta por dois programas.

## Notas por sistema

### macOS
- **Permissão necessária:** ao enviar o primeiro atalho, o macOS pede acesso de
  **Acessibilidade**. Vá em *Ajustes → Privacidade e Segurança → Acessibilidade* e
  habilite o **Terminal** (ou o app que rodou o `npm start`).
- **Volume:** o macOS não tem volume por-app nativo, então o fader controla o
  **volume de saída do sistema**. (Por-app exigiria um driver de áudio virtual.)

### Windows
- **Volume por-app:** usa o `svcl.exe` (NirSoft *SoundVolumeView*). Baixe em
  https://www.nirsoft.net/utils/sound_volume_view.html e coloque o `svcl.exe` na
  pasta `pc/` (ou ajuste `windows.svclPath` no `config.json`).
- O nome do processo do Discord está em `discordProcess` (`Discord.exe`).

## config.json

| Campo | Descrição |
|---|---|
| `serialPort` | `"auto"` ou caminho fixo (`/dev/cu.usbmodem1101`, `COM5`) |
| `baud` | 115200 (igual ao firmware) |
| `discordProcess` | nome do processo p/ volume no Windows |
| `keybinds.mute` / `.deafen` | combinação de teclas (precisa bater com o Discord) |
| `windows.svclPath` | caminho do `svcl.exe` |

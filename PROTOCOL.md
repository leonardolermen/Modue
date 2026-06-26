# Protocolo Modue (PC ↔ módulo)

Comunicação por **USB serial** a **115200 baud**, linhas terminadas em `\n` (ASCII).
Bidirecional: o PC manda comandos, o módulo manda eventos.

## PC → Módulo (comandos)

| Comando | Efeito |
|---|---|
| `PING` | Módulo responde `PONG` (handshake/keepalive) |
| `SCREEN <discord\|spotify>` | Troca a tela/layout do módulo |
| `VOL <0-100>` | Move o fader pra essa posição (sem gerar evento de volta) |
| `STATE <mic\|deaf\|play> <0\|1>` | Define o estado de um botão (1 = ativo/mutado/tocando) |
| `LABEL <texto>` | Texto principal (nome da faixa na tela Spotify) |
| `COLOR <RRGGBB>` | Cor de destaque (fader e realces) |
| `IMG <slot> <w> <h>`<br>+ `w*h*2` bytes RGB565 | Envia imagem pro display (ex.: `IMG art 140 140` + bytes). Slot `art` = capa do álbum. RGB565 little-endian, opaco. |

## Módulo → PC (eventos)

| Evento | Quando |
|---|---|
| `HELLO modue <versao>` | No boot |
| `PONG` | Resposta ao `PING` |
| `EV VOL <0-100>` | Usuário moveu o fader |
| `EV BTN <mic\|deaf> <0\|1>` | Tela Discord: tocou no botão (estado novo) |
| `EV BTN play <0\|1>` | Tela Spotify: play/pause (1 = tocando) |
| `EV BTN <prev\|next> 1` | Tela Spotify: faixa anterior / próxima (momentâneo) |

## Regras

- Comandos/eventos desconhecidos são ignorados (compatibilidade pra frente).
- O PC é a fonte da verdade do estado: ao receber `EV BTN`, aplica no serviço
  (Discord etc.) e devolve `STATE` confirmando — assim o módulo reflete o estado real.
- `VOL`/`STATE` vindos do PC **não** geram `EV` de volta (evita eco/loop).

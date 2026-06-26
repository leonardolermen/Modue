# Protocolo Modue (PC ↔ módulo)

Comunicação por **USB serial** a **115200 baud**, linhas terminadas em `\n` (ASCII).
Bidirecional: o PC manda comandos, o módulo manda eventos.

## PC → Módulo (comandos)

| Comando | Efeito |
|---|---|
| `PING` | Módulo responde `PONG` (handshake/keepalive) |
| `VOL <0-100>` | Move o fader pra essa posição (sem gerar evento de volta) |
| `STATE <mic\|deaf> <0\|1>` | Define o estado do botão (1 = ativo/mutado) |
| `COLOR <RRGGBB>` | Cor de destaque (fader e realces) |

### Fase B (imagens) — ainda não implementado
| Comando | Efeito |
|---|---|
| `LABEL <texto>` | Nome do app no display |
| `IMG <slot> <w> <h>` + `w*h*2` bytes RGB565 | Envia logo/capa pro display |

## Módulo → PC (eventos)

| Evento | Quando |
|---|---|
| `HELLO modue <versao>` | No boot |
| `PONG` | Resposta ao `PING` |
| `EV VOL <0-100>` | Usuário moveu o fader |
| `EV BTN <mic\|deaf> <0\|1>` | Usuário tocou no botão (estado novo) |

## Regras

- Comandos/eventos desconhecidos são ignorados (compatibilidade pra frente).
- O PC é a fonte da verdade do estado: ao receber `EV BTN`, aplica no serviço
  (Discord etc.) e devolve `STATE` confirmando — assim o módulo reflete o estado real.
- `VOL`/`STATE` vindos do PC **não** geram `EV` de volta (evita eco/loop).

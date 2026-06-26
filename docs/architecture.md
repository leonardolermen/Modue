# Arquitetura

![Arquitetura do Modue](architecture.svg)

O firmware é um **renderizador genérico controlado pelo PC**. Em vez de telas fixas,
o módulo recebe comandos (volume, estado dos botões, cor, imagens) e devolve eventos
(toques no fader e nos botões). Quem decide o que cada módulo representa — Discord,
Spotify, etc. — é o app de PC.

## Camadas

- **Hardware (módulos):** ESP32-C6 + display touch JD9853. Cada módulo é um app.
  Desenha a UI em LVGL e emite eventos de toque.
- **App de PC (Electron):** núcleo do sistema.
  - *Gerenciador:* detecta módulos, guarda perfis e qual app está em cada um.
  - *Protocolo + imagens:* fala o [protocolo serial](../PROTOCOL.md) e envia logos/capas.
  - *Integrações:* conecta nos serviços e mantém o estado em tempo real.
- **Serviços externos:** Discord (RPC), Spotify (API) e o volume do sistema operacional.

## Comunicação

PC e módulo conversam por **USB serial, bidirecional** (futuramente BLE). O contrato
completo de comandos e eventos está em [`PROTOCOL.md`](../PROTOCOL.md).

## Fases de implementação

| Fase | Entrega | Status |
|---|---|---|
| A | Protocolo bidirecional + firmware dinâmico | ✅ |
| B | Envio de imagem pro display | ⏳ |
| C | Shell do app (Electron) | ⏳ |
| D | Discord RPC nativo (+ estado de volta) | ⏳ |
| E | Spotify (play/skip + capa) | ⏳ |
| F | Multi-módulo + perfis | ⏳ |

// Liga o módulo ao Spotify (real). Uso: cd app && node src/spotify-run.js
import { SerialPort } from "serialport";
import { ReadlineParser } from "@serialport/parser-readline";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import { Spotify } from "./integrations/spotify.js";
import { imageToRgb565 } from "./image.js";

const ART_SIZE = 140; // casa com a area da capa no firmware

// rede de seguranca: nunca derruba o app por um erro solto de API
process.on("unhandledRejection", (e) => console.error("  ! (ignorado)", e.message));

const __dir = dirname(fileURLToPath(import.meta.url));
const cfg = JSON.parse(readFileSync(join(__dir, "..", "config.json"), "utf8"));

if (!cfg.spotify?.clientId) {
  console.error("Falta o clientId do Spotify em config.json (campo spotify.clientId).");
  process.exit(1);
}

const sp = new Spotify(cfg.spotify);

async function findPort() {
  if (cfg.serialPort && cfg.serialPort !== "auto") return cfg.serialPort;
  const ports = await SerialPort.list();
  const hit = ports.find((p) => /usbmodem|usbserial|wchusbserial|COM\d+/i.test(p.path));
  if (!hit) { console.error("Placa nao encontrada."); process.exit(1); }
  return process.platform === "darwin" ? hit.path.replace("/tty.", "/cu.") : hit.path;
}

let port;
const send = (cmd) => { if (port?.isOpen) port.write(cmd + "\n"); };

// ---- volume com coalescing (nao floodar a API) ----
let pendingVol = null, volTimer = null;
function queueVolume(v) {
  pendingVol = v;
  if (volTimer) return;
  volTimer = setTimeout(async () => {
    const val = pendingVol; pendingVol = null; volTimer = null;
    try { await sp.setVolume(val); } catch (e) { console.error("  ! volume:", e.message); }
  }, 150);
}

// ---- eventos do módulo -> Spotify ----
async function onEvent(line) {
  const s = line.trim();
  if (!s.startsWith("EV ")) {
    if (s) console.log("  <- modulo:", s); // mostra HELLO/PONG/DBG...
    return;
  }
  const [, kind, a, b] = s.split(/\s+/); // EV VOL 70  |  EV BTN play 1
  try {
    if (kind === "VOL") queueVolume(parseInt(a, 10));
    else if (kind === "BTN") {
      if (a === "play") { await (b === "1" ? sp.play() : sp.pause()); setTimeout(refresh, 400); }
      else if (a === "prev") { await sp.prev(); setTimeout(refresh, 400); }
      else if (a === "next") { await sp.next(); setTimeout(refresh, 400); }
      console.log(`  🎛️  ${a}${b ? " " + b : ""}`);
    }
  } catch (e) { console.error("  ! acao:", e.message); }
}

// ---- estado do Spotify -> módulo ----
let lastTrack = null;
async function refresh() {
  let st;
  try { st = await sp.getState(); } catch (e) { console.error("  ! getState:", e.message); return; }
  if (!st) return;
  if (st.volume != null) send(`VOL ${st.volume}`);
  send(`STATE play ${st.isPlaying ? 1 : 0}`);
  if (st.track && st.track !== lastTrack) {
    lastTrack = st.track;
    send(`LABEL ${st.track}`);
    console.log(`\n🎵 ${st.track}`);
    if (st.albumArt) sendArt(st.albumArt);
  }
}

// ---- baixa a capa, converte e manda pro display (comando IMG + bytes) ----
async function sendArt(url) {
  try {
    const { buf, w, h } = await imageToRgb565(url, ART_SIZE);
    send(`IMG art ${w} ${h}`);
    port.write(buf); // bytes RGB565 crus logo apos o header
    console.log(`   🖼️  capa enviada (${w}x${h}, ${buf.length} bytes)`);
  } catch (e) { console.error("  ! capa:", e.message); }
}

async function main() {
  console.log("Modue ↔ Spotify\n");
  await sp.ensureAuth();
  console.log("Autenticado no Spotify ✅");

  const path = await findPort();
  port = new SerialPort({ path, baudRate: cfg.baud || 115200 });
  const parser = port.pipe(new ReadlineParser({ delimiter: "\n" }));
  parser.on("data", onEvent);

  port.on("open", async () => {
    console.log(`Conectado no módulo (${path})`);
    await new Promise((r) => setTimeout(r, 600));
    send("COLOR 1DB954");   // verde Spotify
    send("SCREEN spotify");
    refresh();
    setInterval(refresh, 2000); // espelha estado a cada 2s
    console.log("Rodando. Mexa no módulo ou no Spotify. (Ctrl+C pra sair)\n");
  });
  port.on("error", (e) => console.error("Erro serial:", e.message));
}

main();

// Modue PC bridge — le o serial do modulo e aplica no Discord.
import { SerialPort } from "serialport";
import { ReadlineParser } from "@serialport/parser-readline";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import { makePlatform } from "./platform.js";

const __dir = dirname(fileURLToPath(import.meta.url));
const cfg = JSON.parse(readFileSync(join(__dir, "..", "config.json"), "utf8"));
const plat = makePlatform(cfg);

// Auto-detecta a porta da placa (usbmodem no Mac, COM no Windows)
async function resolvePort() {
  if (cfg.serialPort && cfg.serialPort !== "auto") return cfg.serialPort;
  const ports = await SerialPort.list();
  const hit = ports.find(
    (p) =>
      /usbmodem|usbserial|wchusbserial/i.test(p.path) ||
      /COM\d+/i.test(p.path) ||
      /esp|espressif|1a86|303a/i.test(`${p.vendorId}${p.manufacturer}`)
  );
  if (!hit) {
    console.error("Nenhuma porta serial encontrada. Portas:", ports.map((p) => p.path));
    process.exit(1);
  }
  // macOS: prefira /dev/cu.* a /dev/tty.* (cu nao bloqueia na abertura)
  if (process.platform === "darwin") return hit.path.replace("/tty.", "/cu.");
  return hit.path;
}

// Anti-repique: ignora eventos identicos em rajada (< 120ms)
let lastMute = 0, lastDeafen = 0;
function debounce(ts) {
  return Date.now() - ts < 120;
}

async function handleLine(line) {
  const s = line.trim();
  if (!s) return;
  const [cmd, arg] = s.split(/\s+/);

  switch (cmd) {
    case "VOL": {
      const v = Math.max(0, Math.min(100, parseInt(arg, 10) || 0));
      process.stdout.write(`\r🔊 Volume ${v}%   `);
      await plat.setVolume(v);
      break;
    }
    case "MUTE_MIC": {
      if (debounce(lastMute)) break;
      lastMute = Date.now();
      console.log(`\n🎤 Toggle mute (${arg})`);
      await plat.sendCombo(cfg.keybinds.mute);
      break;
    }
    case "MUTE_FONE": {
      if (debounce(lastDeafen)) break;
      lastDeafen = Date.now();
      console.log(`\n🎧 Toggle deafen (${arg})`);
      await plat.sendCombo(cfg.keybinds.deafen);
      break;
    }
    default:
      // logs de boot do firmware etc. — ignora
      if (/^\[Modue\]|UI pronta|gfx->/.test(s)) console.log(`  · ${s}`);
      break;
  }
}

async function main() {
  const path = await resolvePort();
  console.log(`Modue PC bridge — ${plat.name}`);
  console.log(`  Porta:  ${path} @ ${cfg.baud}`);
  console.log(`  Volume: ${plat.volumeNote}`);
  console.log(`  Mute:   ${comboStr(cfg.keybinds.mute)}   Deafen: ${comboStr(cfg.keybinds.deafen)}`);
  console.log("Aguardando eventos do modulo... (Ctrl+C pra sair)\n");

  const port = new SerialPort({ path, baudRate: cfg.baud });
  const parser = port.pipe(new ReadlineParser({ delimiter: "\n" }));
  parser.on("data", handleLine);
  port.on("error", (e) => console.error("Erro serial:", e.message));
  port.on("close", () => {
    console.error("\nPorta fechada. Saindo.");
    process.exit(1);
  });
}

function comboStr(kb) {
  const m = [];
  if (kb.ctrl) m.push("Ctrl");
  if (kb.alt) m.push("Alt/Opt");
  if (kb.shift) m.push("Shift");
  if (kb.meta) m.push("Cmd/Win");
  m.push(kb.key.toUpperCase());
  return m.join("+");
}

main();

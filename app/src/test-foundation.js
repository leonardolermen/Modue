// Testa o protocolo (Fase A): manda comandos pro modulo e mostra os eventos.
// Uso:  cd app && node src/test-foundation.js
import { SerialPort } from "serialport";
import { ReadlineParser } from "@serialport/parser-readline";

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function findPort() {
  const ports = await SerialPort.list();
  const hit = ports.find((p) => /usbmodem|usbserial|wchusbserial|COM\d+/i.test(p.path));
  if (!hit) { console.error("Placa nao encontrada."); process.exit(1); }
  return process.platform === "darwin" ? hit.path.replace("/tty.", "/cu.") : hit.path;
}

const path = await findPort();
const port = new SerialPort({ path, baudRate: 115200 });
const parser = port.pipe(new ReadlineParser({ delimiter: "\n" }));

parser.on("data", (line) => {
  const s = line.trim();
  if (s) console.log("  <- modulo:", s);
});

const send = (cmd) => { console.log("-> PC:", cmd); port.write(cmd + "\n"); };

port.on("open", async () => {
  console.log(`Conectado em ${path}\n`);
  await sleep(800);

  console.log("\n[1] Handshake");
  send("PING");
  await sleep(500);

  console.log("\n[2] Movendo o fader pelo PC (0 -> 100 -> 30)");
  for (let v = 0; v <= 100; v += 10) { send(`VOL ${v}`); await sleep(180); }
  send("VOL 30"); await sleep(600);

  console.log("\n[3] Mutando o mic e o fone pelo PC");
  send("STATE mic 1");  await sleep(900);
  send("STATE deaf 1"); await sleep(900);
  send("STATE mic 0");  await sleep(600);
  send("STATE deaf 0"); await sleep(600);

  console.log("\n[4] Trocando a cor de destaque (verde Spotify -> roxo)");
  send("COLOR 1DB954"); await sleep(1200);
  send("COLOR 5865F2"); await sleep(600);

  console.log("\n[5] Pronto! Agora TOQUE no modulo (fader/botoes) e veja os eventos chegando:");
  console.log("    (Ctrl+C pra sair)\n");
});

port.on("error", (e) => console.error("Erro serial:", e.message));

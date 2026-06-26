// Acoes especificas por sistema operacional.
// macOS  -> osascript (keystroke + volume do sistema)
// Windows -> PowerShell keybd_event (atalho global) + svcl (volume por app)
import { execFile } from "node:child_process";

const isMac = process.platform === "darwin";
const isWin = process.platform === "win32";

function run(cmd, args) {
  return new Promise((resolve) => {
    execFile(cmd, args, (err, stdout, stderr) => {
      if (err) console.error(`  ! ${cmd} falhou:`, stderr || err.message);
      resolve(!err);
    });
  });
}

// ---------- macOS ----------
function macModifiers(kb) {
  const m = [];
  if (kb.ctrl) m.push("control down");
  if (kb.alt) m.push("option down");
  if (kb.shift) m.push("shift down");
  if (kb.meta) m.push("command down");
  return m.length ? ` using {${m.join(", ")}}` : "";
}

async function macSendCombo(kb) {
  const script = `tell application "System Events" to keystroke "${kb.key}"${macModifiers(kb)}`;
  return run("osascript", ["-e", script]);
}

async function macSetVolume(pct) {
  // macOS nao tem volume por-app nativo -> controla o volume de saida do sistema.
  return run("osascript", ["-e", `set volume output volume ${pct}`]);
}

// ---------- Windows ----------
const VK = {
  ctrl: 0x11, alt: 0x12, shift: 0x10, meta: 0x5b, // win key
};
function winVk(key) {
  // letras/numeros usam o codigo ASCII maiusculo
  return key.toUpperCase().charCodeAt(0);
}

async function winSendCombo(kb) {
  const downs = [];
  const ups = [];
  if (kb.ctrl) { downs.push(VK.ctrl); ups.unshift(VK.ctrl); }
  if (kb.alt) { downs.push(VK.alt); ups.unshift(VK.alt); }
  if (kb.shift) { downs.push(VK.shift); ups.unshift(VK.shift); }
  if (kb.meta) { downs.push(VK.meta); ups.unshift(VK.meta); }
  const k = winVk(kb.key);
  downs.push(k); ups.unshift(k);

  // keybd_event gera eventos de teclado a nivel de SO -> atalho global do Discord pega.
  const ps = `
Add-Type -Name U -Namespace W -MemberDefinition '[DllImport("user32.dll")] public static extern void keybd_event(byte b, byte s, int f, int e);'
${downs.map((v) => `[W.U]::keybd_event(${v},0,0,0)`).join("; ")}
${ups.map((v) => `[W.U]::keybd_event(${v},0,2,0)`).join("; ")}
`.trim();
  return run("powershell", ["-NoProfile", "-Command", ps]);
}

async function winSetVolume(pct, cfg) {
  const svcl = cfg?.windows?.svclPath || "svcl.exe";
  return run(svcl, ["/SetVolume", cfg.discordProcess || "Discord.exe", String(pct)]);
}

// ---------- API publica ----------
export function makePlatform(cfg) {
  if (isMac) {
    return {
      name: "macOS",
      sendCombo: macSendCombo,
      setVolume: macSetVolume,
      volumeNote: "volume do SISTEMA (macOS nao tem volume por-app nativo)",
    };
  }
  if (isWin) {
    return {
      name: "Windows",
      sendCombo: winSendCombo,
      setVolume: (pct) => winSetVolume(pct, cfg),
      volumeNote: `volume do app ${cfg.discordProcess} via svcl`,
    };
  }
  return {
    name: process.platform,
    sendCombo: async () => console.warn("  ! SO nao suportado p/ atalhos"),
    setVolume: async () => console.warn("  ! SO nao suportado p/ volume"),
    volumeNote: "nao suportado",
  };
}

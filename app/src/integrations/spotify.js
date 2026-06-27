// Integração real com a Spotify Web API (OAuth Authorization Code + PKCE).
// Não precisa de client_secret. Guarda os tokens em disco e renova sozinho.
import http from "node:http";
import crypto from "node:crypto";
import { exec } from "node:child_process";
import { readFileSync, writeFileSync, existsSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const __dir = dirname(fileURLToPath(import.meta.url));
const TOKENS_FILE = join(__dir, "..", "..", ".spotify-tokens.json");

const AUTH = "https://accounts.spotify.com";
const API = "https://api.spotify.com/v1";
const SCOPES = [
  "user-read-playback-state",
  "user-modify-playback-state",
  "user-read-currently-playing",
].join(" ");

const b64url = (buf) =>
  buf.toString("base64").replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/, "");

function openBrowser(url) {
  const cmd =
    process.platform === "darwin" ? "open" :
    process.platform === "win32" ? "start \"\"" : "xdg-open";
  exec(`${cmd} "${url}"`);
}

export class Spotify {
  constructor(cfg) {
    this.clientId = cfg.clientId;
    this.redirectUri = cfg.redirectUri || "http://127.0.0.1:8888/callback";
    this.tokens = existsSync(TOKENS_FILE)
      ? JSON.parse(readFileSync(TOKENS_FILE, "utf8"))
      : null;
  }

  saveTokens(t) {
    this.tokens = { ...this.tokens, ...t, obtained_at: Date.now() };
    writeFileSync(TOKENS_FILE, JSON.stringify(this.tokens, null, 2));
  }

  // ---- Login (uma vez; depois usa refresh_token) ----
  async ensureAuth() {
    if (this.tokens?.refresh_token) return;
    await this.login();
  }

  login() {
    const verifier = b64url(crypto.randomBytes(48));
    const challenge = b64url(crypto.createHash("sha256").update(verifier).digest());
    const port = new URL(this.redirectUri).port || 8888;

    const url =
      `${AUTH}/authorize?response_type=code` +
      `&client_id=${this.clientId}` +
      `&redirect_uri=${encodeURIComponent(this.redirectUri)}` +
      `&code_challenge_method=S256&code_challenge=${challenge}` +
      `&scope=${encodeURIComponent(SCOPES)}`;

    return new Promise((resolve, reject) => {
      const server = http.createServer(async (req, res) => {
        if (!req.url.startsWith("/callback")) { res.writeHead(404); res.end(); return; }
        const code = new URL(req.url, this.redirectUri).searchParams.get("code");
        res.writeHead(200, { "Content-Type": "text/html; charset=utf-8" });
        res.end("<h2>Modue conectado ao Spotify ✅</h2><p>Pode fechar esta aba.</p>");
        server.close();
        try {
          const body = new URLSearchParams({
            grant_type: "authorization_code",
            code,
            redirect_uri: this.redirectUri,
            client_id: this.clientId,
            code_verifier: verifier,
          });
          const r = await fetch(`${AUTH}/api/token`, {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body,
          });
          const t = await r.json();
          if (t.error) return reject(new Error(t.error_description || t.error));
          this.saveTokens(t);
          resolve();
        } catch (e) { reject(e); }
      });
      server.listen(port, "127.0.0.1", () => {
        console.log("Abrindo o navegador pra autorizar no Spotify...");
        openBrowser(url);
      });
    });
  }

  async refresh() {
    const body = new URLSearchParams({
      grant_type: "refresh_token",
      refresh_token: this.tokens.refresh_token,
      client_id: this.clientId,
    });
    const r = await fetch(`${AUTH}/api/token`, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body,
    });
    const t = await r.json();
    if (t.error) throw new Error(t.error_description || t.error);
    this.saveTokens(t); // refresh_token pode não vir; o spread preserva o antigo
  }

  expired() {
    if (!this.tokens?.access_token) return true;
    return Date.now() > this.tokens.obtained_at + (this.tokens.expires_in - 60) * 1000;
  }

  // ---- chamada autenticada, renova token e tenta de novo no 401 ----
  async api(method, path, { query, retry = true } = {}) {
    if (this.expired()) await this.refresh();
    const qs = query ? "?" + new URLSearchParams(query) : "";
    const r = await fetch(`${API}${path}${qs}`, {
      method,
      headers: { Authorization: `Bearer ${this.tokens.access_token}` },
    });
    if (r.status === 401 && retry) { await this.refresh(); return this.api(method, path, { query, retry: false }); }
    if (r.status === 204) return null;            // nada tocando / sem conteúdo
    if (r.status === 403) throw new Error("403 — ação não permitida agora (restrição do Spotify, ex. sem device ativo ou estado já aplicado).");
    if (r.status === 404) throw new Error("404 — nenhum device ativo. Abra o Spotify e dê play em algo.");
    if (!r.ok) throw new Error(`Spotify ${r.status} em ${path}`);
    const txt = await r.text();
    if (!txt) return null;
    try { return JSON.parse(txt); } catch { return null; } // controles retornam corpo nao-JSON
  }

  // ---- estado atual (faixa, volume, tocando) ----
  async getState() {
    const p = await this.api("GET", "/me/player");
    if (!p) return null;
    const item = p.item;
    return {
      isPlaying: !!p.is_playing,
      volume: p.device?.volume_percent ?? null,
      track: item ? `${item.name} — ${item.artists.map((a) => a.name).join(", ")}` : null,
      albumArt: item?.album?.images?.[0]?.url ?? null,
    };
  }

  // ---- controles ----
  play() { return this.api("PUT", "/me/player/play"); }
  pause() { return this.api("PUT", "/me/player/pause"); }
  next() { return this.api("POST", "/me/player/next"); }
  prev() { return this.api("POST", "/me/player/previous"); }
  setVolume(pct) {
    return this.api("PUT", "/me/player/volume", { query: { volume_percent: Math.round(pct) } });
  }
}

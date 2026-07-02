// ============================================================
//  Modue — Case do módulo (bandeja/corpo)  v1
//  Fader vertical + display no topo (em linha). Unidades: mm.
//
//  Origem: centro em X e Y; Z=0 = traseira (aberta, recebe a tampa);
//  frente (com janela do display + fenda do knob) em Z = out_d.
//  Componentes entram pela traseira.
// ============================================================

// ---------- Impressão / tolerâncias ----------
wall = 2.0;    // espessura de parede
fit  = 0.3;    // folga de encaixe (FDM)
$fn  = 40;

// ---------- Placa ESP32-C6-LCD-1.47 ----------
pcb_l = 36.37;
pcb_w = 20.32;
win_w = 17.7;   // janela do display (área visível)
win_h = 32.9;

// ---------- Fader (ALPS 80mm, travel 60) ----------
fad_l    = 80.0;
fad_w    = 18.0;
fad_up   = 8.2;    // acima da superfície de montagem
fad_deep = 17.8;   // abaixo (26 total − 8.2)
fad_trav = 60.0;   // curso do knob
slot_w   = 4.0;    // largura da fenda do knob

// ---------- Arranjo ----------
gap_fd = 4.0;   // vão fader <-> placa
usb_w  = 9.5;   // rasgo USB-C (largura)
usb_h  = 3.6;   // rasgo USB-C (altura)

// ---------- Derivadas ----------
inner_w = max(fad_w, pcb_w) + 2*fit;
inner_l = fad_l + gap_fd + pcb_l + 2*fit;
inner_d = fad_deep + fad_up + fit;    // profundidade útil (fader é o mais fundo)

out_w = inner_w + 2*wall;
out_l = inner_l + 2*wall;
out_d = inner_d + wall;               // parede da frente

// centros no comprimento (Y)
fad_y  = -inner_l/2 + fit + fad_l/2;
disp_y =  inner_l/2 - fit - pcb_l/2;

module box(w, l, d) { translate([-w/2,-l/2,0]) cube([w,l,d]); }

module tray() {
  difference() {
    // casca sólida
    box(out_w, out_l, out_d);

    // cavidade interna aberta na traseira (Z de -1 até out_d-wall)
    translate([0,0,-1]) box(inner_w, inner_l, (out_d - wall) + 1);

    // fenda do knob na frente (sobre o fader)
    translate([0, fad_y, out_d - wall - 1])
      box(slot_w, fad_trav, wall + 2);

    // janela do display na frente
    translate([0, disp_y, out_d - wall - 1])
      box(win_w, win_h, wall + 2);

    // rasgo do USB-C (parede lateral, perto da base/placa)
    translate([out_w/2 - wall - 1, disp_y, wall])
      box(wall + 2, usb_w, usb_h);
  }
}

tray();

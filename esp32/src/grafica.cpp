#include "grafica.h"

LGFX lcd;
LGFX_Sprite tela(&lcd);

void graficaInizio() {
  lcd.init();
  lcd.setRotation(1);          // orizzontale: 320 x 170
  lcd.setBrightness(0);
  tela.setColorDepth(16);
  tela.createSprite(SCHERMO_L, SCHERMO_A);
  tela.fillScreen(col::sfondo);
  mostra();
  for (int b = 0; b <= 200; b += 8) { lcd.setBrightness(b); delay(6); }   // accensione dolce
}

void mostra() { tela.pushSprite(0, 0); }

float avvicina(float attuale, float meta, float k) {
  float d = meta - attuale;
  return (fabsf(d) < 0.4f) ? meta : attuale + d * k;
}

void iconaWifi(int cx, int cy, int livello, uint32_t colore) {
  // tre archi concentrici + punto, come l'icona di sistema; gli archi non raggiunti sono grigi
  for (int i = 0; i < 3; ++i) {
    int r = 4 + i * 4;
    uint32_t c = (i < livello) ? colore : col::separatore;
    tela.fillArc(cx, cy, r + 2, r, 225, 315, c);
  }
  tela.fillCircle(cx, cy - 1, 2, livello > 0 ? colore : col::separatore);
}

void lucchetto(int x, int y, uint32_t colore) {
  tela.drawArc(x + 4, y + 4, 4, 3, 180, 360, colore);
  tela.fillRoundRect(x, y + 4, 9, 7, 1, colore);
}

void intestazione(const char* titolo, bool wifi, bool indietro) {
  tela.setTextDatum(textdatum_t::middle_left);
  int x = 12;
  if (indietro) {                       // freccia "indietro" stile iOS
    tela.drawLine(16, 10, 10, 16, col::blu); tela.drawLine(10, 16, 16, 22, col::blu);
    tela.drawLine(17, 10, 11, 16, col::blu); tela.drawLine(11, 16, 17, 22, col::blu);
    x = 24;
  }
  tela.setFont(&fonts::FreeSansBold12pt7b);
  tela.setTextColor(col::testo);
  tela.drawString(titolo, x, 16);
  if (wifi) iconaWifi(SCHERMO_L - 18, 22, 3, col::testo);
}

void iconaTessera(int x, int y, uint32_t colore, const char* lettera) {
  tela.fillSmoothRoundRect(x, y, 22, 22, 6, colore);
  tela.setFont(&fonts::FreeSansBold9pt7b);
  tela.setTextColor(col::testo);
  tela.setTextDatum(textdatum_t::middle_center);
  tela.drawString(lettera, x + 11, y + 11);
}

void rotellina(int cx, int cy, int r, float fase, uint32_t colore) {
  for (int i = 0; i < 12; ++i) {        // raggi che sfumano, come l'indicatore di attività
    float a = (i * 30.0f + fase) * DEG_TO_RAD;
    uint8_t lum = 40 + (i * 215) / 11;
    uint32_t c = ((((colore >> 16) & 0xFF) * lum / 255) << 16) | ((((colore >> 8) & 0xFF) * lum / 255) << 8) |
                 ((colore & 0xFF) * lum / 255);
    tela.drawWideLine(cx + cosf(a) * r * 0.5f, cy + sinf(a) * r * 0.5f, cx + cosf(a) * r, cy + sinf(a) * r, 1.6f, c);
  }
}

void spunta(int cx, int cy, float t, uint32_t colore) {
  tela.fillSmoothCircle(cx, cy, 22, colore);
  float t1 = constrain(t * 2.0f, 0.0f, 1.0f), t2 = constrain(t * 2.0f - 1.0f, 0.0f, 1.0f);
  int ax = cx - 10, ay = cy, bx = cx - 3, by = cy + 7, dx = cx + 11, dy = cy - 8;
  tela.drawWideLine(ax, ay, ax + (bx - ax) * t1, ay + (by - ay) * t1, 3.0f, col::testo);
  if (t2 > 0) tela.drawWideLine(bx, by, bx + (dx - bx) * t2, by + (dy - by) * t2, 3.0f, col::testo);
}

void barraAiuto(const char* testo) {
  tela.setFont(&fonts::Font0);
  tela.setTextColor(col::secondario);
  tela.setTextDatum(textdatum_t::bottom_center);
  tela.drawString(testo, SCHERMO_L / 2, SCHERMO_A - 3);
}

void bandiera(int x, int y, int w, int h, bool italiana) {
  if (italiana) {
    tela.fillRect(x, y, w / 3, h, 0x009246);                   // verde, bianco, rosso
    tela.fillRect(x + w / 3, y, w - 2 * (w / 3), h, 0xFFFFFF);
    tela.fillRect(x + w - w / 3, y, w / 3, h, 0xCE2B37);
  } else {
    tela.setClipRect(x, y, w, h);                              // Union Jack semplificata
    tela.fillRect(x, y, w, h, 0x012169);
    tela.drawWideLine(x, y, x + w, y + h, h * 0.20f, 0xFFFFFF);
    tela.drawWideLine(x + w, y, x, y + h, h * 0.20f, 0xFFFFFF);
    tela.drawWideLine(x, y, x + w, y + h, h * 0.07f, 0xC8102E);
    tela.drawWideLine(x + w, y, x, y + h, h * 0.07f, 0xC8102E);
    tela.fillRect(x + w / 2 - h / 6, y, h / 3, h, 0xFFFFFF);
    tela.fillRect(x, y + h / 2 - h / 6, w, h / 3, 0xFFFFFF);
    tela.fillRect(x + w / 2 - h / 10, y, h / 5, h, 0xC8102E);
    tela.fillRect(x, y + h / 2 - h / 10, w, h / 5, 0xC8102E);
    tela.clearClipRect();
  }
  tela.drawRoundRect(x - 1, y - 1, w + 2, h + 2, 3, col::separatore);   // bordo sottile
}

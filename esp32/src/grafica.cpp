#include "grafica.h"
#include "logo.h"
#include "suoni.h"

LGFX lcd;
LGFX_Sprite tela(&lcd);

void graficaInizio() {
  lcd.init();
  lcd.setRotation(1);          // orizzontale: 320 x 170 (rimesso com'era: Piero 2026-10-25, sera)
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
  logoFilo(SCHERMO_L - 10 - 21 * LOGO_L / 1000, 6, 21, 1.0f, 1.1f, col::testo);   // simbolo di Plekto in alto a destra
  if (wifi) iconaWifi(SCHERMO_L - 46, 22, 3, col::secondario);
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

// ---------------------------------------------------------------- logo di Plekto
void logoFilo(int x, int y, int h, float frazione, float spessore, uint32_t colore, float giro) {
  // il filo si disegna dall'inizio fino a "frazione" della sua lunghezza (0..1): animazione della piega
  static float lung[LOGO_N];
  static bool pronto = false;
  if (!pronto) {
    lung[0] = 0;
    for (int i = 1; i < LOGO_N; ++i)
      lung[i] = lung[i - 1] + hypotf(LOGO_PUNTI[i][0] - LOGO_PUNTI[i - 1][0], LOGO_PUNTI[i][1] - LOGO_PUNTI[i - 1][1]);
    pronto = true;
  }
  float fine = constrain(frazione, 0.0f, 1.0f) * lung[LOGO_N - 1];
  float k = h / 1000.0f;
  // rotazione attorno all'asse verticale: la larghezza si schiaccia col coseno, il retro è un po' più scuro
  float c = cosf(giro * TWO_PI), meta = LOGO_L / 2.0f, cx = x + meta * k;
  if (giro != 0) {
    float lum = 0.55f + 0.45f * fabsf(c);
    colore = tela.color888(((colore >> 16) & 0xFF) * lum, ((colore >> 8) & 0xFF) * lum, (colore & 0xFF) * lum);
  }
  for (int i = 1; i < LOGO_N && lung[i - 1] < fine; ++i) {
    float ax = cx + (LOGO_PUNTI[i - 1][0] - meta) * k * c, ay = y + LOGO_PUNTI[i - 1][1] * k;
    float bx = cx + (LOGO_PUNTI[i][0] - meta) * k * c, by = y + LOGO_PUNTI[i][1] * k;
    if (lung[i] > fine) {                      // ultimo tratto parziale
      float u = (fine - lung[i - 1]) / (lung[i] - lung[i - 1]);
      bx = ax + (bx - ax) * u;
      by = ay + (by - ay) * u;
    }
    tela.drawWideLine(ax, ay, bx, by, spessore, colore);
  }
}

void scrittaPlekto(int x, int y, float alfa, uint32_t colore) {
  uint8_t r = (colore >> 16) & 0xFF, g = (colore >> 8) & 0xFF, b = colore & 0xFF;
  alfa = constrain(alfa, 0.0f, 1.0f);
  for (int j = 0; j < SCRITTA_A; ++j)
    for (int i = 0; i < SCRITTA_L; ++i) {
      uint8_t a = pgm_read_byte(&SCRITTA_ALFA[j * SCRITTA_L + i]);
      if (!a) continue;
      float f = a / 255.0f * alfa;
      tela.drawPixel(x + i, y + j, tela.color888(r * f, g * f, b * f));
    }
}

static float morbido(float t) { t = constrain(t, 0.0f, 1.0f); return t * t * (3 - 2 * t); }

void animazioneAvvio() {
  const int hL = 92, wL = hL * LOGO_L / 1000, gap = 14;
  const int x0 = (SCHERMO_L - (wL + gap + SCRITTA_L)) / 2, y0 = (SCHERMO_A - hL) / 2;
  const int xs = x0 + wL + gap, ys = y0 + hL * 68 / 100 - SCRITTA_BASE;    // linea di base della scritta
  uint32_t inizio = millis();
  bool suonato = false;
  for (;;) {
    float t = (millis() - inizio) / 1000.0f;
    if (!suonato && t > 2.15f) { suona(SUONO_AVVIO); suonato = true; }   // la melodia accompagna il giro e la scritta
    tela.fillScreen(col::sfondo);
    float giro = morbido((t - 2.2f) / 1.4f);                                       // 2. gira su se stesso
    logoFilo(x0, y0, hL, morbido(t / 2.0f), 2.3f, col::testo, giro);               // 1. il filo si piega
    float a = morbido((t - 3.1f) / 1.0f);                                          // 3. compare "plekto"
    if (a > 0) scrittaPlekto(xs + (int)(8 * (1 - a)), ys, a, col::testo);
    mostra();
    if (t > 4.6f) break;
  }
}

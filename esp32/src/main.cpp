// Bending machine ortodontica - firmware ESP32 (interfaccia utente)
// Primo avvio: lingua (italiano/inglese) -> rete Wi-Fi -> password scritta con la rotella -> menu.
// Le scelte sono salvate nella memoria NVS e condivise col Raspberry Pi Zero sul collegamento USB.
#include <Arduino.h>
#include <WiFi.h>
#include "ota.h"
#include "aggiornamento.h"
#include <vector>
#include "config.h"
#include "grafica.h"
#include "i18n.h"
#include "impostazioni.h"
#include "input.h"
#include "collegamento.h"

enum Schermata { S_LINGUA, S_RICERCA, S_RETI, S_PASSWORD, S_CONNESSIONE, S_MENU, S_INFO, S_IN_ARRIVO, S_NUOVO_FW,
                 S_ESITO_AGG };
static void controllaAggiornamenti(bool mostraEsito);
static Schermata schermata;
static bool daMenu = false;              // lingua/rete aperte dal menu: alla fine si torna al menu
static uint32_t inizioSchermata = 0;

static void vai(Schermata s) { schermata = s; inizioSchermata = millis(); }
static float t() { return (millis() - inizioSchermata) / 1000.0f; }

// ---------------------------------------------------------------- lista con evidenziatore animato
struct Voce { String testo; uint32_t colore; String lettera; String destra; int livelloWifi; bool lucchetto; };

struct Lista {
  std::vector<Voce> voci;
  int sel = 0;
  float yEvid = 0, scorri = 0;
  static constexpr int y0 = 36, h = 30;
  void muovi(int d) { if (!voci.empty()) sel = constrain(sel + d, 0, (int)voci.size() - 1); }
  void disegna() {
    int visibili = (SCHERMO_A - y0) / h;
    float metaScorri = constrain(sel - visibili + 2, 0, max(0, (int)voci.size() - visibili));
    scorri = avvicina(scorri, metaScorri * h);
    yEvid = avvicina(yEvid, sel * h);
    tela.fillSmoothRoundRect(6, y0, SCHERMO_L - 12, SCHERMO_A - y0 - 4, 10, col::scheda);
    tela.setClipRect(6, y0, SCHERMO_L - 12, SCHERMO_A - y0 - 4);
    tela.fillSmoothRoundRect(8, y0 + 2 + yEvid - scorri, SCHERMO_L - 16, h - 4, 8, col::evidenza);
    for (size_t i = 0; i < voci.size(); ++i) {
      int y = y0 + i * h - scorri;
      if (y < y0 - h || y > SCHERMO_A) continue;
      const Voce& v = voci[i];
      int x = 16;
      if (v.lettera.length()) { iconaTessera(x, y + 4, v.colore, v.lettera.c_str()); x += 32; }
      if (v.livelloWifi >= 0) { iconaWifi(x + 8, y + 22, v.livelloWifi, col::testo); x += 26; }
      tela.setFont(&fonts::FreeSans9pt7b);
      tela.setTextColor(col::testo);
      tela.setTextDatum(textdatum_t::middle_left);
      tela.drawString(v.testo.c_str(), x, y + h / 2);
      int xd = SCHERMO_L - 22;
      // chevron ">" a destra
      tela.drawLine(xd, y + 10, xd + 5, y + 15, col::secondario); tela.drawLine(xd + 5, y + 15, xd, y + 20, col::secondario);
      if (v.lucchetto) lucchetto(xd - 18, y + 9, col::secondario);
      if (v.destra.length()) {
        tela.setTextColor(col::secondario);
        tela.setTextDatum(textdatum_t::middle_right);
        tela.drawString(v.destra.c_str(), xd - (v.lucchetto ? 24 : 8), y + h / 2);
      }
      if (i + 1 < voci.size()) tela.drawFastHLine(x, y + h - 1, SCHERMO_L - 14 - x, col::separatore);
    }
    tela.clearClipRect();
  }
};

static Lista menu, reti;
static std::vector<String> retiSsid;
static std::vector<bool> retiProtette;

// ---------------------------------------------------------------- 1. lingua
static int selLingua = 0;
static float xLingua = 0;

static void disegnaLingua() {
  tela.setTextDatum(textdatum_t::middle_center);
  tela.setFont(&fonts::FreeSansBold18pt7b);
  tela.setTextColor(col::testo);
  tela.drawString(selLingua == 0 ? "Benvenuto" : "Welcome", SCHERMO_L / 2, 38);
  tela.setFont(&fonts::FreeSans9pt7b);
  tela.setTextColor(col::secondario);
  tela.drawString(selLingua == 0 ? "Scegli la lingua" : "Choose your language", SCHERMO_L / 2, 70);
  const char* nomi[2] = {"Italiano", "English"};
  const uint32_t colori[2] = {col::verde, col::blu};
  xLingua = avvicina(xLingua, selLingua * 150.0f);
  tela.fillSmoothRoundRect(15 + xLingua, 92, 140, 50, 14, col::evidenza);
  for (int i = 0; i < 2; ++i) {
    int x = 15 + i * 150;
    tela.drawRoundRect(x, 92, 140, 50, 14, i == selLingua ? colori[i] : col::separatore);
    bandiera(x + 12, 108, 27, 18, i == 0);
    tela.setFont(&fonts::FreeSansBold9pt7b);
    tela.setTextColor(col::testo);
    tela.setTextDatum(textdatum_t::middle_left);
    tela.drawString(nomi[i], x + 46, 117);
  }
  barraAiuto(tr(T_AIUTO_LISTA));
}

static void avviaRicerca() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.scanNetworks(true);            // ricerca in background: l'interfaccia resta animata
  vai(S_RICERCA);
}

static void eventoLingua(Evento e) {
  if (e == EV_ORARIO || e == EV_DESTRA) selLingua = 1;
  if (e == EV_ANTIORARIO || e == EV_SINISTRA) selLingua = 0;
  if (e == EV_CENTRO) {
    imp.lingua = (Lingua)selLingua;
    impostazioniSalva();
    if (daMenu) { daMenu = false; vai(S_MENU); } else avviaRicerca();
  }
}

// ---------------------------------------------------------------- 2. ricerca e scelta della rete
static void disegnaRicerca() {
  intestazione(tr(T_RETE_WIFI), false, false);
  rotellina(SCHERMO_L / 2, 95, 18, millis() * 0.36f, col::testo);
  tela.setFont(&fonts::FreeSans9pt7b);
  tela.setTextColor(col::secondario);
  tela.setTextDatum(textdatum_t::middle_center);
  tela.drawString(tr(T_RICERCA_RETI), SCHERMO_L / 2, 135);
  int n = WiFi.scanComplete();
  if (n >= 0) {
    reti.voci.clear(); retiSsid.clear(); retiProtette.clear();
    std::vector<int> ordine;
    for (int i = 0; i < n; ++i) ordine.push_back(i);
    std::sort(ordine.begin(), ordine.end(), [](int a, int b) { return WiFi.RSSI(a) > WiFi.RSSI(b); });
    for (int i : ordine) {
      String s = WiFi.SSID(i);
      if (!s.length() || std::find(retiSsid.begin(), retiSsid.end(), s) != retiSsid.end()) continue;
      int r = WiFi.RSSI(i);
      bool chiusa = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
      reti.voci.push_back({s, 0, "", "", r > -60 ? 3 : r > -72 ? 2 : 1, chiusa});
      retiSsid.push_back(s); retiProtette.push_back(chiusa);
    }
    WiFi.scanDelete();
    reti.voci.push_back({tr(T_RIPROVA), col::blu, "R", "", -1, false});
    reti.voci.push_back({tr(T_SALTA), col::grigio, "-", "", -1, false});
    reti.sel = 0;
    vai(S_RETI);
  }
}

static String pwd;
static int insieme = 0, carattere = 0;
static bool mostraPwd = false;
static float xCar = 0;
static String reteScelta;

static void eventoReti(Evento e) {
  if (e == EV_ORARIO || e == EV_GIU) reti.muovi(1);
  if (e == EV_ANTIORARIO || e == EV_SU) reti.muovi(-1);
  if (e == EV_SINISTRA && daMenu) { daMenu = false; vai(S_MENU); }
  if (e != EV_CENTRO) return;
  int n = retiSsid.size();
  if (reti.sel == n) { avviaRicerca(); return; }                       // riprova
  if (reti.sel == n + 1) { imp.configurato = true; impostazioniSalva(); daMenu = false; vai(S_MENU); return; }   // salta
  reteScelta = retiSsid[reti.sel];
  pwd = (reteScelta == imp.ssid) ? imp.password : "";
  if (!retiProtette[reti.sel]) { pwd = ""; WiFi.begin(reteScelta.c_str()); vai(S_CONNESSIONE); return; }
  vai(S_PASSWORD);
}

static void disegnaReti() {
  intestazione(tr(T_RETE_WIFI), false, daMenu);
  reti.disegna();
}

// ---------------------------------------------------------------- 3. password con la rotella
static const char* const INSIEMI[4] = {"abcdefghijklmnopqrstuvwxyz", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "0123456789",
                                       "!@#$%&*?-_.,:;+=/\\()[]{}<>'\"^~`| "};
static const char* const NOMI_INSIEMI[4] = {"abc", "ABC", "123", "#+="};

static void disegnaPassword() {
  intestazione(reteScelta.c_str(), false, true);
  // campo password
  tela.fillSmoothRoundRect(10, 36, SCHERMO_L - 20, 34, 10, col::scheda);
  String vis;
  for (size_t i = 0; i < pwd.length(); ++i) vis += (mostraPwd || i + 1 == pwd.length()) ? pwd[i] : '*';
  tela.setFont(&fonts::FreeMono12pt7b);
  tela.setTextColor(col::testo);
  tela.setTextDatum(textdatum_t::middle_left);
  int lw = tela.textWidth(vis.c_str());
  int xs = min(20, SCHERMO_L - 40 - lw);
  tela.setClipRect(14, 36, SCHERMO_L - 28, 34);
  tela.drawString(vis.c_str(), xs, 54);
  if ((millis() / 500) % 2) tela.fillRect(xs + lw + 2, 44, 2, 20, col::blu);      // cursore lampeggiante
  tela.clearClipRect();
  // carosello dei caratteri
  const char* ins = INSIEMI[insieme];
  int n = strlen(ins);
  xCar = avvicina(xCar, carattere * 34.0f);
  tela.fillSmoothRoundRect(SCHERMO_L / 2 - 20, 80, 40, 46, 10, col::blu);
  tela.setTextDatum(textdatum_t::middle_center);
  for (int i = 0; i < n; ++i) {
    float x = SCHERMO_L / 2 + i * 34.0f - xCar;
    if (x < -20 || x > SCHERMO_L + 20) continue;
    float d = fabsf(x - SCHERMO_L / 2) / (SCHERMO_L / 2);
    tela.setFont(fabsf(x - SCHERMO_L / 2) < 17 ? &fonts::FreeSansBold18pt7b : &fonts::FreeSans12pt7b);
    uint8_t g = 255 - (uint8_t)(min(d, 1.0f) * 170);
    tela.setTextColor(fabsf(x - SCHERMO_L / 2) < 17 ? col::testo : (uint32_t)((g << 16) | (g << 8) | g));
    char c[2] = {ins[i] == ' ' ? '_' : ins[i], 0};
    tela.drawString(c, x, 103);
  }
  // insieme di caratteri attivo
  tela.setFont(&fonts::Font2);
  for (int k = 0; k < 4; ++k) {
    int x = 70 + k * 50;
    if (k == insieme) tela.fillSmoothRoundRect(x - 20, 131, 40, 16, 8, col::evidenza);
    tela.setTextColor(k == insieme ? col::testo : col::secondario);
    tela.drawString(NOMI_INSIEMI[k], x, 139);
  }
  barraAiuto(tr(T_AIUTO_PASSWORD));
}

static void eventoPassword(Evento e) {
  int n = strlen(INSIEMI[insieme]);
  switch (e) {
    case EV_ORARIO: carattere = (carattere + 1) % n; break;
    case EV_ANTIORARIO: carattere = (carattere + n - 1) % n; break;
    case EV_CENTRO: if (pwd.length() < 63) pwd += INSIEMI[insieme][carattere]; break;
    case EV_SINISTRA: if (pwd.length()) pwd.remove(pwd.length() - 1); else vai(S_RETI); break;
    case EV_SU: insieme = (insieme + 1) % 4; carattere = 0; xCar = 0; break;
    case EV_GIU: mostraPwd = !mostraPwd; break;
    case EV_DESTRA: case EV_CENTRO_LUNGO:
      WiFi.begin(reteScelta.c_str(), pwd.c_str());
      vai(S_CONNESSIONE);
      break;
    default: break;
  }
}

// ---------------------------------------------------------------- 4. connessione
static bool connessioneOk = false, connessioneFallita = false;

static void disegnaConnessione() {
  intestazione(reteScelta.c_str(), false, false);
  tela.setFont(&fonts::FreeSans9pt7b);
  tela.setTextDatum(textdatum_t::middle_center);
  if (!connessioneOk && !connessioneFallita) {
    if (WiFi.status() == WL_CONNECTED) {
      connessioneOk = true; inizioSchermata = millis();
      imp.ssid = reteScelta; imp.password = pwd; imp.configurato = true;
      impostazioniSalva();                                   // salvate e mandate al Raspberry
    } else if (t() > 15) {
      connessioneFallita = true; inizioSchermata = millis();
    }
    rotellina(SCHERMO_L / 2, 90, 18, millis() * 0.36f, col::testo);
    tela.setTextColor(col::secondario);
    tela.drawString(tr(T_CONNESSIONE), SCHERMO_L / 2, 132);
  } else if (connessioneOk) {
    spunta(SCHERMO_L / 2, 88, min(t() / 0.6f, 1.0f), col::verde);
    tela.setTextColor(col::testo);
    tela.drawString(tr(T_CONNESSO), SCHERMO_L / 2, 128);
    tela.setTextColor(col::secondario);
    tela.drawString(WiFi.localIP().toString().c_str(), SCHERMO_L / 2, 148);
    if (t() > 1.6f) { connessioneOk = false; daMenu = false; vai(S_MENU); }
  } else {
    tela.fillSmoothCircle(SCHERMO_L / 2, 88, 22, col::rosso);
    tela.drawWideLine(SCHERMO_L / 2 - 8, 80, SCHERMO_L / 2 + 8, 96, 3, col::testo);
    tela.drawWideLine(SCHERMO_L / 2 + 8, 80, SCHERMO_L / 2 - 8, 96, 3, col::testo);
    tela.setTextColor(col::testo);
    tela.drawString(tr(T_ERRORE_WIFI), SCHERMO_L / 2, 128);
    barraAiuto(tr(T_AIUTO_LISTA));
  }
}

static void eventoConnessione(Evento e) {
  if (connessioneFallita && (e == EV_CENTRO || e == EV_SINISTRA)) { connessioneFallita = false; vai(S_PASSWORD); }
}

// ---------------------------------------------------------------- 5. menu principale
static void costruisciMenu() {
  int s = menu.sel;
  menu.voci = {
    {tr(T_PIEGA), col::arancio, "P", "", -1, false},
    {tr(T_CARICA_FILO), col::verde, "C", "", -1, false},
    {tr(T_TAGLIO_PEEK), col::rosso, "T", "", -1, false},
    {tr(T_LUCI), col::giallo, "L", "", -1, false},
    {tr(T_RETE_WIFI), col::blu, "W", WiFi.status() == WL_CONNECTED ? imp.ssid : String(tr(T_NON_COLLEGATO)), -1, false},
    {tr(T_LINGUA), col::viola, "A", imp.lingua == IT ? "Italiano" : "English", -1, false},
    {tr(T_AGGIORNAMENTI), col::grigio, "U", FW_VERSION, -1, false},
    {tr(T_INFO), col::azzurro, "i", "", -1, false},
  };
  menu.sel = s;
}

static void disegnaMenu() {
  costruisciMenu();
  intestazione(tr(T_MENU), WiFi.status() == WL_CONNECTED, false);
  menu.disegna();
}

static void eventoMenu(Evento e) {
  if (e == EV_ORARIO || e == EV_GIU) menu.muovi(1);
  if (e == EV_ANTIORARIO || e == EV_SU) menu.muovi(-1);
  if (e != EV_CENTRO && e != EV_DESTRA) return;
  switch (menu.sel) {
    case 4: daMenu = true; avviaRicerca(); break;
    case 5: daMenu = true; selLingua = imp.lingua; xLingua = selLingua * 150.0f; vai(S_LINGUA); break;
    case 6: controllaAggiornamenti(true); break;
    case 7: vai(S_INFO); break;
    default: vai(S_IN_ARRIVO); break;
  }
}

static void disegnaInfo() {
  intestazione(tr(T_INFO), WiFi.status() == WL_CONNECTED, true);
  tela.fillSmoothRoundRect(6, 36, SCHERMO_L - 12, SCHERMO_A - 40, 10, col::scheda);
  tela.setFont(&fonts::FreeSans9pt7b);
  String righe[5][2] = {
    {"Firmware", FW_VERSION},
    {tr(T_RETE_WIFI), WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String(tr(T_NON_COLLEGATO))},
    {"ANO", inputRotellaPresente() ? "OK" : "-"},
    {tr(T_RASPBERRY), rasp.visto ? (rasp.wifi ? rasp.ip : String("USB OK")) : String(tr(T_NON_COLLEGATO))},
    {"Config.", String("v") + imp.versione},
  };
  for (int i = 0; i < 5; ++i) {
    int y = 50 + i * 22;
    tela.setTextColor(col::testo); tela.setTextDatum(textdatum_t::middle_left); tela.drawString(righe[i][0].c_str(), 18, y);
    tela.setTextColor(col::secondario); tela.setTextDatum(textdatum_t::middle_right); tela.drawString(righe[i][1].c_str(), SCHERMO_L - 18, y);
  }
  barraAiuto(tr(T_SINCRONIZZATO));
}

// ---------------------------------------------------------------- aggiornamenti da internet (release GitHub)
static EsitoControllo esitoAgg = AGG_GIA_AGGIORNATO;

static void controllaAggiornamenti(bool mostraEsito) {
  if (mostraEsito) {                      // il controllo blocca 1-3 s: prima disegno "Controllo..."
    tela.fillScreen(col::sfondo);
    intestazione(tr(T_AGGIORNAMENTI), WiFi.status() == WL_CONNECTED, true);
    tela.setFont(&fonts::FreeSans12pt7b);
    tela.setTextColor(col::secondario);
    tela.setTextDatum(textdatum_t::middle_center);
    tela.drawString(tr(T_CONTROLLO_AGG), SCHERMO_L / 2, 100);
    mostra();
  }
  esitoAgg = aggControlla();
  if (esitoAgg == AGG_NUOVO) vai(S_NUOVO_FW);
  else if (mostraEsito) vai(S_ESITO_AGG);
}

static void disegnaNuovoFw() {
  intestazione(tr(T_AGGIORNAMENTI), true, true);
  tela.setTextDatum(textdatum_t::middle_center);
  tela.setFont(&fonts::FreeSansBold12pt7b);
  tela.setTextColor(col::testo);
  tela.drawString(tr(T_NUOVO_FW), SCHERMO_L / 2, 70);
  tela.setFont(&fonts::FreeSans12pt7b);
  tela.setTextColor(col::verde);
  tela.drawString((String(FW_VERSION) + "  >  " + aggVersioneNuova()).c_str(), SCHERMO_L / 2, 102);
  barraAiuto(tr(T_AIUTO_INSTALLA));
}

static void disegnaEsitoAgg() {
  intestazione(tr(T_AGGIORNAMENTI), WiFi.status() == WL_CONNECTED, true);
  tela.setTextDatum(textdatum_t::middle_center);
  tela.setFont(&fonts::FreeSans12pt7b);
  tela.setTextColor(esitoAgg == AGG_ERRORE ? col::arancio : col::testo);
  tela.drawString(tr(esitoAgg == AGG_ERRORE ? T_ERRORE_RETE : T_GIA_AGGIORNATO), SCHERMO_L / 2, 90);
  tela.setFont(&fonts::FreeSans9pt7b);
  tela.setTextColor(col::secondario);
  tela.drawString((String("Firmware ") + FW_VERSION).c_str(), SCHERMO_L / 2, 122);
}

static void disegnaInArrivo() {
  intestazione(menu.voci.size() ? menu.voci[menu.sel].testo.c_str() : "", WiFi.status() == WL_CONNECTED, true);
  tela.setFont(&fonts::FreeSans12pt7b);
  tela.setTextColor(col::secondario);
  tela.setTextDatum(textdatum_t::middle_center);
  tela.drawString(tr(T_IN_ARRIVO), SCHERMO_L / 2, 100);
}

// ---------------------------------------------------------------- avvio e ciclo principale
static Lingua linguaMostrata;

void setup() {
  impostazioniCarica();
  collegamentoInizio();
  graficaInizio();
  inputInizio();
  linguaMostrata = imp.lingua;
  if (imp.configurato) {
    if (imp.ssid.length()) { WiFi.mode(WIFI_STA); WiFi.begin(imp.ssid.c_str(), imp.password.c_str()); }
    vai(S_MENU);
  } else {
    vai(S_LINGUA);
  }
}

void loop() {
  otaAggiorna();
  collegamentoAggiorna();
  Evento e;
  while ((e = inputLeggi()) != EV_NIENTE) {
    if ((schermata == S_INFO || schermata == S_IN_ARRIVO || schermata == S_ESITO_AGG) && (e == EV_SINISTRA || e == EV_CENTRO)) {
      vai(S_MENU);
      continue;
    }
    if (schermata == S_NUOVO_FW) {
      if (e == EV_SINISTRA) vai(S_MENU);
      else if (e == EV_CENTRO || e == EV_DESTRA) { if (!aggInstalla()) vai(S_MENU); }
      continue;
    }
    if (schermata == S_MENU && e == EV_SINISTRA) continue;
    switch (schermata) {
      case S_LINGUA: eventoLingua(e); break;
      case S_RETI: eventoReti(e); break;
      case S_PASSWORD: eventoPassword(e); break;
      case S_CONNESSIONE: eventoConnessione(e); break;
      case S_MENU: eventoMenu(e); break;
      default: break;
    }
  }
  // controllo automatico degli aggiornamenti: una volta per accensione, appena c'e' il Wi-Fi, solo dal menu
  static bool controllato = false;
  if (!controllato && schermata == S_MENU && WiFi.status() == WL_CONNECTED && millis() > 5000) {
    controllato = true;
    controllaAggiornamenti(false);
  }
  // impostazioni cambiate dal Raspberry: la lingua si aggiorna subito, la rete si ricollega
  static String ssidUsato = imp.ssid;
  if (linguaMostrata != imp.lingua) linguaMostrata = imp.lingua;
  if (schermata == S_MENU && ssidUsato != imp.ssid) {
    ssidUsato = imp.ssid;
    if (imp.ssid.length()) { WiFi.disconnect(); WiFi.begin(imp.ssid.c_str(), imp.password.c_str()); }
  }

  tela.fillScreen(col::sfondo);
  switch (schermata) {
    case S_LINGUA: disegnaLingua(); break;
    case S_RICERCA: disegnaRicerca(); break;
    case S_RETI: disegnaReti(); break;
    case S_PASSWORD: disegnaPassword(); break;
    case S_CONNESSIONE: disegnaConnessione(); break;
    case S_MENU: disegnaMenu(); break;
    case S_INFO: disegnaInfo(); break;
    case S_IN_ARRIVO: disegnaInArrivo(); break;
    case S_NUOVO_FW: disegnaNuovoFw(); break;
    case S_ESITO_AGG: disegnaEsitoAgg(); break;
  }
  mostra();
}

#include "ota.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "grafica.h"
#include "i18n.h"

static bool avviato = false;

void otaSchermata(int percento, const char* nota) {
  tela.fillScreen(col::sfondo);
  tela.setTextDatum(textdatum_t::middle_center);
  tela.setFont(&fonts::FreeSansBold12pt7b);
  tela.setTextColor(col::testo);
  tela.drawString(tr(T_AGGIORNAMENTO), SCHERMO_L / 2, 50);
  tela.fillRoundRect(40, 85, SCHERMO_L - 80, 10, 5, col::evidenza);
  if (percento > 0) tela.fillRoundRect(40, 85, (SCHERMO_L - 80) * percento / 100, 10, 5, col::blu);
  tela.setFont(&fonts::FreeSans9pt7b);
  tela.setTextColor(col::secondario);
  tela.drawString(nota, SCHERMO_L / 2, 120);
  mostra();
}

static void avvia() {
  ArduinoOTA.setHostname("plekto");
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() { otaSchermata(0, tr(T_NON_SPEGNERE)); });
  ArduinoOTA.onProgress([](unsigned int fatto, unsigned int totale) {
    static int ultimo = -1;
    int p = totale ? (int)(100ULL * fatto / totale) : 0;
    if (p != ultimo) { ultimo = p; char s[16]; snprintf(s, sizeof s, "%d %%", p); otaSchermata(p, s); }
  });
  ArduinoOTA.onEnd([]() { otaSchermata(100, tr(T_RIAVVIO)); });
  ArduinoOTA.onError([](ota_error_t) { otaSchermata(0, tr(T_ERRORE_OTA)); delay(1500); });
  ArduinoOTA.begin();
  // l'indirizzo va anche sulla seriale: serve al PC/Raspberry per trovare la scheda
  Serial.printf("{\"t\":\"rete\",\"ip\":\"%s\",\"host\":\"plekto.local\"}\n", WiFi.localIP().toString().c_str());
  avviato = true;
}

void otaAggiorna() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!avviato) avvia();
  ArduinoOTA.handle();
}

#include "collegamento.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "impostazioni.h"
#include "input.h"
#include "config.h"

StatoRaspberry rasp;
static String riga;
static int tastoInAttesa = -1;
static bool chiestoAgg = false;
static int luci = 0;

static void inviaStatoEsp() {
  JsonDocument d;
  d["t"] = "esp";
  d["fw"] = FW_VERSION;
  d["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  d["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  d["s"] = millis() / 1000;
  d["ano"] = inputRotellaPresente();
  d["luci"] = luci;
  serializeJson(d, Serial);
  Serial.println();
}

static void comando(JsonDocument& d) {
  const char* c = d["c"] | "";
  if (!strcmp(c, "luci")) {                 // LED1 e LED2 della scheda madre, 0..100 %
    luci = constrain((int)(d["v"] | 0), 0, 100);
    analogWrite(PIN_LED1, luci * 255 / 100);
    analogWrite(PIN_LED2, luci * 255 / 100);
  } else if (!strcmp(c, "suono")) {
    tone(PIN_AUDIO, d["hz"] | 1760, d["ms"] | 120);
  } else if (!strcmp(c, "riavvia")) {
    Serial.flush();
    delay(100);
    ESP.restart();
  } else if (!strcmp(c, "agg")) {
    chiestoAgg = true;
  }
  inviaStatoEsp();
}

bool collegamentoChiestoAggiornamento() {
  bool b = chiestoAgg;
  chiestoAgg = false;
  return b;
}

void collegamentoInizio() {
  Serial.begin(115200);
  JsonDocument d;
  d["t"] = "ciao";
  d["fw"] = FW_VERSION;
  d["ver"] = imp.versione;
  serializeJson(d, Serial);
  Serial.println();
}

void collegamentoInviaImpostazioni() {
  JsonDocument d;
  d["t"] = "cfg";
  d["ver"] = imp.versione;
  d["lingua"] = imp.lingua == IT ? "it" : "en";
  d["configurato"] = imp.configurato;
  d["wifi"]["ssid"] = imp.ssid;
  d["wifi"]["pass"] = imp.password;
  serializeJson(d, Serial);
  Serial.println();
}

static void gestisci(const String& s) {
  JsonDocument d;
  if (deserializeJson(d, s)) return;
  const char* t = d["t"] | "";
  rasp.visto = true;
  rasp.ultimoMs = millis();
  if (!strcmp(t, "cfg?")) {
    collegamentoInviaImpostazioni();
  } else if (!strcmp(t, "cfg")) {
    uint32_t v = d["ver"] | 0;
    if (v > imp.versione) {            // il Raspberry ha impostazioni più recenti: le adotto
      imp.lingua = String(d["lingua"] | "it") == "en" ? EN : IT;
      imp.ssid = (const char*)(d["wifi"]["ssid"] | "");
      imp.password = (const char*)(d["wifi"]["pass"] | "");
      imp.configurato = d["configurato"] | imp.configurato;
      imp.versione = v;
      impostazioniSalva(false);
    } else if (v < imp.versione) {     // le mie sono più recenti: gliele rimando
      collegamentoInviaImpostazioni();
    }
  } else if (!strcmp(t, "cmd")) {
    comando(d);
  } else if (!strcmp(t, "esp?")) {
    inviaStatoEsp();
  } else if (!strcmp(t, "stato")) {
    rasp.wifi = d["wifi"] | false;
    rasp.ip = (const char*)(d["ip"] | "");
  }
}

void collegamentoAggiorna() {
  while (Serial.available()) {
    char c = Serial.read();
    if (riga.length() == 0 && c != '{') {           // tastiera seriale di prova (a/d ruota, spazio = centro...)
      if (c != '\r' && c != '\n') tastoInAttesa = c;
      continue;
    }
    if (c == '\n') {
      gestisci(riga);
      riga = "";
    } else if (c != '\r' && riga.length() < 512) {
      riga += c;
    }
  }
}

int collegamentoTastoEmulato() {
  int t = tastoInAttesa;
  tastoInAttesa = -1;
  return t;
}

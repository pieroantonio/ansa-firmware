#include "collegamento.h"
#include <ArduinoJson.h>
#include "impostazioni.h"

StatoRaspberry rasp;
static String riga;
static int tastoInAttesa = -1;

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

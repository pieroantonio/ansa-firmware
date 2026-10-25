// Testi dell'interfaccia in italiano e inglese.
// I font della libreria grafica sono ASCII: niente lettere accentate nei testi (si useranno font completi più avanti).
#pragma once
#include <Arduino.h>

enum Lingua : uint8_t { IT = 0, EN = 1 };

enum TestoId : uint8_t {
  T_BENVENUTO, T_SCEGLI_LINGUA, T_RETE_WIFI, T_RICERCA_RETI, T_NESSUNA_RETE, T_RIPROVA,
  T_PASSWORD, T_CONNESSIONE, T_CONNESSO, T_ERRORE_WIFI, T_SALTA,
  T_MENU, T_PIEGA, T_CARICA_FILO, T_TAGLIO_PEEK, T_LUCI, T_LINGUA, T_AGGIORNAMENTI, T_INFO,
  T_AIUTO_PASSWORD, T_AIUTO_LISTA, T_RETE_APERTA, T_SINCRONIZZATO, T_RASPBERRY, T_NON_COLLEGATO,
  T_IN_ARRIVO, T_AGGIORNAMENTO, T_NON_SPEGNERE, T_RIAVVIO, T_ERRORE_OTA,
  T_NUOVO_FW, T_AIUTO_INSTALLA, T_GIA_AGGIORNATO, T_CONTROLLO_AGG, T_ERRORE_RETE, T__N
};

static const char* const TESTI[T__N][2] = {
  {"Benvenuto", "Welcome"},
  {"Scegli la lingua", "Choose your language"},
  {"Rete Wi-Fi", "Wi-Fi network"},
  {"Cerco le reti...", "Searching networks..."},
  {"Nessuna rete trovata", "No networks found"},
  {"Riprova", "Try again"},
  {"Password", "Password"},
  {"Connessione...", "Connecting..."},
  {"Connesso", "Connected"},
  {"Password errata o rete assente", "Wrong password or network missing"},
  {"Salta", "Skip"},
  {"Ansa", "Ansa"},
  {"Piega", "Bend"},
  {"Carica filo", "Load wire"},
  {"Taglio PEEK", "PEEK cutting"},
  {"Luci", "Lights"},
  {"Lingua", "Language"},
  {"Aggiornamenti", "Updates"},
  {"Informazioni", "About"},
  {"ruota: lettera   centro: scrivi   sx: cancella   dx: OK   su: abc/123   giu: mostra",
   "turn: letter   center: type   left: delete   right: OK   up: abc/123   down: show"},
  {"ruota per scegliere, centro per confermare", "turn to choose, press center to confirm"},
  {"rete aperta", "open network"},
  {"Impostazioni condivise col Raspberry", "Settings shared with Raspberry"},
  {"Raspberry", "Raspberry"},
  {"non collegato", "not connected"},
  {"in arrivo", "coming soon"},
  {"Aggiornamento firmware", "Firmware update"},
  {"non spegnere la macchina", "do not switch off"},
  {"riavvio...", "restarting..."},
  {"aggiornamento non riuscito", "update failed"},
  {"Nuova versione", "New version"},
  {"centro: installa      sinistra: piu' tardi", "center: install      left: later"},
  {"Firmware aggiornato", "Firmware up to date"},
  {"Controllo aggiornamenti...", "Checking for updates..."},
  {"Controllo non riuscito (rete?)", "Check failed (network?)"},
};

extern Lingua linguaCorrente;
inline const char* tr(TestoId id) { return TESTI[id][linguaCorrente]; }

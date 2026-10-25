// Collegamento seriale (USB, CH340) con il Raspberry Pi: messaggi JSON, una riga per messaggio.
// Le righe che non iniziano con '{' sono testo di servizio e vengono ignorate dal Raspberry.
//
// ESP32 -> Pi   {"t":"ciao","fw":"0.1.0","ver":N}
//               {"t":"cfg","ver":N,"lingua":"it","wifi":{"ssid":"...","pass":"..."},"configurato":true}
// Pi -> ESP32   {"t":"cfg?"}                         chiede le impostazioni
//               {"t":"cfg", ...}                      impostazioni dal Pi: applicate solo se ver > locale
//               {"t":"stato","wifi":true,"ip":"..."}  stato della rete del Pi (mostrato in Informazioni)
//               {"t":"cmd","c":"luci","v":0..100}    comandi dalla pagina web: luci, suono (tipo: avvio/fatto/errore), riavvia, agg,
//                                                     lavoro (v 1/0), retainer_fatto; esp? risponde anche con i traguardi
// ESP32 -> Pi   {"t":"esp","fw":..,"ip":..,"rssi":..,"s":secondi,"ano":bool,"luci":0..100}  risposta a esp?
#pragma once
#include <Arduino.h>

struct StatoRaspberry {
  bool visto = false;        // ha mai risposto
  bool wifi = false;
  String ip;
  uint32_t ultimoMs = 0;
};
extern StatoRaspberry rasp;

void collegamentoInizio();
void collegamentoAggiorna();          // da chiamare nel loop: legge i messaggi in arrivo
void collegamentoInviaImpostazioni();
// tasti della tastiera seriale per provare l'interfaccia senza rotella (vedi input.cpp)
int collegamentoTastoEmulato();       // -1 se nessuno
// comando "agg" dal web: il loop lo legge e lancia il controllo degli aggiornamenti
bool collegamentoChiestoAggiornamento();

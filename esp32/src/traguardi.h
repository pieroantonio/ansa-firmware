// Traguardi di chi usa Plekto: ore di lavoro (tempo in cui la macchina piega), retainer fatti e ore di accensione,
// salvati nella memoria NVS. Ogni soglia raggiunta dà una medaglia (bronzo, argento, oro, platino).
// Il lavoro e i retainer finiti li segnala il ciclo di piega (o il Raspberry con i comandi "lavoro" e "retainer_fatto").
// I CONTEGGI NON SI DEVONO MAI PERDERE (Piero, 2026-10-25):
//  - stanno nella partizione NVS, namespace "traguardi": gli aggiornamenti (OTA, da internet, via USB) riscrivono solo
//    il programma. Non cambiare mai nome del namespace o delle chiavi, né la tabella delle partizioni, e non usare
//    "pio run -t erase" / esptool erase_flash su una macchina in uso;
//  - il Raspberry tiene una copia dei valori più alti e, se l'ESP32 riparte da numeri più bassi, li rimanda
//    (traguardiRipristina accetta solo valori più alti);
//  - l'azzeramento richiede la parola di conferma AZZERA ed è solo per prove al banco.
#pragma once
#include <Arduino.h>

enum TipoMedaglia : uint8_t { MED_RETAINER, MED_ORE };

struct Medaglia {
  TipoMedaglia tipo;
  uint32_t soglia;        // retainer, oppure ore
  const char* etichetta;  // scritta dentro la medaglia
  const char* nome[2];    // italiano, inglese (solo ASCII: i font del display non hanno accenti)
};

extern const Medaglia MEDAGLIE[];
extern const int N_MEDAGLIE;

void traguardiInizio();
void traguardiAggiorna();              // nel loop: conta il tempo e salva ogni 5 minuti
void traguardiLavoro(bool attivo);     // il lavoro di piega inizia / finisce
void traguardiRetainerFatto();
void traguardiAzzera();               // per le prove e prima della consegna
void traguardiRipristina(uint32_t sLavoro, uint32_t sAcceso, uint32_t retainer, uint32_t viste);   // solo verso l'alto
uint32_t traguardiViste();            // medaglie già annunciate

uint32_t traguardiSecondiLavoro();
uint32_t traguardiSecondiAcceso();
uint32_t traguardiRetainer();
uint32_t traguardiMedaglie();          // bit i = medaglia i ottenuta
float traguardiAvanzamento(int i);     // 0..1 verso la medaglia i
uint8_t traguardiLivello(int i);       // 0 bronzo, 1 argento, 2 oro, 3 platino
int traguardiNuovaMedaglia();          // indice di una medaglia appena ottenuta (una volta sola), -1 se nessuna

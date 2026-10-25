// Impostazioni persistenti (memoria NVS) condivise con il Raspberry Pi.
// Ogni modifica incrementa 'versione': nella sincronizzazione vince la versione più alta.
#pragma once
#include <Arduino.h>
#include "i18n.h"

struct Impostazioni {
  bool configurato = false;     // primo avvio completato (lingua + Wi-Fi scelti o saltati)
  Lingua lingua = IT;
  String ssid;
  String password;
  uint32_t versione = 0;
};

extern Impostazioni imp;

void impostazioniCarica();
void impostazioniSalva(bool nuovaVersione = true);   // nuovaVersione: incrementa e avvisa il Raspberry

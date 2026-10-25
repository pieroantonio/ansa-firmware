// Comandi: rotella Adafruit ANO (tramite espansore MCP23017) oppure, per le prove al banco,
// tastiera dal monitor seriale e tasto BOOT della scheda.
#pragma once
#include <Arduino.h>

enum Evento : uint8_t { EV_NIENTE, EV_ORARIO, EV_ANTIORARIO, EV_CENTRO, EV_SU, EV_GIU, EV_SINISTRA, EV_DESTRA,
                        EV_CENTRO_LUNGO };

void inputInizio();
Evento inputLeggi();          // un evento alla volta (coda interna)
bool inputRotellaPresente();

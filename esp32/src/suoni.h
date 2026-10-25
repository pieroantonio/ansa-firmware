// Suoni di Plekto sull'altoparlante della scheda madre (PIN_AUDIO, uscita PDM dell'I2S0).
// suona() non blocca: la sintesi gira in un compito FreeRTOS sul core 0.
#pragma once
#include <Arduino.h>

enum Suono { SUONO_AVVIO, SUONO_FATTO, SUONO_ERRORE };

void suoniInizio();
void suona(Suono s);

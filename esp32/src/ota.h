// Aggiornamento del firmware via Wi-Fi (ArduinoOTA, protetto da password) con barra di avanzamento sul display.
#pragma once
void otaAggiorna();   // da chiamare nel loop: si avvia da solo appena il Wi-Fi è collegato
void otaSchermata(int percento, const char* nota);   // schermata con barra di avanzamento

// Aggiornamento del firmware da internet: manifest.json nell'ultima release GitHub del repository pubblico
// ({"versione":"0.2.0","url":".../firmware.bin","sha256":"..."}). HTTPS con certificati verificati, controllo SHA-256
// prima di attivare il nuovo firmware; se non parte, il bootloader torna alla versione precedente.
#pragma once
#include <Arduino.h>

enum EsitoControllo { AGG_NUOVO, AGG_GIA_AGGIORNATO, AGG_ERRORE };

EsitoControllo aggControlla();            // bloccante (1-3 s): legge il manifest
const String& aggVersioneNuova();
bool aggInstalla();                       // bloccante: scarica, verifica, scrive e riavvia (false = errore)

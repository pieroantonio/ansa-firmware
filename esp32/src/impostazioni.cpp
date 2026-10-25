#include "impostazioni.h"
#include <Preferences.h>
#include "collegamento.h"

Impostazioni imp;
Lingua linguaCorrente = IT;
static Preferences pref;

void impostazioniCarica() {
  pref.begin("bending", true);
  imp.configurato = pref.getBool("config", false);
  imp.lingua = (Lingua)pref.getUChar("lingua", IT);
  imp.ssid = pref.getString("ssid", "");
  imp.password = pref.getString("pass", "");
  imp.versione = pref.getULong("ver", 0);
  pref.end();
  linguaCorrente = imp.lingua;
}

void impostazioniSalva(bool nuovaVersione) {
  if (nuovaVersione) imp.versione++;
  pref.begin("bending", false);
  pref.putBool("config", imp.configurato);
  pref.putUChar("lingua", imp.lingua);
  pref.putString("ssid", imp.ssid);
  pref.putString("pass", imp.password);
  pref.putULong("ver", imp.versione);
  pref.end();
  linguaCorrente = imp.lingua;
  if (nuovaVersione) collegamentoInviaImpostazioni();   // condivise subito col Raspberry
}

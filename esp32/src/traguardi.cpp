#include "traguardi.h"
#include <Preferences.h>

const Medaglia MEDAGLIE[] = {
  {MED_RETAINER, 1, "1", {"Primo retainer", "First retainer"}},
  {MED_RETAINER, 10, "10", {"Dieci anse", "Ten loops"}},
  {MED_RETAINER, 25, "25", {"Mano ferma", "Steady hand"}},
  {MED_RETAINER, 50, "50", {"Mezzo cento", "Half a hundred"}},
  {MED_RETAINER, 100, "100", {"Centurione", "Centurion"}},
  {MED_RETAINER, 250, "250", {"Maestro del filo", "Wire master"}},
  {MED_RETAINER, 500, "500", {"Leggenda", "Legend"}},
  {MED_RETAINER, 1000, "1k", {"Mille anse", "Thousand loops"}},
  {MED_ORE, 1, "1h", {"Prima ora", "First hour"}},
  {MED_ORE, 10, "10h", {"Dieci ore", "Ten hours"}},
  {MED_ORE, 50, "50h", {"Rodaggio finito", "Run-in done"}},
  {MED_ORE, 100, "100h", {"Cento ore", "Hundred hours"}},
  {MED_ORE, 250, "250h", {"Instancabile", "Tireless"}},
  {MED_ORE, 500, "500h", {"Veterana", "Veteran"}},
  {MED_ORE, 1000, "1kh", {"Mille ore", "Thousand hours"}},
};
const int N_MEDAGLIE = sizeof(MEDAGLIE) / sizeof(MEDAGLIE[0]);

static Preferences pref;
static uint32_t sLavoro = 0, sAcceso = 0, retainer = 0, viste = 0;   // viste: medaglie già annunciate
static bool lavoro = false, daSalvare = false;
static uint32_t ultimoSecondo = 0, ultimoSalvataggio = 0;
static int nuova = -1;

static void salva() {
  pref.begin("traguardi", false);
  pref.putUInt("s_lavoro", sLavoro);
  pref.putUInt("s_acceso", sAcceso);
  pref.putUInt("retainer", retainer);
  pref.putUInt("viste", viste);
  pref.end();
  daSalvare = false;
  ultimoSalvataggio = millis();
}

static uint32_t valore(const Medaglia& m) {
  return m.tipo == MED_RETAINER ? retainer : sLavoro / 3600;
}

uint32_t traguardiMedaglie() {
  uint32_t b = 0;
  for (int i = 0; i < N_MEDAGLIE; ++i)
    if (valore(MEDAGLIE[i]) >= MEDAGLIE[i].soglia) b |= 1UL << i;
  return b;
}

static void controllaNuove() {
  if (nuova >= 0) return;                      // c'è già una medaglia da annunciare
  uint32_t mancanti = traguardiMedaglie() & ~viste;
  if (!mancanti) return;
  for (int i = 0; i < N_MEDAGLIE; ++i)
    if (mancanti & (1UL << i)) { nuova = i; viste |= 1UL << i; break; }   // una alla volta
  salva();
}

void traguardiInizio() {
  pref.begin("traguardi", true);
  sLavoro = pref.getUInt("s_lavoro", 0);
  sAcceso = pref.getUInt("s_acceso", 0);
  retainer = pref.getUInt("retainer", 0);
  viste = pref.getUInt("viste", 0);
  pref.end();
  ultimoSecondo = ultimoSalvataggio = millis();
}

void traguardiAggiorna() {
  uint32_t ora = millis();
  while (ora - ultimoSecondo >= 1000) {
    ultimoSecondo += 1000;
    ++sAcceso;
    if (lavoro) ++sLavoro;
    daSalvare = true;
  }
  if (daSalvare && ora - ultimoSalvataggio > 300000UL) salva();   // ogni 5 minuti: la memoria flash non si consuma
  controllaNuove();
}

void traguardiLavoro(bool attivo) {
  if (lavoro && !attivo) salva();
  lavoro = attivo;
}

void traguardiRetainerFatto() {
  ++retainer;
  salva();
  controllaNuove();
}

void traguardiAzzera() {
  sLavoro = sAcceso = retainer = viste = 0;
  nuova = -1;
  salva();
}

void traguardiRipristina(uint32_t l, uint32_t a, uint32_t r, uint32_t v) {
  bool cambiato = l > sLavoro || a > sAcceso || r > retainer || (v | viste) != viste;
  sLavoro = max(sLavoro, l);
  sAcceso = max(sAcceso, a);
  retainer = max(retainer, r);
  viste |= v;
  if (cambiato) salva();
}

uint32_t traguardiViste() { return viste; }

uint32_t traguardiSecondiLavoro() { return sLavoro; }
uint32_t traguardiSecondiAcceso() { return sAcceso; }
uint32_t traguardiRetainer() { return retainer; }

float traguardiAvanzamento(int i) {
  return constrain((float)valore(MEDAGLIE[i]) / MEDAGLIE[i].soglia, 0.0f, 1.0f);
}

uint8_t traguardiLivello(int i) {
  int k = 0;                                   // posizione della medaglia nella sua serie
  for (int j = 0; j < i; ++j)
    if (MEDAGLIE[j].tipo == MEDAGLIE[i].tipo) ++k;
  return k < 2 ? 0 : k < 4 ? 1 : k < 6 ? 2 : 3;
}

int traguardiNuovaMedaglia() {
  int n = nuova;
  nuova = -1;
  return n;
}

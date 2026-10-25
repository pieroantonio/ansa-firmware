#include "input.h"
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include "config.h"
#include "collegamento.h"

static Adafruit_MCP23X17 mcp;
static bool rotella = false;
static QueueHandle_t coda;

static void metti(Evento e) { xQueueSend(coda, &e, 0); }

// lettura della rotella in un task a ~1 kHz: decodifica in quadratura + tasti con antirimbalzo
static void taskRotella(void*) {
  static const int8_t tab[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
  uint8_t prec = 0;
  int conta = 0;
  uint8_t tastiPrec = 0x7C;            // pull-up: 1 = rilasciato
  uint32_t premutoDa = 0;
  bool lungoInviato = false;
  for (;;) {
    uint8_t a = mcp.readGPIOA();
    uint8_t ab = ((a >> ANO_ENCA) & 1) << 1 | ((a >> ANO_ENCB) & 1);
    conta += tab[(prec << 2) | ab];
    prec = ab;
    if (conta >= ANO_TRANSIZIONI_PER_SCATTO) { metti(EV_ORARIO); conta = 0; }
    if (conta <= -ANO_TRANSIZIONI_PER_SCATTO) { metti(EV_ANTIORARIO); conta = 0; }

    uint8_t tasti = a & 0x7C;
    uint8_t premuti = (~tasti) & tastiPrec;          // fronti di discesa
    if (premuti & (1 << ANO_CENTER)) { premutoDa = millis(); lungoInviato = false; }
    if (!(tasti & (1 << ANO_CENTER)) && !lungoInviato && millis() - premutoDa > 700) {
      metti(EV_CENTRO_LUNGO); lungoInviato = true;
    }
    uint8_t rilasciati = tasti & ~tastiPrec;
    if ((rilasciati & (1 << ANO_CENTER)) && !lungoInviato) metti(EV_CENTRO);
    if (premuti & (1 << ANO_UP)) metti(EV_SU);
    if (premuti & (1 << ANO_DOWN)) metti(EV_GIU);
    if (premuti & (1 << ANO_LEFT)) metti(EV_SINISTRA);
    if (premuti & (1 << ANO_RIGHT)) metti(EV_DESTRA);
    tastiPrec = tasti;
    vTaskDelay(1);
  }
}

void inputInizio() {
  coda = xQueueCreate(32, sizeof(Evento));
  pinMode(PIN_BOOT, INPUT_PULLUP);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  rotella = mcp.begin_I2C(MCP23017_ADDR, &Wire);
  if (rotella) {
    for (uint8_t p : {ANO_ENCA, ANO_ENCB, ANO_CENTER, ANO_DOWN, ANO_RIGHT, ANO_UP, ANO_LEFT}) mcp.pinMode(p, INPUT_PULLUP);
    xTaskCreatePinnedToCore(taskRotella, "rotella", 3072, nullptr, 2, nullptr, 0);
  }
}

bool inputRotellaPresente() { return rotella; }

Evento inputLeggi() {
  // tastiera seriale di prova:  a/d ruota  w/s su/giu  q/e sinistra/destra  spazio o invio = centro  l = centro lungo
  switch (collegamentoTastoEmulato()) {
    case 'd': return EV_ORARIO;
    case 'a': return EV_ANTIORARIO;
    case 'w': return EV_SU;
    case 's': return EV_GIU;
    case 'q': return EV_SINISTRA;
    case 'e': return EV_DESTRA;
    case ' ': return EV_CENTRO;
    case 'l': return EV_CENTRO_LUNGO;
    default: break;
  }
  // tasto BOOT come "centro" (solo per le prove: GPIO0 è anche il pin di avvio)
  static bool bootPrec = true;
  bool boot = digitalRead(PIN_BOOT);
  Evento e = EV_NIENTE;
  if (bootPrec && !boot) e = EV_CENTRO;
  bootPrec = boot;
  if (e != EV_NIENTE) return e;
  if (coda && xQueueReceive(coda, &e, 0) == pdTRUE) return e;
  return EV_NIENTE;
}

#include "suoni.h"
#include "config.h"
#include <driver/i2s.h>

// Suoni "alla Apple": seno + armoniche che si spengono prima (timbro di vetro), attacco morbido, coda esponenziale,
// riverbero corto. Stesso algoritmo di strumenti/sintesi_suoni.py (lì si ascoltano in WAV).
// Uscita: I2S0 in modo PDM sul PIN_AUDIO (densità di impulsi a ~1 MHz): il MOSFET e l'altoparlante fanno da filtro.
// Tra un suono e l'altro l'I2S è spento e il pin resta basso (altoparlante senza corrente).

static const int SR = 16000, BLOCCO = 256;
static const float RAMPA = 0.03f;     // s: la componente continua della PDM sale e scende piano (niente "toc")

struct Nota { float inizio, hz, durata, vol; };
struct Melodia { const Nota* note; int n; float guadagno; };
// stessi valori di sintesi_suoni.py; il guadagno porta il picco a 0,85
static const Nota AVVIO[] = {{0.00f, 523.25f, 1.9f, 0.55f}, {0.06f, 659.26f, 1.8f, 0.45f}, {0.12f, 783.99f, 1.7f, 0.45f},
                             {0.18f, 987.77f, 1.6f, 0.35f}, {0.32f, 1046.5f, 1.4f, 0.40f}};
static const Nota FATTO[] = {{0.00f, 1174.66f, 0.9f, 0.55f}, {0.11f, 1567.98f, 1.1f, 0.60f}};
static const Nota ERRORE[] = {{0.00f, 587.33f, 0.35f, 0.65f}, {0.16f, 466.16f, 0.55f, 0.65f}};
static const Melodia MELODIE[] = {{AVVIO, 5, 0.865f}, {FATTO, 2, 1.262f}, {ERRORE, 2, 1.759f}};
static const float ARM_MUL[3] = {1, 2, 3}, ARM_AMP[3] = {1.0f, 0.28f, 0.10f}, ARM_VEL[3] = {1.0f, 2.5f, 4.0f};
static const float ATTACCO = 0.012f, CODA = 0.6f, VOLUME = 0.8f;

static float seno[1024];
static QueueHandle_t richieste;

// riverbero: 2 pettini + 1 passa-tutto (ritardi in campioni a 16 kHz)
static float pet1[557], pet2[593], pt[179];
static int i1, i2, ip;

static float riverbera(float x) {
  float a = pet1[i1], b = pet2[i2];
  pet1[i1] = x + 0.62f * a;
  pet2[i2] = x + 0.60f * b;
  i1 = (i1 + 1) % 557;
  i2 = (i2 + 1) % 593;
  float w = 0.5f * (a + b);
  float d = pt[ip];                                     // passa-tutto in forma canonica
  float v = w + 0.5f * d;
  pt[ip] = v;
  ip = (ip + 1) % 179;
  float y = d - 0.5f * v;
  return x * 0.78f + y * 0.22f;
}

static void suonaMelodia(const Melodia& m) {
  memset(pet1, 0, sizeof pet1);
  memset(pet2, 0, sizeof pet2);
  memset(pt, 0, sizeof pt);
  i1 = i2 = ip = 0;
  float fine = 0;
  for (int k = 0; k < m.n; ++k) fine = max(fine, m.note[k].inizio + m.note[k].durata);
  const int totale = (int)((fine + CODA) * SR);
  const int rampa = (int)(RAMPA * SR);
  float fase[5][3] = {}, coda[5][3], cala[5][3];       // coda esponenziale: si moltiplica per "cala" a ogni campione
  for (int k = 0; k < m.n; ++k)
    for (int h = 0; h < 3; ++h) {
      coda[k][h] = 1.0f;
      cala[k][h] = expf(-ARM_VEL[h] * 4.0f / (m.note[k].durata * SR));
    }
  int16_t buf[BLOCCO];
  // rampa iniziale: da densità 0 (pin basso) a 50 % (silenzio della PDM)
  for (int s = 0; s < rampa; s += BLOCCO) {
    for (int j = 0; j < BLOCCO; ++j) buf[j] = (int16_t)(-32767.0f * (1.0f - (float)(s + j) / rampa));
    size_t scritti;
    i2s_write(I2S_NUM_0, buf, sizeof buf, &scritti, portMAX_DELAY);
  }
  for (int s = 0; s < totale; s += BLOCCO) {
    for (int j = 0; j < BLOCCO; ++j) {
      float t = (float)(s + j) / SR, x = 0;
      for (int k = 0; k < m.n; ++k) {
        const Nota& n = m.note[k];
        float tn = t - n.inizio;
        if (tn < 0 || tn > n.durata) continue;
        float att = tn < ATTACCO ? tn / ATTACCO : 1.0f;
        for (int h = 0; h < 3; ++h) {
          float f = n.hz * ARM_MUL[h];
          if (f > SR / 2) continue;
          fase[k][h] += f / SR;
          if (fase[k][h] >= 1.0f) fase[k][h] -= 1.0f;
          x += ARM_AMP[h] * seno[(int)(fase[k][h] * 1024) & 1023] * coda[k][h] * att * n.vol;
          coda[k][h] *= cala[k][h];
        }
      }
      float y = riverbera(x) * m.guadagno * VOLUME;
      buf[j] = (int16_t)(constrain(y, -1.0f, 1.0f) * 32767.0f);
    }
    size_t scritti;
    i2s_write(I2S_NUM_0, buf, sizeof buf, &scritti, portMAX_DELAY);
  }
  for (int s = 0; s < rampa; s += BLOCCO) {             // rampa finale verso il pin basso
    for (int j = 0; j < BLOCCO; ++j) buf[j] = (int16_t)(-32767.0f * (float)(s + j) / rampa);
    size_t scritti;
    i2s_write(I2S_NUM_0, buf, sizeof buf, &scritti, portMAX_DELAY);
  }
}

static void avviaI2S() {
  i2s_config_t c = {};
  c.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_PDM);
  c.sample_rate = SR;
  c.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  c.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  c.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  c.dma_buf_count = 4;
  c.dma_buf_len = BLOCCO;
  c.tx_desc_auto_clear = false;
  i2s_driver_install(I2S_NUM_0, &c, 0, nullptr);
  i2s_pin_config_t p = {};
  p.mck_io_num = I2S_PIN_NO_CHANGE;
  p.bck_io_num = I2S_PIN_NO_CHANGE;
  p.ws_io_num = I2S_PIN_NO_CHANGE;                      // in PDM sarebbe il clock: non serve
  p.data_out_num = PIN_AUDIO;
  p.data_in_num = I2S_PIN_NO_CHANGE;
  i2s_set_pin(I2S_NUM_0, &p);
}

static void fermaI2S() {
  i2s_driver_uninstall(I2S_NUM_0);
  pinMode(PIN_AUDIO, OUTPUT);
  digitalWrite(PIN_AUDIO, LOW);
}

static void compito(void*) {
  int quale;
  for (;;) {
    if (xQueueReceive(richieste, &quale, portMAX_DELAY) != pdTRUE) continue;
    avviaI2S();
    suonaMelodia(MELODIE[quale]);
    vTaskDelay(pdMS_TO_TICKS(40));                      // svuota i buffer DMA
    fermaI2S();
  }
}

void suoniInizio() {
  pinMode(PIN_AUDIO, OUTPUT);
  digitalWrite(PIN_AUDIO, LOW);
  for (int i = 0; i < 1024; ++i) seno[i] = sinf(TWO_PI * i / 1024);
  richieste = xQueueCreate(1, sizeof(int));
  xTaskCreatePinnedToCore(compito, "suoni", 4096, nullptr, 2, nullptr, 0);
}

void suona(Suono s) {
  int q = (int)s;
  xQueueOverwrite(richieste, &q);                       // l'ultimo suono chiesto vince
}


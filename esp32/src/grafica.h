// Display ST7789 della HW-656A (LovyanGFX) e piccoli componenti grafici in stile Apple (modalità scura).
#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 pannello;
  lgfx::Bus_SPI bus;
  lgfx::Light_PWM luce;
 public:
  LGFX() {
    { auto c = bus.config();
      c.spi_host = VSPI_HOST; c.spi_mode = 0; c.freq_write = 40000000; c.freq_read = 16000000;
      c.pin_sclk = PIN_LCD_SCLK; c.pin_mosi = PIN_LCD_MOSI; c.pin_miso = -1; c.pin_dc = PIN_LCD_DC;
      bus.config(c); pannello.setBus(&bus); }
    { auto c = pannello.config();
      c.pin_cs = PIN_LCD_CS; c.pin_rst = PIN_LCD_RST; c.pin_busy = -1;
      c.panel_width = 170; c.panel_height = 320; c.offset_x = 35; c.offset_y = 0;
      c.readable = false; c.invert = true; c.rgb_order = false; c.dlen_16bit = false; c.bus_shared = false;
      pannello.config(c); }
    { auto c = luce.config();
      c.pin_bl = PIN_LCD_BL; c.invert = LCD_BL_INVERT; c.freq = 44100; c.pwm_channel = 7;
      luce.config(c); pannello.setLight(&luce); }
    setPanel(&pannello);
  }
};

// palette "modalità scura" dei sistemi Apple
namespace col {
static constexpr uint32_t sfondo = 0x000000, scheda = 0x1C1C1E, evidenza = 0x2C2C2E, separatore = 0x38383A;
static constexpr uint32_t testo = 0xFFFFFF, secondario = 0x8E8E93, blu = 0x0A84FF, verde = 0x30D158;
static constexpr uint32_t arancio = 0xFF9F0A, rosso = 0xFF453A, viola = 0xBF5AF2, azzurro = 0x64D2FF;
static constexpr uint32_t giallo = 0xFFD60A, grigio = 0x8E8E93, rosa = 0xFF375F;
}

constexpr int SCHERMO_L = 320, SCHERMO_A = 170;
extern LGFX lcd;
extern LGFX_Sprite tela;      // tutto si disegna qui e poi si copia sul display: niente sfarfallio

void graficaInizio();
void mostra();                                                     // copia la tela sul display
float avvicina(float attuale, float meta, float k = 0.28f);       // animazioni morbide
void intestazione(const char* titolo, bool wifi, bool indietro);
void iconaTessera(int x, int y, uint32_t colore, const char* lettera);
void iconaWifi(int cx, int cy, int livello, uint32_t colore);      // livello 0..3
void lucchetto(int x, int y, uint32_t colore);
void rotellina(int cx, int cy, int r, float fase, uint32_t colore); // indicatore di attesa
void spunta(int cx, int cy, float avanzamento, uint32_t colore);    // segno di conferma animato
void barraAiuto(const char* testo);
void bandiera(int x, int y, int w, int h, bool italiana);         // tricolore o Union Jack con angoli arrotondati
void logoFilo(int x, int y, int h, float frazione, float spessore, uint32_t colore);   // simbolo di Ansa (filo a omega)
void scrittaAnsa(int x, int y, float alfa, uint32_t colore);       // scritta "ansa" (maschera 8 bit) su sfondo nero
void animazioneAvvio();                                           // il filo si piega, poi compare "ansa" (~2 s)

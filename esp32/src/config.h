// Configurazione hardware della scheda HW-656A e dei comandi.
// I pin del display sono letti dalla serigrafia della scheda (foto di Piero, 2026-10-23).
#pragma once
#include <Arduino.h>

// ---------- display ST7789 1,9" 170x320 (montato in orizzontale sul fronte della base)
constexpr int PIN_LCD_SCLK = 18;
constexpr int PIN_LCD_MOSI = 23;
constexpr int PIN_LCD_CS   = 15;
constexpr int PIN_LCD_DC   = 2;    // "RS" sulla serigrafia (è anche il LED blu della scheda)
constexpr int PIN_LCD_RST  = 4;
constexpr int PIN_LCD_BL   = 32;   // retroilluminazione (LEDK)
constexpr bool LCD_BL_INVERT = false;  // ⚠ se il display resta spento all'avvio, mettere true

// ---------- bus I2C (verso l'espansore MCP23017, che ora sta sulla scheda madre)
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;
constexpr uint8_t MCP23017_ADDR = 0x20;
constexpr int PIN_MCP_INT = 39;    // INTA dell'espansore (solo ingresso, pull-up sulla scheda madre)

// ---------- macchina: pin dell'ESP32 -> segnali della scheda madre (adattatore passivo + cavo IDC 2x13)
// Fonte: cad/kicad/adattatore_esp32/genera_adattatore.py. TMC_TX/TMC_UART non collegati (pin finiti).
constexpr int PIN_X_STEP = 25, PIN_X_DIR = 26;       // feeder
constexpr int PIN_Y_STEP = 27, PIN_Y_DIR = 14;       // testa di piega
constexpr int PIN_DRV_EN = 16;                       // basso = driver accesi (pull-up 10k: spenti al boot)
constexpr int PIN_SOL    = 12;                       // solenoide (strapping: il 100k sul gate lo tiene basso)
constexpr int PIN_AUX    = 13;
constexpr int PIN_SERVO  = 33;
constexpr int PIN_AUDIO  = 19;
constexpr int PIN_LED1   = 5,  PIN_LED2 = 17;
constexpr int PIN_PROBE  = 34, PIN_ENDWIRE = 35, PIN_HOME = 36;   // solo ingressi

// ---------- rotella Adafruit ANO collegata all'espansore MCP23017 (porta A)
// Cablaggio fissato dalla scheda madre (J23, JST XH 9 poli). COMA e COMB vanno a massa.
constexpr uint8_t ANO_ENCA   = 0;   // GPA0
constexpr uint8_t ANO_ENCB   = 1;   // GPA1
constexpr uint8_t ANO_CENTER = 2;   // GPA2  (SW1)
constexpr uint8_t ANO_DOWN   = 3;   // GPA3  (SW2)
constexpr uint8_t ANO_RIGHT  = 4;   // GPA4  (SW3)
constexpr uint8_t ANO_UP     = 5;   // GPA5  (SW4)
constexpr uint8_t ANO_LEFT   = 6;   // GPA6  (SW5)
constexpr int ANO_TRANSIZIONI_PER_SCATTO = 4;   // ⚠ da tarare sulla rotella vera (2 o 4)

// ---------- tasto BOOT della scheda: usato come "OK" quando la rotella non c'è (prove al banco)
constexpr int PIN_BOOT = 0;

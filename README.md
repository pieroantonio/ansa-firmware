# Ansa – firmware

Firmware di **Ansa**, piegafilo CNC per retainer ortodontici progettati in digitale.

| Cartella | Cosa contiene |
|---|---|
| `esp32/` | firmware della scheda ESP32 HW-656A (display 1,9" ST7789, rotella Adafruit ANO): interfaccia, Wi-Fi, aggiornamenti |
| `raspberry/` | ponte Raspberry Pi ⇄ ESP32 (impostazioni condivise via USB) |

## Compilare e caricare (PlatformIO)

```
cd esp32
copy segreti.ini.esempio segreti.ini      (poi scegliere la password OTA)
pio run -e hw656a -t upload               via cavo USB (tenere premuto BOOT durante "Connecting...")
pio run -e hw656a_ota -t upload           via Wi-Fi, scheda in rete locale (ansa.local)
```

## Aggiornamenti da internet

All'accensione, appena collegata al Wi-Fi, la scheda legge `manifest.json` dall'ultima release di questo repository:

```json
{"versione": "0.2.0", "url": "https://github.com/pieroantonio/ansa-firmware/releases/download/v0.2.0/firmware.bin",
 "sha256": "<impronta SHA-256 di firmware.bin>"}
```

Se la versione è più nuova chiede conferma sul display; scarica in HTTPS (certificati verificati), controlla lo
SHA-256 e solo allora attiva il nuovo firmware. Se il nuovo firmware non parte, torna alla versione precedente.

⚠ Dispositivo in sviluppo per uso in studio: ogni retainer va verificato sul modello prima dell'uso clinico.

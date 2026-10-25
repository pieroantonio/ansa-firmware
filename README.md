# Plekto – firmware

Firmware di **Plekto**, piegafilo CNC per retainer ortodontici progettati in digitale.

| Cartella | Cosa contiene |
|---|---|
| `esp32/` | firmware della scheda ESP32 HW-656A (display 1,9" ST7789, rotella Adafruit ANO): interfaccia, Wi-Fi, aggiornamenti |
| `raspberry/` | software del Raspberry Pi 4: pagina web per progettare il retainer dagli STL, controllo dell'ESP32 via USB, aggiornamenti |

## Compilare e caricare (PlatformIO)

```
cd esp32
copy segreti.ini.esempio segreti.ini      (poi scegliere la password OTA)
pio run -e hw656a -t upload               via cavo USB (tenere premuto BOOT durante "Connecting...")
pio run -e hw656a_ota -t upload           via Wi-Fi, scheda in rete locale (plekto.local)
```

## Aggiornamenti da internet

All'accensione, appena collegata al Wi-Fi, la scheda legge `manifest.json` dall'ultima release di questo repository:

```json
{"versione": "0.2.0", "url": "https://github.com/pieroantonio/plekto-firmware/releases/download/v0.2.0/firmware.bin",
 "sha256": "<impronta SHA-256 di firmware.bin>"}
```

Se la versione è più nuova chiede conferma sul display; scarica in HTTPS (certificati verificati), controlla lo
SHA-256 e solo allora attiva il nuovo firmware. Se il nuovo firmware non parte, torna alla versione precedente.

## Raspberry Pi 4: pagina web

```
sudo git clone https://github.com/pieroantonio/plekto-firmware /opt/plekto
sudo bash /opt/plekto/raspberry/installa.sh
```

Poi aprire `http://<nome-del-raspberry>.local` da un computer dello studio:

- **Progetta**: trascinare gli STL delle arcate (in millimetri, in occlusione se sono due), scegliere il retainer e
  cliccare il dente di inizio, il dente di fine e un punto centrale. Il retainer è piano. La pagina controlla i
  contatti con l'antagonista e propone la correzione.
- **Macchina**: stato dell'ESP32, luci, suono, lingua, aggiornamenti del firmware, riavvio.
- **Sistema**: aggiornamento del software del Raspberry dai tag di questo repository, con ritorno automatico alla
  versione precedente se qualcosa non parte.

Prova senza scansioni: `python raspberry/plekto/prova_sintetica.py` crea due arcate sintetiche e progetta un retainer.

⚠ Dispositivo in sviluppo per uso in studio: ogni retainer va verificato sul modello prima dell'uso clinico.

#!/bin/bash
# Primo avvio dell'immagine di Plekto (lanciato da cloud-init, vedi user-data): installa tutto senza internet.
# Il pacchetto plekto.tar.gz nella partizione di avvio contiene il programma, le librerie Python per arm64 e three.js.
# La rete Wi-Fi arriva dopo, dall'ESP32 via USB (stessa rete impostata sul display della macchina).
set -euo pipefail
P=/boot/firmware/plekto
exec >>/var/log/plekto-primo-avvio.log 2>&1
echo "== $(date) primo avvio"
tar -xzf "$P/plekto.tar.gz" -C /opt
ln -sfn "/opt/plekto-versioni/$(cat "$P/VERSIONE")" /opt/plekto
mkdir -p /var/lib/plekto && chmod 700 /var/lib/plekto
bash /opt/plekto/raspberry/immagine/attiva.sh
rm -rf "$P"                                                    # libera la partizione di avvio
echo "== fatto"

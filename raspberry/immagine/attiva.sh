#!/bin/bash
# Attiva Plekto (servizi, Wi-Fi sbloccato con il paese IT). Usato dal primo avvio dell'immagine e da installa.sh.
set -euo pipefail
cp /opt/plekto/raspberry/plekto.service /opt/plekto/raspberry/plekto-aggiorna.service /etc/systemd/system/
raspi-config nonint do_wifi_country IT 2>/dev/null || true     # senza paese il Wi-Fi resta spento
rfkill unblock wifi 2>/dev/null || true
systemctl daemon-reload
systemctl enable --now plekto
echo "Plekto è pronto: http://$(hostname).local"

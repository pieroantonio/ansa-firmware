#!/bin/bash
# Installa il software di Plekto su un Raspberry Pi 4 (Raspberry Pi OS Bookworm, 64 bit).
# Prima volta:
#   sudo git clone https://github.com/pieroantonio/plekto-firmware /opt/plekto
#   sudo bash /opt/plekto/raspberry/installa.sh
# Poi la pagina è su http://<nome-del-raspberry>.local e gli aggiornamenti si fanno dalla scheda "Sistema".
set -euo pipefail
REPO=/opt/plekto
THREE=0.160.0

[ "$(id -u)" -eq 0 ] || { echo "Lanciare con sudo"; exit 1; }
apt-get update
apt-get install -y git python3-venv python3-pip libspatialindex-dev

python3 -m venv /opt/plekto-venv
/opt/plekto-venv/bin/pip install --upgrade pip
/opt/plekto-venv/bin/pip install -r "$REPO/raspberry/requirements.txt"
mkdir -p /var/lib/plekto
chmod 700 /var/lib/plekto

# copia locale di three.js: la pagina funziona anche se lo studio non ha internet
D="$REPO/raspberry/plekto/static/vendor"
for f in build/three.module.js examples/jsm/loaders/STLLoader.js examples/jsm/controls/OrbitControls.js \
         examples/jsm/utils/BufferGeometryUtils.js; do
  mkdir -p "$D/$(dirname "$f")"
  curl -fsSL "https://cdn.jsdelivr.net/npm/three@$THREE/$f" -o "$D/$f"
done

# il vecchio ponte (bending_ponte.py) ora è dentro il server
systemctl disable --now bending-ponte 2>/dev/null || true
rm -f /etc/systemd/system/bending-ponte.service

cp "$REPO/raspberry/plekto.service" "$REPO/raspberry/plekto-aggiorna.service" /etc/systemd/system/
systemctl daemon-reload
systemctl enable --now plekto
sleep 3
systemctl --no-pager status plekto | head -5
echo
echo "Plekto è pronto: http://$(hostname).local  oppure  http://$(hostname -I | awk '{print $1}')"

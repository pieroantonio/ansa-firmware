#!/bin/bash
# Installa Plekto su un Raspberry Pi 4 con Raspberry Pi OS (Trixie, 64 bit) già avviato e collegato a internet.
# In alternativa c'è l'immagine pronta (raspberry/immagine/), che non ha bisogno di internet al primo avvio.
#   curl -fsSL https://raw.githubusercontent.com/pieroantonio/plekto-firmware/main/raspberry/installa.sh | sudo bash
# Poi la pagina è su http://plekto.local e gli aggiornamenti si fanno dalla scheda "Sistema".
set -euo pipefail
REPO=pieroantonio/plekto-firmware
THREE=0.160.0

[ "$(id -u)" -eq 0 ] || { echo "Lanciare con sudo"; exit 1; }
apt-get update
apt-get install -y python3-pip curl

# ultima versione pubblicata (tag vX.Y.Z)
TAG=$(curl -fsSL "https://api.github.com/repos/$REPO/tags?per_page=100" | python3 -c '
import json, re, sys
t = [x["name"] for x in json.load(sys.stdin) if re.fullmatch(r"v\d+\.\d+\.\d+", x["name"])]
print(max(t, key=lambda v: tuple(map(int, v[1:].split(".")))))')
DEST=/opt/plekto-versioni/$TAG
rm -rf "$DEST" && mkdir -p "$DEST"
curl -fsSL "https://github.com/$REPO/archive/refs/tags/$TAG.tar.gz" | tar -xz --strip-components=1 -C "$DEST"
echo "$TAG" > "$DEST/VERSIONE"
ln -sfn "$DEST" /opt/plekto

python3 -m pip install --quiet --break-system-packages --target /opt/plekto-lib -r /opt/plekto/raspberry/requirements.txt
mkdir -p /var/lib/plekto && chmod 700 /var/lib/plekto

# copia locale di three.js: la pagina funziona anche se lo studio non ha internet
for f in build/three.module.js examples/jsm/loaders/STLLoader.js examples/jsm/controls/OrbitControls.js \
         examples/jsm/utils/BufferGeometryUtils.js; do
  mkdir -p "/opt/plekto-vendor/$(dirname "$f")"
  curl -fsSL "https://cdn.jsdelivr.net/npm/three@$THREE/$f" -o "/opt/plekto-vendor/$f"
done

bash /opt/plekto/raspberry/immagine/attiva.sh

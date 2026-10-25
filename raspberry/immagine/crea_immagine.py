"""Crea l'immagine della scheda SD del Raspberry di Plekto partendo da Raspberry Pi OS Lite 64 bit (Trixie, cloud-init).

Funziona su Windows senza Linux: tocca solo la partizione di avvio (FAT32), dove mette
- user-data e network-config (cloud-init): nome "plekto", utente piero con chiave SSH, SSH acceso;
- plekto/plekto.tar.gz: il programma alla versione indicata, le librerie Python per arm64 (Python 3.13) e three.js;
- plekto/primo-avvio.sh: al primo avvio installa tutto in /opt senza bisogno di internet.

Uso (serve pyfatfs + setuptools<81):
  python crea_immagine.py --base raspios_lite_arm64.img.xz --tag v0.2.2 --chiave %USERPROFILE%/.ssh/id_ed25519_plekto.pub
                          --uscita plekto-v0.2.2.img.xz [--lavoro cartella_temporanea]
"""
import argparse
import io
import lzma
import os
import shutil
import struct
import subprocess
import sys
import tarfile
import urllib.request
import zipfile

QUI = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(QUI, '..', '..'))
THREE = 'https://cdn.jsdelivr.net/npm/three@0.160.0/'
THREE_FILE = ['build/three.module.js', 'examples/jsm/loaders/STLLoader.js', 'examples/jsm/controls/OrbitControls.js',
              'examples/jsm/utils/BufferGeometryUtils.js']
PYTHON_PI = '3.13'                     # Python di Raspberry Pi OS Trixie


def decomprimi(base, img):
    print('decomprimo', base)
    with lzma.open(base) as src, open(img, 'wb') as dst:
        shutil.copyfileobj(src, dst, 16 << 20)


def partizione_avvio(img):
    """Inizio (byte) della prima partizione (FAT32 di avvio) dalla tabella MBR."""
    with open(img, 'rb') as f:
        mbr = f.read(512)
    assert mbr[510:512] == b'\x55\xaa', 'MBR non valido'
    tipo = mbr[446 + 4]
    inizio = struct.unpack('<I', mbr[446 + 8:446 + 12])[0]
    assert tipo in (0x0b, 0x0c), 'la prima partizione non è FAT32 (tipo %#x)' % tipo
    return inizio * 512


def pacchetto(tag, lavoro):
    """plekto.tar.gz: plekto-versioni/<tag>/ (repository al tag), plekto-lib/ (librerie), plekto-vendor/ (three.js)."""
    radice = os.path.join(lavoro, 'pacchetto')
    shutil.rmtree(radice, ignore_errors=True)
    versione = os.path.join(radice, 'plekto-versioni', tag)
    os.makedirs(versione)
    archivio = subprocess.run(['git', '-C', REPO, 'archive', '--format=tar', tag], capture_output=True, check=True).stdout
    with tarfile.open(fileobj=io.BytesIO(archivio)) as t:
        t.extractall(versione, filter='data')
    open(os.path.join(versione, 'VERSIONE'), 'w', newline='\n').write(tag + '\n')

    ruote = os.path.join(lavoro, 'ruote')
    shutil.rmtree(ruote, ignore_errors=True)
    print('scarico le librerie per arm64 / Python', PYTHON_PI)
    subprocess.run([sys.executable, '-m', 'pip', 'download', '--quiet', '--only-binary=:all:', '--dest', ruote,
                    '--platform', 'manylinux_2_28_aarch64', '--platform', 'manylinux_2_17_aarch64',
                    '--platform', 'manylinux2014_aarch64', '--platform', 'linux_aarch64',
                    '--python-version', PYTHON_PI, '--implementation', 'cp',
                    '-r', os.path.join(versione, 'raspberry', 'requirements.txt')], check=True)
    lib = os.path.join(radice, 'plekto-lib')
    for w in sorted(os.listdir(ruote)):
        print('   ', w)
        with zipfile.ZipFile(os.path.join(ruote, w)) as z:          # una wheel è uno zip: basta estrarla
            z.extractall(lib)

    vendor = os.path.join(radice, 'plekto-vendor')
    for f in THREE_FILE:
        dest = os.path.join(vendor, f)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with urllib.request.urlopen(THREE + f, timeout=60) as r, open(dest, 'wb') as d:
            d.write(r.read())

    uscita = os.path.join(lavoro, 'plekto.tar.gz')
    with tarfile.open(uscita, 'w:gz', compresslevel=6) as t:
        for nome in ('plekto-versioni', 'plekto-lib', 'plekto-vendor'):
            t.add(os.path.join(radice, nome), arcname=nome, filter=lambda i: (setattr(i, 'uid', 0), setattr(i, 'gid', 0),
                                                                             setattr(i, 'uname', 'root'),
                                                                             setattr(i, 'gname', 'root'), i)[-1])
    print('pacchetto %.1f MB' % (os.path.getsize(uscita) / 1e6))
    return uscita, versione


def scrivi_avvio(img, offset, file):
    from pyfatfs.PyFatFS import PyFatFS
    fs = PyFatFS(img, offset=offset, preserve_case=True)
    try:
        if not fs.exists('/plekto'):
            fs.makedir('/plekto')
        for dest, dati in file.items():
            if fs.exists(dest):
                fs.remove(dest)
            with fs.openbin(dest, 'w') as f:
                f.write(dati)
            print('  ', dest, len(dati), 'byte')
    finally:
        fs.close()
    fs = PyFatFS(img, offset=offset, read_only=True)    # rilettura: i file devono essere identici
    try:
        for dest, dati in file.items():
            with fs.openbin(dest, 'r') as f:
                assert f.read() == dati, 'file diverso dopo la scrittura: ' + dest
        print('   rilettura dalla partizione: tutti i file identici')
    finally:
        fs.close()


def comprimi(img, uscita):
    print('comprimo', uscita)
    with open(img, 'rb') as src, lzma.open(uscita, 'wb', preset=1) as dst:
        shutil.copyfileobj(src, dst, 16 << 20)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--base', required=True)
    ap.add_argument('--tag', required=True)
    ap.add_argument('--chiave', required=True, help='chiave SSH pubblica (.pub) da autorizzare')
    ap.add_argument('--uscita', required=True)
    ap.add_argument('--lavoro', default='lavoro_immagine')
    a = ap.parse_args()
    os.makedirs(a.lavoro, exist_ok=True)
    chiave = open(a.chiave, encoding='utf-8').read().strip()
    assert chiave.startswith('ssh-') and 'PRIVATE' not in chiave, 'serve la chiave PUBBLICA (.pub)'

    tgz, versione = pacchetto(a.tag, a.lavoro)
    leggi = lambda *p: open(os.path.join(versione, 'raspberry', 'immagine', *p), 'rb').read()
    user_data = leggi('user-data.modello').decode('utf-8').replace('{CHIAVE_SSH}', chiave).encode('utf-8')

    img = os.path.join(a.lavoro, 'plekto.img')
    decomprimi(a.base, img)
    offset = partizione_avvio(img)
    print('partizione di avvio a', offset, 'byte')
    scrivi_avvio(img, offset, {
        '/user-data': user_data,
        '/network-config': leggi('network-config'),
        '/plekto/primo-avvio.sh': leggi('primo-avvio.sh'),
        '/plekto/VERSIONE': (a.tag + '\n').encode(),
        '/plekto/plekto.tar.gz': open(tgz, 'rb').read(),
    })
    comprimi(img, a.uscita)
    os.remove(img)
    print('immagine pronta: %s (%.0f MB)' % (a.uscita, os.path.getsize(a.uscita) / 1e6))


if __name__ == '__main__':
    main()

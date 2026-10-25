"""Aggiornamento del software del Raspberry da GitHub (stesso repository e stesse versioni del firmware ESP32).

Disposizione sul Raspberry (vedi immagine/ e installa.sh):
    /opt/plekto-versioni/vX.Y.Z/   una cartella per versione (contenuto del repository a quel tag)
    /opt/plekto                    collegamento alla versione in uso
    /opt/plekto-lib                librerie Python (PYTHONPATH dei servizi)
- controlla(): legge i tag "vX.Y.Z" dall'API di GitHub e li confronta con la versione installata.
- installa(): eseguito come servizio a parte (plekto-aggiorna.service, lanciato dalla pagina web) perché deve
  riavviare il server: scarica l'archivio del tag, lo estrae in una cartella nuova, sposta il collegamento e
  riavvia "plekto". Se il server nuovo non risponde entro 90 s rimette il collegamento alla versione di prima.
  Non serve git. Se cambiano le librerie (requirements.txt) le installa con pip in una cartella nuova.
Uso a mano:  sudo python3 aggiorna.py [controlla|installa]
"""
import io
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import time
import urllib.request

REPO_GITHUB = 'pieroantonio/plekto-firmware'
QUI = os.path.dirname(os.path.abspath(__file__))
RADICE = os.path.abspath(os.path.join(QUI, '..', '..'))          # cartella della versione in uso
COLLEGAMENTO = '/opt/plekto'
VERSIONI = '/opt/plekto-versioni'
LIB = '/opt/plekto-lib'
DATI = os.environ.get('PLEKTO_DATI', '/var/lib/plekto')
ESITO = os.path.join(DATI, 'aggiornamento.json')
SALUTE = 'http://127.0.0.1/api/salute'


def _num(v):
    return tuple(int(x) for x in re.findall(r'\d+', v)[:3])


def _scarica(url, timeout=30):
    req = urllib.request.Request(url, headers={'User-Agent': 'plekto-aggiorna', 'Accept': 'application/vnd.github+json'})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read()


def versione_installata():
    try:
        with open(os.path.join(RADICE, 'VERSIONE'), encoding='utf-8') as f:
            return f.read().strip()
    except OSError:
        pass
    r = subprocess.run(['git', '-C', RADICE, 'describe', '--tags', '--abbrev=0'], capture_output=True, text=True)
    return r.stdout.strip() if r.returncode == 0 else 'sviluppo'


def ultima_versione():
    tag = [t['name'] for t in json.loads(_scarica('https://api.github.com/repos/%s/tags?per_page=100' % REPO_GITHUB))]
    tag = [t for t in tag if re.fullmatch(r'v\d+\.\d+\.\d+', t)]
    return max(tag, key=_num) if tag else None


def controlla():
    attuale = versione_installata()
    ultima = ultima_versione()
    nuova = bool(ultima) and (attuale == 'sviluppo' or _num(ultima) > _num(attuale))
    return {'attuale': attuale, 'ultima': ultima, 'nuova': nuova}


def _scrivi_esito(**d):
    os.makedirs(DATI, exist_ok=True)
    d['quando'] = time.strftime('%Y-%m-%d %H:%M:%S')
    with open(ESITO, 'w', encoding='utf-8') as f:
        json.dump(d, f, ensure_ascii=False)


def _risponde(versione, attesa=90):
    fine = time.time() + attesa
    while time.time() < fine:
        try:
            with urllib.request.urlopen(SALUTE, timeout=3) as r:
                if json.load(r).get('versione') == versione:
                    return True
        except Exception:
            pass
        time.sleep(3)
    return False


def _estrai(tag):
    """Scarica l'archivio del tag e lo estrae in VERSIONI/tag (senza la cartella radice dell'archivio)."""
    dati = _scarica('https://github.com/%s/archive/refs/tags/%s.tar.gz' % (REPO_GITHUB, tag), timeout=300)
    dest = os.path.join(VERSIONI, tag)
    tmp = dest + '.tmp'
    shutil.rmtree(tmp, ignore_errors=True)
    os.makedirs(tmp)
    with tarfile.open(fileobj=io.BytesIO(dati), mode='r:gz') as t:
        for m in t.getmembers():
            parti = m.name.split('/', 1)
            if len(parti) < 2 or not parti[1] or m.issym() or m.islnk() or '..' in parti[1].split('/'):
                continue
            m.name = parti[1]
            t.extract(m, tmp)
    with open(os.path.join(tmp, 'VERSIONE'), 'w', encoding='utf-8') as f:
        f.write(tag + '\n')
    shutil.rmtree(dest, ignore_errors=True)
    os.replace(tmp, dest)
    return dest


def _librerie(nuova):
    """Se requirements.txt è cambiato, installa le librerie in una cartella nuova e la mette al posto di LIB."""
    vecchio, nuovo = (os.path.join(d, 'raspberry', 'requirements.txt') for d in (RADICE, nuova))
    try:
        if open(vecchio, encoding='utf-8').read() == open(nuovo, encoding='utf-8').read():
            return
    except OSError:
        pass
    tmp = LIB + '.nuova'
    shutil.rmtree(tmp, ignore_errors=True)
    subprocess.run([sys.executable, '-m', 'pip', 'install', '--quiet', '--break-system-packages', '--target', tmp, '-r', nuovo],
                   check=True, timeout=1800)
    shutil.rmtree(LIB + '.vecchia', ignore_errors=True)
    if os.path.exists(LIB):
        os.replace(LIB, LIB + '.vecchia')
    os.replace(tmp, LIB)


def _punta(cartella):
    tmp = COLLEGAMENTO + '.tmp'
    if os.path.lexists(tmp):
        os.remove(tmp)
    os.symlink(cartella, tmp)
    os.replace(tmp, COLLEGAMENTO)
    for nome in ('plekto.service', 'plekto-aggiorna.service'):          # i servizi possono cambiare tra versioni
        shutil.copy(os.path.join(cartella, 'raspberry', nome), os.path.join('/etc/systemd/system', nome))
    subprocess.run(['systemctl', 'daemon-reload'], check=True, timeout=60)
    subprocess.run(['systemctl', 'restart', 'plekto'], check=True, timeout=60)


def installa():
    c = controlla()
    if not c['nuova']:
        _scrivi_esito(esito='gia_aggiornato', versione=c['attuale'])
        return
    prima = os.path.realpath(COLLEGAMENTO)
    _scrivi_esito(esito='in_corso', versione=c['ultima'])
    try:
        nuova = _estrai(c['ultima'])
        _librerie(nuova)
        _punta(nuova)
        if _risponde(c['ultima']):
            _scrivi_esito(esito='ok', versione=c['ultima'], prima=c['attuale'])
            for d in os.listdir(VERSIONI):                     # tengo solo la versione nuova e quella di prima
                p = os.path.join(VERSIONI, d)
                if p not in (nuova, prima) and os.path.isdir(p):
                    shutil.rmtree(p, ignore_errors=True)
            return
        errore = 'il server nuovo non risponde'
    except Exception as e:
        errore = str(e)
    try:                                                       # torno alla versione di prima
        if os.path.exists(LIB + '.vecchia'):
            shutil.rmtree(LIB, ignore_errors=True)
            os.replace(LIB + '.vecchia', LIB)
        _punta(prima)
    except Exception as e:
        errore += ' | ritorno non riuscito: %s' % e
    _scrivi_esito(esito='errore', versione=c['ultima'], errore=errore, tornato_a=c['attuale'])


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'installa':
        installa()
    else:
        print(json.dumps(controlla(), ensure_ascii=False))

"""Aggiornamento del software del Raspberry da GitHub (stesso repository e stesse versioni del firmware ESP32).

Il Raspberry tiene una copia git del repository pubblico in /opt/plekto. Le versioni sono i tag "vX.Y.Z".
- controlla(): legge i tag remoti (git ls-remote, nessun download) e li confronta con la versione installata.
- installa(): eseguito come servizio a parte (plekto-aggiorna.service, lanciato dalla pagina web) perché deve
  riavviare il server: scarica il tag nuovo, aggiorna le librerie Python, riavvia "plekto" e controlla che risponda.
  Se il server nuovo non risponde entro 90 s torna alla versione di prima.
Uso a mano:  sudo python3 aggiorna.py [controlla|installa]
"""
import json
import os
import re
import subprocess
import sys
import time
import urllib.request

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
DATI = os.environ.get('PLEKTO_DATI', '/var/lib/plekto')
ESITO = os.path.join(DATI, 'aggiornamento.json')
VENV_PIP = '/opt/plekto-venv/bin/pip'
SALUTE = 'http://127.0.0.1/api/salute'


def _git(*a, **k):
    return subprocess.run(['git', '-C', REPO] + list(a), capture_output=True, text=True, timeout=k.get('timeout', 60))


def _num(v):
    return tuple(int(x) for x in re.findall(r'\d+', v)[:3])


def versione_installata():
    r = _git('describe', '--tags', '--abbrev=0')
    return r.stdout.strip() if r.returncode == 0 else 'sviluppo'


def ultima_versione():
    r = _git('ls-remote', '--tags', '--refs', 'origin', 'v*', timeout=20)
    if r.returncode != 0:
        raise RuntimeError(r.stderr.strip() or 'git ls-remote non riuscito')
    tag = [riga.split('refs/tags/')[1] for riga in r.stdout.splitlines() if 'refs/tags/v' in riga]
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


def _passa_a(rif):
    r = _git('checkout', '--force', '--detach', rif)
    if r.returncode != 0:
        raise RuntimeError(r.stderr.strip())
    req = os.path.join(REPO, 'raspberry', 'requirements.txt')
    if os.path.exists(VENV_PIP) and os.path.exists(req):
        subprocess.run([VENV_PIP, 'install', '--quiet', '-r', req], check=True, timeout=1800)
    subprocess.run(['systemctl', 'restart', 'plekto'], check=True, timeout=60)


def installa():
    c = controlla()
    if not c['nuova']:
        _scrivi_esito(esito='gia_aggiornato', versione=c['attuale'])
        return
    prima = _git('rev-parse', 'HEAD').stdout.strip()
    _scrivi_esito(esito='in_corso', versione=c['ultima'])
    try:
        r = _git('fetch', '--force', 'origin', 'tag', c['ultima'], timeout=300)
        if r.returncode != 0:
            raise RuntimeError(r.stderr.strip())
        _passa_a(c['ultima'])
        if _risponde(c['ultima']):
            _scrivi_esito(esito='ok', versione=c['ultima'], prima=c['attuale'])
            return
        errore = 'il server nuovo non risponde'
    except Exception as e:
        errore = str(e)
    try:                                              # torno alla versione di prima
        _passa_a(prima)
    except Exception as e:
        errore += ' | ritorno non riuscito: %s' % e
    _scrivi_esito(esito='errore', versione=c['ultima'], errore=errore, tornato_a=c['attuale'])


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'installa':
        installa()
    else:
        print(json.dumps(controlla(), ensure_ascii=False))

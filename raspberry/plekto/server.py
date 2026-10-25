"""Server web di Plekto sul Raspberry Pi 4: progetto del retainer dal browser e controllo della macchina.

Avvio:  uvicorn server:app --host 0.0.0.0 --port 80        (dalla cartella raspberry/plekto, vedi plekto.service)
Variabili: PLEKTO_DATI (cartella dei dati, default /var/lib/plekto), PLEKTO_PORTA (seriale ESP32, default /dev/ttyUSB0)
"""
import io
import json
import os
import subprocess
import threading
from contextlib import asynccontextmanager

import trimesh
from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import FileResponse, PlainTextResponse, RedirectResponse
from fastapi.staticfiles import StaticFiles

import aggiorna
import macchina
import retainer

QUI = os.path.dirname(os.path.abspath(__file__))
DATI = macchina.DATI
LATI = ('sup', 'inf')
NOMI = {'sup': 'superiore', 'inf': 'inferiore'}
MAX_STL = 80 * 1024 * 1024
VERSIONE = aggiorna.versione_installata()
CDN = 'https://cdn.jsdelivr.net/npm/three@0.160.0/'        # se manca la copia locale in static/vendor

mac = macchina.Macchina(os.environ.get('PLEKTO_PORTA', '/dev/ttyUSB0'))
arcate = {}                                                 # lato -> trimesh.Trimesh
progetti = {}                                               # lato -> ultimo risultato
blocco = threading.Lock()


@asynccontextmanager
async def avvio(app):
    os.makedirs(os.path.join(DATI, 'arcate'), exist_ok=True)
    for lato in LATI:                                       # le arcate caricate restano dopo un riavvio
        p = os.path.join(DATI, 'arcate', lato + '.stl')
        if os.path.exists(p):
            try:
                arcate[lato] = retainer.carica_stl(p)
            except Exception:
                pass
    mac.start()
    yield


app = FastAPI(title='Plekto', lifespan=avvio)


@app.get('/')
def pagina():
    return FileResponse(os.path.join(QUI, 'static', 'index.html'))


@app.get('/vendor/{percorso:path}')
def vendor(percorso):
    locale = os.path.normpath(os.path.join(QUI, 'static', 'vendor', percorso))
    if locale.startswith(os.path.join(QUI, 'static', 'vendor')) and os.path.isfile(locale):
        return FileResponse(locale, media_type='text/javascript')
    return RedirectResponse(CDN + percorso)


@app.get('/api/salute')
def salute():
    return {'ok': True, 'versione': VERSIONE}


# ------------------------------------------------------------------------------------------------ arcate e retainer
def _info(lato):
    m = arcate[lato]
    return {'lato': lato, 'facce': int(len(m.faces)), 'misure': [round(float(x), 1) for x in m.extents],
            'chiusa': bool(m.is_watertight)}


@app.get('/api/arcate')
def elenco_arcate():
    return {lato: _info(lato) for lato in arcate}


@app.get('/api/arcata/{lato}.stl')
def scarica_arcata(lato: str):
    p = os.path.join(DATI, 'arcate', lato + '.stl')
    if lato not in LATI or not os.path.exists(p):
        raise HTTPException(404, 'Arcata non caricata')
    return FileResponse(p, media_type='model/stl')


@app.put('/api/arcata/{lato}')
async def carica_arcata(lato: str, req: Request):
    if lato not in LATI:
        raise HTTPException(404, 'Lato sconosciuto')
    dati = await req.body()
    if not dati or len(dati) > MAX_STL:
        raise HTTPException(413, 'File vuoto o troppo grande (massimo 80 MB)')
    try:
        m = trimesh.load(io.BytesIO(dati), file_type='stl', force='mesh')
        if not isinstance(m, trimesh.Trimesh) or m.is_empty:
            raise ValueError
    except Exception:
        raise HTTPException(422, 'Il file non è un STL valido')
    if not 20 < max(m.extents) < 150:
        raise HTTPException(422, "Misure strane (%.0f mm): l'STL deve essere in millimetri" % max(m.extents))
    with blocco:
        with open(os.path.join(DATI, 'arcate', lato + '.stl'), 'wb') as f:
            f.write(dati)
        arcate[lato] = m
        progetti.pop(lato, None)
    return _info(lato)


@app.delete('/api/arcata/{lato}')
def togli_arcata(lato: str):
    with blocco:
        arcate.pop(lato, None)
        progetti.pop(lato, None)
        p = os.path.join(DATI, 'arcate', lato + '.stl')
        if os.path.exists(p):
            os.remove(p)
    return {'ok': True}


@app.post('/api/progetta/{lato}')
def progetta(lato: str, corpo: dict):
    if lato not in arcate:
        raise HTTPException(404, "Carica prima l'arcata %s" % NOMI.get(lato, lato))
    punti = corpo.get('punti')
    antagonista = arcate.get('inf' if lato == 'sup' else 'sup')
    try:
        r = retainer.progetta(arcate[lato], punti, antagonista, 'retainer %s' % NOMI[lato])
    except retainer.ErroreProgetto as e:
        raise HTTPException(422, str(e))
    except Exception as e:
        raise HTTPException(500, 'Calcolo non riuscito: %s' % e)
    r['punti'] = punti
    r['antagonista'] = antagonista is not None
    progetti[lato] = r
    return r


@app.get('/api/gcode/{lato}')
def scarica_gcode(lato: str):
    if lato not in progetti:
        raise HTTPException(404, 'Nessun retainer progettato')
    return PlainTextResponse(progetti[lato]['gcode'], headers={
        'Content-Disposition': 'attachment; filename="retainer_%s.gcode"' % NOMI[lato]})


@app.post('/api/invia/{lato}')
def invia_alla_macchina(lato: str):
    if lato not in progetti:
        raise HTTPException(404, 'Nessun retainer progettato')
    if not mac.collegata:
        raise HTTPException(503, 'Macchina non collegata')
    # l'interprete G-code dell'ESP32 non c'è ancora: per ora il lavoro resta pronto sul Raspberry
    return {'ok': False, 'motivo': "La piegatura dalla pagina web arriva con il firmware dei motori"}


# ------------------------------------------------------------------------------------------------ macchina (ESP32)
@app.get('/api/macchina')
def stato_macchina():
    imp = macchina.leggi_imp()
    return {'collegata': mac.collegata, 'esp': mac.esp, 'lingua': imp.get('lingua', 'it'),
            'wifi': imp.get('wifi', {}).get('ssid', ''), 'pi': dict(macchina.stato_rete(), versione=VERSIONE)}


@app.post('/api/macchina/comando')
def comando_macchina(corpo: dict):
    c = corpo.get('c')
    if c not in ('luci', 'suono', 'riavvia', 'agg'):
        raise HTTPException(400, 'Comando sconosciuto')
    if not mac.comando(c, **{k: v for k, v in corpo.items() if k in ('v', 'hz', 'ms')}):
        raise HTTPException(503, 'Macchina non collegata')
    return {'ok': True}


@app.post('/api/impostazioni')
def imposta(corpo: dict):
    if corpo.get('lingua') not in ('it', 'en'):
        raise HTTPException(400, 'Lingua non valida')
    mac.imposta(lingua=corpo['lingua'])
    return {'ok': True}


# ------------------------------------------------------------------------------------------------ aggiornamenti
@app.get('/api/sistema/aggiornamenti')
def controlla_aggiornamenti():
    try:
        r = aggiorna.controlla()
    except Exception as e:
        raise HTTPException(502, 'Controllo non riuscito (rete?): %s' % e)
    try:
        with open(aggiorna.ESITO, encoding='utf-8') as f:
            r['ultimo_esito'] = json.load(f)
    except (OSError, ValueError):
        pass
    return r


@app.post('/api/sistema/aggiorna')
def aggiorna_sistema():
    r = subprocess.run(['systemctl', 'start', '--no-block', 'plekto-aggiorna'], capture_output=True, text=True)
    if r.returncode != 0:
        raise HTTPException(500, r.stderr.strip() or 'Avvio aggiornamento non riuscito')
    return {'ok': True}


app.mount('/static', StaticFiles(directory=os.path.join(QUI, 'static')), name='static')

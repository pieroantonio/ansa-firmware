"""Progetto del retainer linguale su UN SOLO PIANO (la macchina piega solo nel piano).

Il clinico clicca 3 punti sull'arcata: dente di inizio, dente di fine e un punto centrale che fissa altezza e
inclinazione. Il piano per i 3 punti taglia l'arcata. Dall'interno dell'arcata (lato lingua) si "guarda" il taglio
con dei raggi: il primo dente colpito è la superficie linguale. Il filo corre a un raggio di filo dai denti e
scavalca gli spazi interdentali (apertura morfologica), così non entra tra un dente e l'altro.
Funziona sia con scansioni intraorali aperte sia con modelli chiusi. Se c'è l'arcata antagonista (scansioni in
occlusione) si controllano i contatti e si propone di quanto spostare il retainer verso la gengiva.
Infine la linea diventa una sequenza di avanzamenti e pieghe per la macchina. Unità: mm e gradi.
"""
import numpy as np
import trimesh
from shapely.geometry import LineString, Point, Polygon
from shapely.ops import substring

D_FILO = 0.4445              # 0,0175" (filo scelto, DEC-002)
SCAVALCO = 1.5               # mm: raggio dell'apertura, il filo non entra in spazi più stretti di 2x questo valore
GIOCO_CONTATTO = 0.30        # mm: distanza minima tra la superficie del filo e l'antagonista
TOLL_PIEGHE = 0.10           # mm: scarto massimo tra la curva e la spezzata realizzata dalla macchina
TRATTO_MIN = 1.5             # mm: tratto dritto minimo tra due pieghe (⚠ ipotesi, da misurare sulla testa di piega)
ANGOLO_MIN = 1.0             # gradi: pieghe più piccole vengono unite al tratto
N_RAGGI = 900


class ErroreProgetto(Exception):
    pass


def carica_stl(percorso):
    m = trimesh.load(percorso, file_type='stl', force='mesh')
    if not isinstance(m, trimesh.Trimesh) or m.is_empty:
        raise ErroreProgetto('Il file non contiene una superficie valida')
    return m


class Piano:
    """Piano per i 3 punti con coordinate 2D: origine nel dente di inizio, asse u verso il dente di fine."""

    def __init__(self, punti):
        p = np.asarray(punti, float)
        if p.shape != (3, 3):
            raise ErroreProgetto('Servono 3 punti: inizio, fine e centro')
        n = np.cross(p[1] - p[0], p[2] - p[0])
        if np.linalg.norm(n) < 1e-3 * max(1.0, np.linalg.norm(p[1] - p[0]) ** 2):
            raise ErroreProgetto('I tre punti sono allineati: sposta il punto centrale')
        self.o, self.n = p[0], n / np.linalg.norm(n)
        self.u = (p[1] - p[0]) / np.linalg.norm(p[1] - p[0])
        self.w = np.cross(self.n, self.u)

    def a_2d(self, x):
        d = np.asarray(x, float) - self.o
        return np.c_[d @ self.u, d @ self.w]

    def a_3d(self, c):
        c = np.asarray(c, float)
        return self.o + np.outer(c[:, 0], self.u) + np.outer(c[:, 1], self.w)


def _colpi(origine, angoli, seg):
    """Primo punto colpito da ogni raggio (2D) sui segmenti del taglio; NaN se il raggio non colpisce nulla."""
    d = np.c_[np.cos(angoli), np.sin(angoli)]                      # (N, 2)
    a, b = seg[:, 0], seg[:, 1]                                    # (M, 2)
    e = b - a
    oa = a - origine
    den = d[:, None, 0] * e[None, :, 1] - d[:, None, 1] * e[None, :, 0]          # d x e
    with np.errstate(divide='ignore', invalid='ignore'):
        t = (oa[None, :, 0] * e[None, :, 1] - oa[None, :, 1] * e[None, :, 0]) / den
        s = (oa[None, :, 0] * d[:, None, 1] - oa[None, :, 1] * d[:, None, 0]) / den
    t = np.where((np.abs(den) > 1e-12) & (t > 1e-6) & (s >= 0) & (s <= 1), t, np.inf)
    tmin = t.min(axis=1)
    return origine + d * tmin[:, None], np.isfinite(tmin)


def _linea_filo(arcata, punti):
    """Linea del centro del filo nel piano (2D, dal dente di inizio al dente di fine) e il piano."""
    pi = Piano(punti)
    seg3 = trimesh.intersections.mesh_plane(arcata, pi.n, pi.o)
    if len(seg3) == 0:
        raise ErroreProgetto("Il piano non taglia l'arcata: prova a spostare i punti")
    seg = pi.a_2d(seg3.reshape(-1, 3)).reshape(-1, 2, 2)
    p1, p2, p3 = pi.a_2d(punti)
    m = (p1 + p2) / 2
    origine = m + 0.2 * (p3 - m)                                   # dentro l'arcata, lato lingua
    a1 = np.arctan2(*(p1 - origine)[::-1])
    a2 = np.arctan2(*(p2 - origine)[::-1])
    a3 = np.arctan2(*(p3 - origine)[::-1])
    da = (a2 - a1) % (2 * np.pi)                                   # da inizio a fine passando dal centro
    if (a3 - a1) % (2 * np.pi) > da:
        da -= 2 * np.pi
    marg = 0.15 * np.sign(da)
    ang = a1 - marg + np.linspace(0, 1, N_RAGGI) * (da + 2 * marg)
    colpi, ok = _colpi(origine, ang, seg)
    if ok.mean() < 0.8:
        raise ErroreProgetto('Il taglio non è chiuso tra inizio e fine: metti i punti sulle superfici linguali, '
                             'sopra la gengiva')
    zona = Polygon(np.vstack([origine, colpi[ok]])).buffer(0)     # lato lingua visto dall'interno dell'arcata
    r = D_FILO / 2
    libera = zona.buffer(-(r + SCAVALCO), join_style=1).buffer(SCAVALCO, join_style=1)
    if libera.is_empty:
        raise ErroreProgetto("Spazio linguale troppo stretto per il filo")
    if libera.geom_type != 'Polygon':
        libera = max(libera.geoms, key=lambda g: g.area)
    anello = LineString(libera.exterior.coords)
    L = anello.length
    s1, s2, s3 = (anello.project(Point(p)) for p in (p1, p2, p3))
    a, b = sorted((s1, s2))
    if a <= s3 <= b:
        c = np.array(substring(anello, a, b).coords)
    else:                                                          # il tratto giusto passa per l'inizio dell'anello
        c = np.vstack([np.array(substring(anello, b, L).coords), np.array(substring(anello, 0, a).coords)[1:]])
    if np.linalg.norm(c[0] - p1) > np.linalg.norm(c[-1] - p1):
        c = c[::-1]
    return c, pi


def _contatti(punti3d, antagonista):
    if antagonista is None:
        return [], None
    passo = np.linalg.norm(np.diff(punti3d, axis=0), axis=1)
    s = np.r_[0, np.cumsum(passo)]
    fitti = np.c_[[np.interp(np.arange(0, s[-1], 0.25), s, punti3d[:, k]) for k in range(3)]].T
    _, dist, _ = trimesh.proximity.closest_point(antagonista, fitti)
    gioco = dist - D_FILO / 2
    idx = np.where(gioco < GIOCO_CONTATTO)[0]
    contatti = [{'punto': fitti[i].round(3).tolist(), 'gioco': round(float(gioco[i]), 2)} for i in idx[::4]]
    return contatti, float(gioco.min())


def direzione_gengivale(arcata, antagonista):
    """Asse perpendicolare al piano occlusale (minima varianza dei vertici), verso la gengiva (via dall'antagonista)."""
    v = arcata.vertices[:: max(1, len(arcata.vertices) // 20000)]
    _, _, vt = np.linalg.svd(v - v.mean(axis=0), full_matrices=False)
    asse = vt[2]
    if np.dot(antagonista.vertices.mean(axis=0) - arcata.vertices.mean(axis=0), asse) > 0:
        asse = -asse
    return asse


def _spezzata(c2):
    """Curva -> spezzata con tratti di almeno TRATTO_MIN: toglie il vertice meno piegato finché serve."""
    p = [np.asarray(x) for x in LineString(c2).simplify(TOLL_PIEGHE, preserve_topology=False).coords]

    def angolo(i):
        d1, d2 = p[i] - p[i - 1], p[i + 1] - p[i]
        return float(np.degrees(np.arctan2(d1[0] * d2[1] - d1[1] * d2[0], d1 @ d2)))

    while len(p) > 2:
        lung = [np.linalg.norm(p[i + 1] - p[i]) for i in range(len(p) - 1)]
        corti = [i for i, l in enumerate(lung) if l < TRATTO_MIN]
        piatti = [i for i in range(1, len(p) - 1) if abs(angolo(i)) < ANGOLO_MIN]
        if not corti and not piatti:
            break
        if piatti:
            del p[min(piatti, key=lambda i: abs(angolo(i)))]
            continue
        i = corti[0]                                               # tratto corto tra i vertici i e i+1
        cand = [k for k in (i, i + 1) if 0 < k < len(p) - 1]
        del p[min(cand, key=lambda k: abs(angolo(k)))]
    return np.array(p)


def _pieghe(spezzata):
    """Avanzamenti e angoli di piega (positivo = a sinistra guardando il filo che esce)."""
    passi = []
    for i in range(1, len(spezzata)):
        d1 = spezzata[i] - spezzata[i - 1]
        passo = {'avanza': round(float(np.linalg.norm(d1)), 2), 'piega': 0.0}
        if i < len(spezzata) - 1:
            d2 = spezzata[i + 1] - spezzata[i]
            passo['piega'] = round(float(np.degrees(np.arctan2(d1[0] * d2[1] - d1[1] * d2[0], d1 @ d2))), 1)
        passi.append(passo)
    return passi


def gcode(passi, nome='retainer'):
    """G-code semplice per l'interprete dell'ESP32: G1 L = avanzamento del filo, B = piega nel piano.
    La compensazione del ritorno elastico e dell'abbassamento per gravità la aggiunge la taratura (in arrivo)."""
    righe = ['; Plekto - %s' % nome, '; filo %.4f mm, %d pieghe' % (D_FILO, sum(1 for p in passi if p['piega'])),
             'G21', 'G91']
    for p in passi:
        righe.append('G1 L%.2f' % p['avanza'])
        if p['piega']:
            righe.append('B%.1f' % p['piega'])
    righe.append('M5 ; taglio')
    return '\n'.join(righe) + '\n'


def progetta(arcata, punti, antagonista=None, nome='retainer'):
    """punti: [inizio, fine, centro] in mm sulla superficie dell'arcata. Restituisce un dizionario per l'interfaccia."""
    c2, pi = _linea_filo(arcata, punti)
    p3 = pi.a_3d(c2)
    contatti, gioco_min = _contatti(p3, antagonista)
    suggerimento = None
    if contatti:                                                   # di quanto spostare tutto il retainer?
        g = direzione_gengivale(arcata, antagonista)
        for s in np.arange(0.25, 3.01, 0.25):
            q = np.asarray(punti, float) + g * s
            try:
                c2b, pib = _linea_filo(arcata, q)
            except ErroreProgetto:
                continue
            if not _contatti(pib.a_3d(c2b), antagonista)[0]:
                suggerimento = {'sposta_mm': round(float(s), 2), 'vettore': (g * s).round(4).tolist(),
                                'testo': 'Sposta il retainer di %.2f mm verso la gengiva: spariscono i contatti' % s}
                break
        if suggerimento is None:
            suggerimento = {'sposta_mm': None, 'testo': 'Contatti anche spostando di 3 mm: cambia inclinazione '
                                                         '(punto centrale) o accorcia il retainer'}
    spezzata = _spezzata(c2)
    passi = _pieghe(spezzata)
    return {
        'curva': p3.round(3).tolist(),
        'spezzata': pi.a_3d(spezzata).round(3).tolist(),
        'lunghezza': round(float(sum(p['avanza'] for p in passi)), 1),
        'pieghe': passi,
        'n_pieghe': sum(1 for p in passi if p['piega']),
        'scarto_max': round(float(LineString(spezzata).hausdorff_distance(LineString(c2))), 2),
        'contatti': contatti,
        'gioco_min': None if gioco_min is None else round(gioco_min, 2),
        'suggerimento': suggerimento,
        'gcode': gcode(passi, nome),
    }

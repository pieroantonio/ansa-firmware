"""Arcate sintetiche (denti ellissoidi su una parabola, in occlusione) per provare il progetto del retainer senza
scansioni vere. Scrive arcata_sup.stl e arcata_inf.stl nella cartella indicata e stampa l'esito di due progetti:
retainer superiore 13-23 alto (nessun contatto) e basso (contatto con gli incisivi inferiori).
Uso: python prova_sintetica.py [cartella]"""
import os
import sys
import numpy as np
import trimesh

import retainer

# larghezze mesio-distali (mm) dal centrale al primo molare, e spessore vestibolo-linguale
SUP = [(8.5, 7.0), (6.5, 6.0), (7.5, 8.0), (7.0, 9.0), (6.5, 9.0), (10.0, 11.0)]
INF = [(5.0, 6.0), (5.5, 6.0), (6.5, 7.0), (7.0, 7.5), (7.0, 8.0), (11.0, 10.5)]


def arcata(denti, profondita, k, z_centro, aperta):
    """Denti lungo y = profondita - k x^2 (incisivi davanti, verso +y). aperta: toglie le facce sotto la gengiva
    come in una scansione intraorale."""
    xs = np.linspace(0, 40, 4001)
    ys = profondita - k * xs ** 2
    s = np.r_[0, np.cumsum(np.hypot(np.diff(xs), np.diff(ys)))]
    parti, centri = [], []
    for lato in (1, -1):
        pos = 0.0
        for w, b in denti:
            m = pos + w / 2
            pos += w
            x, y = np.interp(m, s, xs), np.interp(m, s, ys)
            t = np.array([1.0, -2 * k * x])
            t /= np.linalg.norm(t)
            nrm = np.array([-t[1], t[0]])                    # verso +y (vestibolare per gli incisivi)
            e = trimesh.creation.icosphere(subdivisions=4)
            e.apply_scale([w / 2 * 1.12, b / 2, 5.0])        # 12 % di sovrapposizione: punti di contatto
            rot = np.eye(4)
            rot[:2, 0], rot[:2, 1] = t, nrm
            rot[:3, 3] = [lato * x, y, z_centro]
            if lato < 0:
                rot[0, :2] *= -1
            e.apply_transform(rot)
            parti.append(e)
            centri.append((lato * x, y, nrm * [lato, 1], b / 2))
    m = trimesh.util.concatenate(parti)
    if aperta:
        gengiva = z_centro + (3.5 if z_centro > 0 else -3.5)
        tieni = (m.triangles_center[:, 2] < gengiva) if z_centro > 0 else (m.triangles_center[:, 2] > gengiva)
        m.update_faces(tieni)
        m.remove_unreferenced_vertices()
    return m, centri


def punto_linguale(centri, i, z):
    x, y, nrm, b = centri[i]
    return [x - nrm[0] * b, y - nrm[1] * b, z]


def main():
    cartella = sys.argv[1] if len(sys.argv) > 1 else '.'
    sup, cs = arcata(SUP, 40.0, 0.060, 5.0, aperta=True)
    inf, ci = arcata(INF, 35.8, 0.072, -3.0, aperta=True)    # incisivi inferiori dietro ai superiori, overbite 2 mm
    sup.export(os.path.join(cartella, 'arcata_sup.stl'))
    inf.export(os.path.join(cartella, 'arcata_inf.stl'))
    for nome, z in (('alto', 5.5), ('basso', 2.2)):
        # 13 = canino destro (indice 2), 23 = canino sinistro (indice 8), centro = incisivo centrale (indice 0)
        punti = [punto_linguale(cs, 2, z + 1.0), punto_linguale(cs, 8, z + 1.0), punto_linguale(cs, 0, z)]
        r = retainer.progetta(sup, punti, inf, 'prova ' + nome)
        print('%-5s lunghezza %.1f mm, %d pieghe, scarto %.2f mm, gioco minimo %s mm, contatti %d'
              % (nome, r['lunghezza'], r['n_pieghe'], r['scarto_max'], r['gioco_min'], len(r['contatti'])))
        if r['suggerimento']:
            print('      ', r['suggerimento']['testo'])
    print(r['gcode'][:300])


if __name__ == '__main__':
    main()

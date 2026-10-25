"""Sintetizzatore dei suoni di Plekto: lo stesso algoritmo del firmware (src/suoni.cpp), qui per ascoltarli in WAV.

Timbro "vetro": seno + 2a e 3a armonica che si spengono prima della fondamentale; inviluppo con attacco morbido e
coda esponenziale; riverbero corto (2 filtri a pettine + 1 passa-tutto). Frequenza di campionamento 16 kHz come
l'uscita PDM dell'ESP32. Uso: python sintesi_suoni.py [cartella]
"""
import math
import os
import sys
import wave

import numpy as np

SR = 16000

# ogni nota: (inizio s, frequenza Hz, durata s, volume 0..1)
SUONI = {
    # avvio: accordo di do maggiore con la settima (do mi sol si) che sboccia nota dopo nota, poi un do acuto
    'avvio': [(0.00, 523.25, 1.9, 0.55), (0.06, 659.26, 1.8, 0.45), (0.12, 783.99, 1.7, 0.45),
              (0.18, 987.77, 1.6, 0.35), (0.32, 1046.5, 1.4, 0.40)],
    # lavoro compiuto: due note di vetro che salgono (re - sol), la seconda sopra la coda della prima
    'fatto': [(0.00, 1174.66, 0.9, 0.55), (0.11, 1567.98, 1.1, 0.60)],
    # errore: due note morbide che scendono (re - si bemolle), corte e smorzate
    'errore': [(0.00, 587.33, 0.35, 0.65), (0.16, 466.16, 0.55, 0.65)],
    # traguardo (medaglia nuova): scala luminosa che sale veloce (sol do mi sol) e un do acuto che resta nell'aria
    'traguardo': [(0.00, 783.99, 0.9, 0.40), (0.07, 1046.5, 0.9, 0.40), (0.14, 1318.51, 1.0, 0.40),
                  (0.21, 1567.98, 1.2, 0.42), (0.30, 2093.0, 1.8, 0.50)],
}
ATTACCO = 0.012          # s
ARMONICHE = [(1, 1.0, 1.0), (2, 0.28, 2.5), (3, 0.10, 4.0)]   # (multiplo, ampiezza, velocità di decadimento)


def nota(f, durata, vol):
    n = int(SR * durata)
    t = np.arange(n) / SR
    x = np.zeros(n)
    for k, amp, vel in ARMONICHE:
        if f * k > SR / 2:
            continue
        coda = np.exp(-t * vel * 4.0 / durata)
        x += amp * np.sin(2 * math.pi * f * k * t) * coda
    x *= np.minimum(1.0, t / ATTACCO)                # attacco morbido, niente clic
    return x * vol


def riverbero(x, misto=0.22):
    def pettine(x, d, g):
        y = np.copy(x)
        for i in range(d, len(y)):
            y[i] += g * y[i - d]
        return y

    def passatutto(x, d, g):
        y = np.zeros_like(x)
        for i in range(len(x)):
            xd = x[i - d] if i >= d else 0.0
            yd = y[i - d] if i >= d else 0.0
            y[i] = -g * x[i] + xd + g * yd
        return y
    w = (pettine(x, 557, 0.62) + pettine(x, 593, 0.60)) * 0.5
    w = passatutto(w, 179, 0.5)
    return x * (1 - misto) + w * misto


def suono(note, coda=0.6):
    fine = max(i + d for i, _, d, _ in note) + coda
    x = np.zeros(int(SR * fine))
    for inizio, f, d, v in note:
        a = int(SR * inizio)
        y = nota(f, d, v)
        x[a:a + len(y)] += y
    x = riverbero(x)
    return x / max(1e-9, np.abs(x).max()) * 0.85


def main():
    cartella = sys.argv[1] if len(sys.argv) > 1 else '.'
    os.makedirs(cartella, exist_ok=True)
    for nome, note in SUONI.items():
        x = suono(note)
        with wave.open(os.path.join(cartella, 'plekto_%s.wav' % nome), 'wb') as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(SR)
            w.writeframes((x * 32767).astype('<i2').tobytes())
        print(nome, '%.2f s' % (len(x) / SR))


if __name__ == '__main__':
    main()

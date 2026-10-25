#!/usr/bin/env python3
"""Ponte tra Raspberry Pi Zero 2 W ed ESP32 della bending machine (cavo USB, CH340).

- Tiene allineate le impostazioni condivise (lingua, rete Wi-Fi): vince la versione più alta.
  * Una scelta fatta sull'ESP32 con la rotella arriva qui, viene salvata in /etc/bending/impostazioni.json
    e applicata al Raspberry: rete Wi-Fi con NetworkManager (nmcli), lingua nel file di configurazione.
  * Una modifica fatta sul Raspberry (per esempio dalla futura pagina web) va scritta con versione + 1
    nello stesso file: il ponte la manda all'ESP32.
- Manda all'ESP32 lo stato della rete del Raspberry (mostrato in "Informazioni").

Uso:   python3 bending_ponte.py [--porta /dev/ttyUSB0]
Servizio: vedi bending-ponte.service
"""
import argparse, json, os, subprocess, time
import serial  # pip install pyserial

FILE_IMP = '/etc/bending/impostazioni.json'


def leggi_imp():
    try:
        with open(FILE_IMP, encoding='utf-8') as f:
            return json.load(f)
    except (OSError, ValueError):
        return {'ver': 0}


def scrivi_imp(d):
    os.makedirs(os.path.dirname(FILE_IMP), exist_ok=True)
    tmp = FILE_IMP + '.tmp'
    with open(tmp, 'w', encoding='utf-8') as f:
        json.dump(d, f, ensure_ascii=False, indent=2)
    os.chmod(tmp, 0o600)                      # contiene la password del Wi-Fi
    os.replace(tmp, FILE_IMP)


def stato_rete():
    try:
        ip = subprocess.run(['hostname', '-I'], capture_output=True, text=True, timeout=5).stdout.split()
        return {'wifi': bool(ip), 'ip': ip[0] if ip else ''}
    except Exception:
        return {'wifi': False, 'ip': ''}


def applica(d, precedente):
    """Applica al Raspberry le impostazioni arrivate dall'ESP32."""
    w = d.get('wifi', {})
    ssid, pw = w.get('ssid', ''), w.get('pass', '')
    vecchio = precedente.get('wifi', {})
    if ssid and (ssid != vecchio.get('ssid') or pw != vecchio.get('pass')):
        cmd = ['nmcli', 'device', 'wifi', 'connect', ssid] + (['password', pw] if pw else [])
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        print('# wifi', ssid, 'ok' if r.returncode == 0 else 'errore: ' + r.stderr.strip())
    # la lingua è letta dal software della macchina dal file delle impostazioni: niente altro da fare qui


def invia(ser, msg):
    ser.write((json.dumps(msg, ensure_ascii=False) + '\n').encode('utf-8'))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--porta', default='/dev/ttyUSB0')
    a = ap.parse_args()
    while True:
        try:
            ser = serial.Serial()
            ser.port, ser.baudrate, ser.timeout = a.porta, 115200, 0.5
            ser.dtr = False                          # non resettare l'ESP32 all'apertura della porta
            ser.rts = False
            ser.open()
            with ser:
                time.sleep(0.5)
                invia(ser, {'t': 'cfg?'})
                locale = leggi_imp()
                if locale.get('ver', 0) > 0:
                    invia(ser, dict(locale, t='cfg'))     # se le mie sono più recenti, l'ESP32 le adotta
                ultimo_stato = 0
                while True:
                    riga = ser.readline().decode('utf-8', 'replace').strip()
                    if riga.startswith('{'):
                        try:
                            m = json.loads(riga)
                        except ValueError:
                            continue
                        if m.get('t') == 'cfg':
                            locale = leggi_imp()
                            if m.get('ver', 0) > locale.get('ver', 0):
                                m.pop('t', None)
                                applica(m, locale)
                                scrivi_imp(m)
                            elif m.get('ver', 0) < locale.get('ver', 0):
                                invia(ser, dict(locale, t='cfg'))
                    # file modificato sul Raspberry con versione più alta: lo mando all'ESP32
                    locale = leggi_imp()
                    if time.time() - ultimo_stato > 5:
                        invia(ser, dict(stato_rete(), t='stato'))
                        if locale.get('ver', 0) > 0:
                            invia(ser, dict(locale, t='cfg'))
                        ultimo_stato = time.time()
        except serial.SerialException as e:
            print('# porta non disponibile:', e)
            time.sleep(3)


if __name__ == '__main__':
    main()

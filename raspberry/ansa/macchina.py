"""Collegamento USB (CH340) tra Raspberry Pi ed ESP32 di Ansa, in un thread del server web.

Protocollo: una riga JSON per messaggio, vedi firmware/esp32/src/collegamento.h.
- Tiene allineate le impostazioni condivise (lingua, rete Wi-Fi): vince la versione più alta. Una scelta fatta
  sull'ESP32 con la rotella viene salvata qui e applicata al Raspberry (rete con NetworkManager); una modifica
  fatta dalla pagina web viene salvata con versione + 1 e mandata all'ESP32.
- Manda all'ESP32 lo stato della rete del Raspberry e i comandi della pagina web; ne riceve lo stato.
"""
import json
import os
import subprocess
import threading
import time

try:
    import serial
except ImportError:                                   # sul PC di sviluppo si può provare il server senza pyserial
    serial = None

DATI = os.environ.get('ANSA_DATI', '/var/lib/ansa')
FILE_IMP = os.path.join(DATI, 'impostazioni.json')


def leggi_imp():
    try:
        with open(FILE_IMP, encoding='utf-8') as f:
            return json.load(f)
    except (OSError, ValueError):
        return {'ver': 0}


def scrivi_imp(d):
    os.makedirs(DATI, exist_ok=True)
    tmp = FILE_IMP + '.tmp'
    with open(tmp, 'w', encoding='utf-8') as f:
        json.dump(d, f, ensure_ascii=False, indent=2)
    os.chmod(tmp, 0o600)                              # contiene la password del Wi-Fi
    os.replace(tmp, FILE_IMP)


def stato_rete():
    try:
        ip = subprocess.run(['hostname', '-I'], capture_output=True, text=True, timeout=5).stdout.split()
        return {'wifi': bool(ip), 'ip': ip[0] if ip else ''}
    except Exception:
        return {'wifi': False, 'ip': ''}


def applica_wifi(d, precedente):
    w, vecchio = d.get('wifi', {}), precedente.get('wifi', {})
    ssid, pw = w.get('ssid', ''), w.get('pass', '')
    if ssid and (ssid != vecchio.get('ssid') or pw != vecchio.get('pass')):
        cmd = ['nmcli', 'device', 'wifi', 'connect', ssid] + (['password', pw] if pw else [])
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            print('# wifi', ssid, 'ok' if r.returncode == 0 else 'errore: ' + r.stderr.strip())
        except (OSError, subprocess.TimeoutExpired) as e:
            print('# wifi', ssid, 'errore:', e)


class Macchina(threading.Thread):
    def __init__(self, porta):
        super().__init__(daemon=True)
        self.porta = porta
        self.ser = None
        self.blocco = threading.Lock()
        self.esp = {}                                 # ultimo {"t":"esp",...}
        self.ultimo_msg = 0.0

    @property
    def collegata(self):
        return self.ser is not None and time.time() - self.ultimo_msg < 15

    def invia(self, msg):
        with self.blocco:
            if self.ser is None:
                return False
            try:
                self.ser.write((json.dumps(msg, ensure_ascii=False) + '\n').encode('utf-8'))
                return True
            except Exception:
                return False

    def comando(self, c, **parametri):
        return self.invia(dict(parametri, t='cmd', c=c))

    def imposta(self, **modifiche):
        """Modifica dalla pagina web (es. lingua): versione + 1, poi la ricevono Raspberry ed ESP32."""
        d = leggi_imp()
        precedente = dict(d)
        d.update(modifiche)
        d['ver'] = d.get('ver', 0) + 1
        scrivi_imp(d)
        applica_wifi(d, precedente)
        self.invia(dict(d, t='cfg'))
        return d

    def _ricevuto(self, m):
        self.ultimo_msg = time.time()
        t = m.get('t')
        if t == 'esp':
            m.pop('t')
            self.esp = m
        elif t == 'cfg':
            locale = leggi_imp()
            if m.get('ver', 0) > locale.get('ver', 0):
                m.pop('t')
                applica_wifi(m, locale)
                scrivi_imp(m)
            elif m.get('ver', 0) < locale.get('ver', 0):
                self.invia(dict(locale, t='cfg'))
        elif t == 'ciao':
            self.esp['fw'] = m.get('fw', '')
            self.invia({'t': 'cfg?'})

    def run(self):
        if serial is None:
            print('# pyserial non installato: macchina non collegata')
            return
        while True:
            try:
                s = serial.Serial()
                s.port, s.baudrate, s.timeout = self.porta, 115200, 0.5
                s.dtr = False                         # non resettare l'ESP32 all'apertura della porta
                s.rts = False
                s.open()
                with s:
                    self.ser = s
                    time.sleep(0.5)
                    self.invia({'t': 'cfg?'})
                    self.invia({'t': 'esp?'})
                    ultimo_stato = 0
                    while True:
                        riga = s.readline().decode('utf-8', 'replace').strip()
                        if riga.startswith('{'):
                            try:
                                self._ricevuto(json.loads(riga))
                            except ValueError:
                                pass
                        if time.time() - ultimo_stato > 5:
                            self.invia(dict(stato_rete(), t='stato'))
                            self.invia({'t': 'esp?'})
                            ultimo_stato = time.time()
            except Exception as e:                    # porta assente o cavo staccato: riprovo
                self.ser = None
                self.esp = {}
                if not isinstance(e, getattr(serial, 'SerialException', Exception)):
                    print('# collegamento:', e)
                time.sleep(3)

"""Imposta lingua italiana e rete Wi-Fi sull'ESP32 della bending machine, via cavo USB.

La password del Wi-Fi la scrivi tu qui nel terminale (non compare a schermo) e va direttamente alla scheda.
Uso (dalla cartella bending):
    %USERPROFILE%\\.platformio\\penv\\Scripts\\python.exe firmware\\esp32\\imposta_wifi.py [COM6]
"""
import getpass, json, sys, time
import serial  # incluso nel Python di PlatformIO

porta = sys.argv[1] if len(sys.argv) > 1 else 'COM6'
ssid = input('Nome della rete Wi-Fi: ').strip()
pw = getpass.getpass('Password del Wi-Fi (non viene mostrata): ')

s = serial.Serial()
s.port, s.baudrate, s.timeout = porta, 115200, 0.3
s.dtr = False                       # aprire la porta senza riavviare la scheda
s.rts = False
s.open()
time.sleep(0.3)


def leggi_json(secondi):
    fine, buf = time.time() + secondi, b''
    while time.time() < fine:
        buf += s.read(512)
        while b'\n' in buf:
            riga, buf = buf.split(b'\n', 1)
            riga = riga.strip()
            if riga.startswith(b'{'):
                try:
                    yield json.loads(riga)
                except ValueError:
                    pass


s.write(b'{"t":"cfg?"}\n')
ver = 0
for m in leggi_json(3):
    if m.get('t') == 'cfg':
        ver = m.get('ver', 0)
        break
cfg = {'t': 'cfg', 'ver': ver + 1, 'lingua': 'it', 'configurato': True, 'wifi': {'ssid': ssid, 'pass': pw}}
s.write((json.dumps(cfg) + '\n').encode('utf-8'))
time.sleep(1.0)
print('Impostazioni inviate (lingua: italiano, rete: %s). Riavvio la scheda...' % ssid)
s.rts = True                        # impulso su EN: riavvio normale
time.sleep(0.15)
s.rts = False
ip = None
for m in leggi_json(25):
    if m.get('t') == 'rete':
        ip = m.get('ip')
        break
s.close()
if ip:
    print('Collegata al Wi-Fi. Indirizzo della scheda: %s (anche bending.local)' % ip)
else:
    print('La scheda non ha comunicato un indirizzo entro 25 s: controlla nome e password della rete (deve essere a 2,4 GHz).')

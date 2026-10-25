// Lingua della pagina: la stessa scelta sul display della macchina (l'ESP32 la manda al Raspberry via USB).
// La pagina è scritta in italiano; qui le traduzioni inglesi. t('testo', {x: 1}) traduce e sostituisce {x}.
const EN = {
  // pagina
  'Progetta': 'Design', 'Macchina': 'Machine', 'Sistema': 'System',
  'Macchina collegata': 'Machine connected', 'Macchina non collegata': 'Machine not connected',
  'Trascina qui gli STL delle arcate': 'Drop the arch STL files here', "Rilascia per caricare l'arcata": 'Release to load the arch',
  'Superiore': 'Upper', 'Inferiore': 'Lower', 'Entrambi': 'Both', 'Occlusale': 'Occlusal', 'Adatta': 'Fit',
  'Arcate': 'Arches', 'Retainer': 'Retainer', 'Posizione': 'Position', 'Verifica': 'Check',
  "Trascina l'STL o fai clic": 'Drop the STL or click', 'Caricamento': 'Loading', 'Togli': 'Remove',
  "Con entrambe le arcate in occlusione Plekto controlla i contatti con l'antagonista.":
    'With both arches in occlusion, Plekto checks contacts with the opposing arch.',
  'Dente di inizio': 'Start tooth', 'Dente di fine': 'End tooth', 'Punto centrale': 'Middle point',
  'altezza e inclinazione': 'height and tilt',
  'Clicca sulle superfici linguali, sopra la gengiva. Il retainer giace nel piano dei tre punti.':
    'Click on the lingual surfaces, above the gum. The retainer lies in the plane of the three points.',
  'Ricomincia': 'Start over', 'Lunghezza': 'Length', 'Pieghe': 'Bends', 'Scarto': 'Deviation', 'Gioco': 'Clearance',
  'Scarica G-code': 'Download G-code', 'Invia alla macchina': 'Send to machine',
  "Verificare sempre il retainer sul modello prima dell'uso clinico.": 'Always check the retainer on the model before clinical use.',
  'Luci': 'Lights', 'Suono': 'Sound', 'Avvio': 'Start-up', 'Fatto': 'Done', 'Errore': 'Error', 'Prova il suono': 'Test sound', 'Lingua del display': 'Display language',
  'Cerca aggiornamenti': 'Check for updates', 'Se c\'è una versione nuova la macchina chiede conferma sul display.':
    'If a new version is available, the machine asks for confirmation on its display.',
  'Movimenti': 'Motion', 'In arrivo': 'Coming soon', 'Carica filo': 'Load wire', 'Azzera assi': 'Home axes',
  'Taglio PEEK': 'PEEK cut', 'Riavvio': 'Restart', 'Riavvia la macchina': 'Restart the machine',
  'Software di Plekto': 'Plekto software', 'Controlla aggiornamenti': 'Check for updates', 'Aggiorna': 'Update',
  // testi che cambiano
  'Clicca il dente di inizio': 'Click the start tooth', 'Clicca il dente di fine': 'Click the end tooth',
  'Clicca il punto centrale': 'Click the middle point', 'arcata superiore': 'upper arch', 'arcata inferiore': 'lower arch',
  'superiore': 'upper', 'inferiore': 'lower', 'Sup': 'Up', 'Inf': 'Low',
  '{n}k facce': '{n}k faces', 'Serve un file .stl': 'An .stl file is needed', 'Calcolo del retainer…': 'Computing the retainer…',
  'Contatti non controllati': 'Contacts not checked',
  "Carica anche l'arcata antagonista in occlusione.": 'Load the opposing arch too, in occlusion.',
  "Nessun contatto con l'antagonista": 'No contact with the opposing arch',
  'Gioco minimo {g} mm dalla superficie del filo.': 'Minimum clearance {g} mm from the wire surface.',
  "Contatto con l'antagonista": 'Contact with the opposing arch',
  'Sposta il retainer di {s} mm verso la gengiva: spariscono i contatti.': 'Move the retainer {s} mm towards the gum to remove the contacts.',
  'Contatti anche spostando di 3 mm: cambia inclinazione (punto centrale) o accorcia il retainer.':
    'Contacts remain even after moving 3 mm: change the tilt (middle point) or shorten the retainer.',
  'Sposta di {s} mm': 'Move {s} mm', 'Rilascia il punto per la correzione proposta.': 'Release the point to see the proposed correction.', 'Retainer inviato alla macchina': 'Retainer sent to the machine',
  'Collegata via USB': 'Connected via USB', 'Non collegata: controlla il cavo USB': 'Not connected: check the USB cable',
  'Firmware': 'Firmware', 'Wi-Fi': 'Wi-Fi', 'Segnale': 'Signal', 'Rotella': 'Wheel', 'Acceso da': 'On for',
  'ottimo': 'excellent', 'buono': 'good', 'debole': 'weak', 'non configurato': 'not set up',
  'collegata': 'connected', 'non trovata': 'not found', 'Versione installata {v}': 'Installed version {v}',
  'Controllo in corso: guarda il display della macchina': 'Checking: look at the machine display',
  'Riavviare la macchina?': 'Restart the machine?', 'Versione': 'Version', 'Indirizzo': 'Address',
  'Versione installata': 'Installed version', 'Ultima versione': 'Latest version', 'Ultimo aggiornamento': 'Last update',
  'Aggiorna a {v}': 'Update to {v}', 'È disponibile una versione nuova.': 'A new version is available.',
  'Il software è aggiornato.': 'The software is up to date.', 'Controllo…': 'Checking…',
  'Aggiornamento in corso: non spegnere. La pagina si ricarica da sola.': 'Updating: do not switch off. The page reloads by itself.',
  'Aggiornamento non riuscito: è rimasta la versione di prima.': 'Update failed: the previous version is still installed.',
  // messaggi del server
  'I tre punti sono allineati: sposta il punto centrale': 'The three points are aligned: move the middle point',
  "Il piano non taglia l'arcata: prova a spostare i punti": 'The plane does not cut the arch: try moving the points',
  'Il taglio non è chiuso tra inizio e fine: metti i punti sulle superfici linguali, sopra la gengiva':
    'The cut is not closed between start and end: place the points on the lingual surfaces, above the gum',
  'Spazio linguale troppo stretto per il filo': 'Lingual space too narrow for the wire',
  'Il file non è un STL valido': 'The file is not a valid STL', 'File vuoto o troppo grande (massimo 80 MB)': 'Empty or too large file (max 80 MB)',
  'Nessun retainer progettato': 'No retainer designed', 'Macchina non collegata.': 'Machine not connected.',
  'La piegatura dalla pagina web arriva con il firmware dei motori': 'Bending from the web page comes with the motor firmware',
};

let lingua = 'it';
const originali = new WeakMap();                     // nodo di testo -> testo italiano

export function t(testo, valori = {}) {
  let s = lingua === 'en' && EN[testo] !== undefined ? EN[testo] : testo;
  for (const [k, v] of Object.entries(valori)) s = s.replaceAll(`{${k}}`, v);
  return s;
}

export function linguaAttuale() { return lingua; }

// traduce i testi fissi della pagina; restituisce true se la lingua è cambiata
export function impostaLingua(nuova) {
  nuova = nuova === 'en' ? 'en' : 'it';
  if (nuova === lingua) return false;
  lingua = nuova;
  document.documentElement.lang = lingua;
  const giro = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT);
  for (let n = giro.nextNode(); n; n = giro.nextNode()) {
    if (!originali.has(n)) originali.set(n, n.nodeValue);
    const o = originali.get(n), chiave = o.trim();
    if (!chiave || EN[chiave] === undefined) continue;
    n.nodeValue = lingua === 'en' ? o.replace(chiave, EN[chiave]) : o;
  }
  for (const el of document.querySelectorAll('[title]')) {
    if (!el.dataset.titoloIt) el.dataset.titoloIt = el.title;
    el.title = t(el.dataset.titoloIt);
  }
  return true;
}

// Pagina web di Plekto: carica le arcate, sceglie i 3 punti del retainer, mostra il filo e controlla la macchina.
import * as THREE from 'three';
import { STLLoader } from 'three/addons/loaders/STLLoader.js';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { mergeVertices } from 'three/addons/utils/BufferGeometryUtils.js';
import { t, t as tr, impostaLingua } from './lingua.js';

const $ = (s, el = document) => el.querySelector(s);
const $$ = (s, el = document) => [...el.querySelectorAll(s)];
const NOMI = { sup: 'superiore', inf: 'inferiore' };
const COLORI_PUNTI = ['#0071e3', '#af52de', '#ff9500'];
const PASSI = ['Clicca il dente di inizio', 'Clicca il dente di fine', 'Clicca il punto centrale'];

function avviso(testo, ms = 3200) {
  const a = $('#avviso');
  a.textContent = t(testo);
  a.classList.add('su');
  clearTimeout(avviso.t);
  avviso.t = setTimeout(() => a.classList.remove('su'), ms);
}

async function api(metodo, url, corpo, tipo = 'application/json') {
  const opz = { method: metodo, headers: {} };
  if (corpo !== undefined) {
    opz.body = tipo === 'application/json' ? JSON.stringify(corpo) : corpo;
    opz.headers['Content-Type'] = tipo;
  }
  const r = await fetch(url, opz);
  const d = await r.json().catch(() => ({}));
  if (!r.ok) throw new Error(d.detail || `Errore ${r.status}`);
  return d;
}

// ================================================================================================ schede
$$('.schede button').forEach(b => b.addEventListener('click', () => {
  $$('.schede button').forEach(x => x.classList.toggle('attivo', x === b));
  $$('.scheda').forEach(s => s.classList.toggle('attiva', s.id === b.dataset.scheda));
  if (b.dataset.scheda === 'progetta') ridimensiona();
  if (b.dataset.scheda === 'sistema') caricaSistema();
}));

// ================================================================================================ scena 3D
const tela = $('#tela');
const renderer = new THREE.WebGLRenderer({ canvas: tela, antialias: true, alpha: true });
renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
const scena = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(35, 1, 0.5, 2000);
camera.position.set(0, -120, 80);
camera.up.set(0, 0, 1);
const controlli = new OrbitControls(camera, tela);
controlli.enableDamping = true;
controlli.dampingFactor = 0.12;
scena.add(new THREE.HemisphereLight(0xffffff, 0x8a8f99, 1.6));
const luceCamera = new THREE.DirectionalLight(0xffffff, 1.8);
luceCamera.position.set(0.3, 0.5, 1);
camera.add(luceCamera);
scena.add(camera);

const materialeDente = new THREE.MeshStandardMaterial({ color: 0xf1ece2, roughness: 0.55, metalness: 0 });
const materialeAntagonista = new THREE.MeshStandardMaterial({ color: 0xc9ccd3, roughness: 0.6, transparent: true, opacity: 0.28, depthWrite: false });
const materialeFilo = new THREE.MeshStandardMaterial({ color: 0xd4d8de, roughness: 0.25, metalness: 0.9 });
const materialeContatto = new THREE.MeshBasicMaterial({ color: 0xff3b30 });

const stato = {
  mesh: {},                    // lato -> THREE.Mesh
  info: {},                    // lato -> dati dal server (misure, facce)
  visibile: { sup: true, inf: true },
  scelta: null,                // 'sup' | 'inf' | 'entrambi'
  attivo: null,                // arcata su cui si cliccano i punti
  punti: { sup: [], inf: [] },
  progetto: { sup: null, inf: null },
  oggetti: { sup: new THREE.Group(), inf: new THREE.Group() },   // punti, filo, contatti
  occlusaleSu: true,
};
scena.add(stato.oggetti.sup, stato.oggetti.inf);

function ridimensiona() {
  const r = tela.parentElement.getBoundingClientRect();
  if (!r.width) return;
  renderer.setSize(r.width, r.height, false);
  camera.aspect = r.width / r.height;
  camera.updateProjectionMatrix();
}
new ResizeObserver(ridimensiona).observe(tela.parentElement);
renderer.setAnimationLoop(() => { controlli.update(); renderer.render(scena, camera); });

function aggiornaMateriali() {
  for (const lato of ['sup', 'inf']) {
    const m = stato.mesh[lato];
    if (!m) continue;
    const antag = stato.attivo && lato !== stato.attivo;
    m.material = antag ? materialeAntagonista : materialeDente;
    m.renderOrder = antag ? 2 : 0;
    m.visible = stato.visibile[lato];
  }
}

function riquadro() {
  const box = new THREE.Box3();
  for (const lato in stato.mesh) if (stato.mesh[lato].visible) box.expandByObject(stato.mesh[lato]);
  return box;
}

function adatta(direzione) {
  const box = riquadro();
  if (box.isEmpty()) return;
  const c = box.getCenter(new THREE.Vector3());
  const r = box.getSize(new THREE.Vector3()).length() / 2;
  const d = r / Math.sin(THREE.MathUtils.degToRad(camera.fov / 2)) * 0.9;
  const dir = direzione || camera.position.clone().sub(controlli.target).normalize();
  controlli.target.copy(c);
  camera.position.copy(c).addScaledVector(dir, d);
  camera.near = d / 50;
  camera.far = d * 10;
  camera.updateProjectionMatrix();
}

// asse del piano occlusale: autovettore di minima varianza dei vertici (Jacobi 3x3)
function assiPrincipali(geo) {
  const p = geo.attributes.position, n = p.count, passo = Math.max(1, Math.floor(n / 20000));
  const m = [0, 0, 0];
  let k = 0;
  for (let i = 0; i < n; i += passo, k++) { m[0] += p.getX(i); m[1] += p.getY(i); m[2] += p.getZ(i); }
  m[0] /= k; m[1] /= k; m[2] /= k;
  const C = [[0, 0, 0], [0, 0, 0], [0, 0, 0]];
  for (let i = 0; i < n; i += passo) {
    const v = [p.getX(i) - m[0], p.getY(i) - m[1], p.getZ(i) - m[2]];
    for (let a = 0; a < 3; a++) for (let b = 0; b < 3; b++) C[a][b] += v[a] * v[b];
  }
  const V = [[1, 0, 0], [0, 1, 0], [0, 0, 1]];
  for (let giro = 0; giro < 30; giro++) {
    for (const [a, b] of [[0, 1], [0, 2], [1, 2]]) {
      if (Math.abs(C[a][b]) < 1e-9) continue;
      const th = 0.5 * Math.atan2(2 * C[a][b], C[b][b] - C[a][a]), c = Math.cos(th), s = Math.sin(th);
      for (let i = 0; i < 3; i++) { const x = C[i][a], y = C[i][b]; C[i][a] = c * x - s * y; C[i][b] = s * x + c * y; }
      for (let i = 0; i < 3; i++) { const x = C[a][i], y = C[b][i]; C[a][i] = c * x - s * y; C[b][i] = s * x + c * y; }
      for (let i = 0; i < 3; i++) { const x = V[i][a], y = V[i][b]; V[i][a] = c * x - s * y; V[i][b] = s * x + c * y; }
    }
  }
  const ordine = [0, 1, 2].sort((a, b) => C[b][b] - C[a][a]);
  return { centro: new THREE.Vector3(...m), assi: ordine.map(j => new THREE.Vector3(V[0][j], V[1][j], V[2][j])) };
}

function vistaOcclusale() {
  const lato = stato.attivo || Object.keys(stato.mesh)[0];
  if (!lato) return;
  const { centro, assi } = assiPrincipali(stato.mesh[lato].geometry);
  let dir = assi[2].clone();
  const altro = stato.mesh[lato === 'sup' ? 'inf' : 'sup'];
  if (altro) {                                   // dal lato dell'antagonista si vede la faccia occlusale
    const ca = assiPrincipali(altro.geometry).centro;
    if (ca.sub(centro).dot(dir) < 0) dir.negate();
  }
  if (!stato.occlusaleSu) dir.negate();
  stato.occlusaleSu = !stato.occlusaleSu;
  camera.up.copy(assi[1]);
  adatta(dir);
}
$('#vistaOcclusale').addEventListener('click', vistaOcclusale);
$('#vistaAdatta').addEventListener('click', () => adatta());
$$('[data-vedi]').forEach(b => b.addEventListener('click', () => {
  const lato = b.dataset.vedi;
  stato.visibile[lato] = !stato.visibile[lato];
  b.classList.toggle('acceso', stato.visibile[lato]);
  stato.oggetti[lato].visible = stato.visibile[lato];
  aggiornaMateriali();
}));

// ================================================================================================ arcate
const caricatore = new STLLoader();

function mostraArcata(lato, buffer) {
  let geo = caricatore.parse(buffer);
  geo.deleteAttribute('normal');
  geo = mergeVertices(geo, 1e-4);
  geo.computeVertexNormals();
  if (stato.mesh[lato]) { scena.remove(stato.mesh[lato]); stato.mesh[lato].geometry.dispose(); }
  const m = new THREE.Mesh(geo, materialeDente);
  m.userData.lato = lato;
  stato.mesh[lato] = m;
  scena.add(m);
}

function aggiornaTessera(lato, info, caricando) {
  const t = $(`.arcata[data-lato="${lato}"]`);
  t.classList.toggle('caricata', !!info);
  t.classList.toggle('carica', !!caricando);
  $('.dett', t).textContent = caricando ? tr('Caricamento') : info
    ? `${info.misure.slice(0, 2).join(' × ')} mm · ${tr('{n}k facce', { n: (info.facce / 1000).toFixed(0) })}`
    : tr("Trascina l'STL o fai clic");
}

async function caricaFile(lato, file) {
  if (!/\.stl$/i.test(file.name)) return avviso('Serve un file .stl');
  aggiornaTessera(lato, null, true);
  try {
    const buffer = await file.arrayBuffer();
    const info = await api('PUT', `/api/arcata/${lato}`, buffer, 'application/octet-stream');
    mostraArcata(lato, buffer);
    stato.info[lato] = info;
    aggiornaTessera(lato, info);
    azzera(lato);
    aggiornaScelte();
    adatta();
  } catch (e) {
    aggiornaTessera(lato, stato.info[lato]);
    avviso(e.message);
  }
}

$$('.arcata').forEach(t => {
  const lato = t.dataset.lato, input = $('input', t);
  input.addEventListener('change', () => input.files[0] && caricaFile(lato, input.files[0]));
  t.addEventListener('dragover', e => { e.preventDefault(); t.classList.add('sopra'); });
  t.addEventListener('dragleave', () => t.classList.remove('sopra'));
  t.addEventListener('drop', e => {
    e.preventDefault(); e.stopPropagation(); t.classList.remove('sopra');
    if (e.dataTransfer.files[0]) caricaFile(lato, e.dataTransfer.files[0]);
  });
  $('.togli', t).addEventListener('click', async e => {
    e.preventDefault(); e.stopPropagation();
    await api('DELETE', `/api/arcata/${lato}`).catch(() => {});
    if (stato.mesh[lato]) { scena.remove(stato.mesh[lato]); stato.mesh[lato].geometry.dispose(); delete stato.mesh[lato]; }
    delete stato.info[lato];
    azzera(lato);
    aggiornaTessera(lato, null);
    aggiornaScelte();
  });
});

// trascinando sulla vista: due file = superiore e inferiore (dal nome, altrimenti in ordine); uno = primo lato libero
const vista = $('#vista');
vista.addEventListener('dragover', e => { e.preventDefault(); vista.classList.add('sopra'); });
vista.addEventListener('dragleave', e => { if (!vista.contains(e.relatedTarget)) vista.classList.remove('sopra'); });
vista.addEventListener('drop', e => {
  e.preventDefault(); vista.classList.remove('sopra');
  const files = [...e.dataTransfer.files].filter(f => /\.stl$/i.test(f.name)).slice(0, 2);
  const indovina = f => /(inf|low|mand)/i.test(f.name) ? 'inf' : /(sup|upp|max)/i.test(f.name) ? 'sup' : null;
  const liberi = ['sup', 'inf'].filter(l => !stato.mesh[l]);
  for (const f of files) {
    const lato = indovina(f) || liberi.shift() || 'sup';
    caricaFile(lato, f);
  }
});

// ================================================================================================ scelta del retainer
function aggiornaScelte() {
  const c = $('#sceltaRetainer');
  const ha = { sup: !!stato.mesh.sup, inf: !!stato.mesh.inf };
  $('[data-r="sup"]', c).disabled = !ha.sup;
  $('[data-r="inf"]', c).disabled = !ha.inf;
  $('[data-r="entrambi"]', c).disabled = !(ha.sup && ha.inf);
  if (stato.scelta && $(`[data-r="${stato.scelta}"]`, c).disabled) stato.scelta = null;
  if (!stato.scelta) stato.scelta = ha.sup ? 'sup' : ha.inf ? 'inf' : null;
  $$('button', c).forEach(b => b.classList.toggle('attivo', b.dataset.r === stato.scelta));
  const lati = stato.scelta === 'entrambi' ? ['sup', 'inf'] : stato.scelta ? [stato.scelta] : [];
  if (!lati.includes(stato.attivo)) stato.attivo = lati[0] || null;
  for (const l of ['sup', 'inf']) stato.oggetti[l].visible = lati.includes(l) && stato.visibile[l];
  const la = $('#latoAttivo');
  la.innerHTML = '';
  if (lati.length > 1) for (const l of lati) {
    const b = document.createElement('button');
    b.textContent = t(l === 'sup' ? 'Sup' : 'Inf');
    b.classList.toggle('attivo', l === stato.attivo);
    b.addEventListener('click', () => { stato.attivo = l; aggiornaScelte(); });
    la.append(b);
  } else if (lati[0]) la.textContent = t(NOMI[lati[0]]);
  aggiornaMateriali();
  aggiornaPassi();
  mostraVerifica();
}
$$('#sceltaRetainer button').forEach(b => b.addEventListener('click', () => { stato.scelta = b.dataset.r; aggiornaScelte(); }));

function aggiornaPassi() {
  const n = stato.attivo ? stato.punti[stato.attivo].length : -1;
  $$('#passi li').forEach((li, i) => { li.classList.toggle('fatto', i < n); li.classList.toggle('ora', i === n); });
  const aiuto = $('#aiutoVista');
  if (!Object.keys(stato.mesh).length) aiuto.textContent = t('Trascina qui gli STL delle arcate');
  else if (n >= 0 && n < 3) aiuto.textContent = `${t(PASSI[n])} · ${t('arcata ' + NOMI[stato.attivo])}`;
  else aiuto.textContent = '';
  tela.classList.toggle('scegli', n >= 0 && n < 3);
}

function azzera(lato) {
  stato.punti[lato] = [];
  stato.progetto[lato] = null;
  stato.oggetti[lato].clear();
  aggiornaPassi();
  mostraVerifica();
}
$('#ricomincia').addEventListener('click', () => stato.attivo && azzera(stato.attivo));

// ================================================================================================ clic sui denti
const raggio = new THREE.Raycaster();
let giu = null;
let trascina = null;                                   // punto che si sta spostando: {lato, i}

function mira(e) {
  const r = tela.getBoundingClientRect();
  raggio.setFromCamera(new THREE.Vector2((e.clientX - r.left) / r.width * 2 - 1, -(e.clientY - r.top) / r.height * 2 + 1), camera);
}
function marcatoreSotto(e) {                           // un punto già messo sotto il puntatore?
  const lato = stato.attivo;
  if (!lato) return null;
  mira(e);
  const m = stato.oggetti[lato].children.filter(o => o.userData.indice !== undefined);
  const colpo = raggio.intersectObjects(m)[0];
  return colpo ? { lato, i: colpo.object.userData.indice } : null;
}

tela.addEventListener('pointerdown', e => {
  giu = [e.clientX, e.clientY];
  trascina = marcatoreSotto(e);
  if (trascina) {                                      // si sposta il punto, non si ruota la vista
    controlli.enabled = false;
    tela.setPointerCapture(e.pointerId);
  }
});

tela.addEventListener('pointermove', e => {
  if (!trascina) {
    tela.classList.toggle('sposta', !!(e.buttons === 0 && marcatoreSotto(e)));
    return;
  }
  mira(e);
  const colpo = raggio.intersectObject(stato.mesh[trascina.lato])[0];
  if (!colpo) return;
  stato.punti[trascina.lato][trascina.i] = colpo.point.toArray();
  const m = stato.oggetti[trascina.lato].children.find(o => o.userData.indice === trascina.i);
  if (m) m.position.copy(colpo.point);
  if (stato.punti[trascina.lato].length === 3) ricalcola(trascina.lato, true);
});

tela.addEventListener('pointerup', e => {
  if (trascina) {                                      // fine dello spostamento: calcolo completo
    const lato = trascina.lato;
    trascina = null;
    controlli.enabled = true;
    if (stato.punti[lato].length === 3) ricalcola(lato, false);
    else disegnaPunti(lato);
    return;
  }
  if (!giu || Math.hypot(e.clientX - giu[0], e.clientY - giu[1]) > 4) return;   // era una rotazione
  const lato = stato.attivo;
  if (!lato || !stato.mesh[lato] || stato.punti[lato].length >= 3) return;
  mira(e);
  const colpo = raggio.intersectObject(stato.mesh[lato])[0];
  if (!colpo) return;
  stato.punti[lato].push(colpo.point.toArray());
  disegnaPunti(lato);
  aggiornaPassi();
  if (stato.punti[lato].length === 3) progetta(lato);
});

// ricalcolo mentre si trascina: una richiesta alla volta, l'ultima posizione vince
const inCalcolo = {};
function ricalcola(lato, rapido) {
  if (inCalcolo[lato]) { inCalcolo[lato].ancora = rapido ? 'rapido' : 'completo'; return; }
  inCalcolo[lato] = {};
  progetta(lato, rapido).finally(() => {
    const ancora = inCalcolo[lato].ancora;
    inCalcolo[lato] = null;
    if (ancora) ricalcola(lato, ancora === 'rapido' && trascina !== null);
  });
}

function sfera(pos, colore, r) {
  const s = new THREE.Mesh(new THREE.SphereGeometry(r, 24, 16), new THREE.MeshStandardMaterial({ color: colore, roughness: 0.3 }));
  s.position.set(...pos);
  return s;
}

function disegnaPunti(lato) {
  const g = stato.oggetti[lato];
  g.clear();
  stato.punti[lato].forEach((p, i) => {
    const s = sfera(p, COLORI_PUNTI[i], 0.45);
    s.userData.indice = i;                             // si può trascinare
    g.add(s);
  });
  const prog = stato.progetto[lato];
  if (!prog) return;
  const percorso = new THREE.CurvePath();
  const v = prog.spezzata.map(p => new THREE.Vector3(...p));
  for (let i = 1; i < v.length; i++) percorso.add(new THREE.LineCurve3(v[i - 1], v[i]));
  g.add(new THREE.Mesh(new THREE.TubeGeometry(percorso, Math.max(64, v.length * 24), 0.2223, 14, false), materialeFilo));
  for (const p of [v[0], v[v.length - 1]]) g.add(new THREE.Mesh(new THREE.SphereGeometry(0.2223, 16, 12), materialeFilo).translateX(p.x).translateY(p.y).translateZ(p.z));
  for (const c of prog.contatti) {
    const s = new THREE.Mesh(new THREE.SphereGeometry(0.35, 16, 12), materialeContatto);
    s.position.set(...c.punto);
    g.add(s);
  }
}

async function progetta(lato, rapido = false) {
  if (!rapido) $('#aiutoVista').textContent = t('Calcolo del retainer…');
  try {
    stato.progetto[lato] = await api('POST', `/api/progetta/${lato}`, { punti: stato.punti[lato], rapido });
  } catch (e) {
    if (rapido) return;                                // durante il trascinamento: si aspetta la posizione buona
    stato.progetto[lato] = null;
    avviso(e.message, 5000);
  }
  disegnaPunti(lato);
  aggiornaPassi();
  mostraVerifica();
}

// ================================================================================================ verifica
function mostraVerifica() {
  const lato = stato.attivo, p = lato && stato.progetto[lato];
  $('#cartaVerifica').hidden = !p;
  if (!p) return;
  $('#nLunghezza').textContent = `${p.lunghezza.toFixed(1)} mm`;
  $('#nPieghe').textContent = p.n_pieghe;
  $('#nScarto').textContent = `${p.scarto_max.toFixed(2)} mm`;
  $('#nGioco').textContent = p.gioco_min == null ? '–' : `${p.gioco_min.toFixed(2)} mm`;
  const es = $('#esito');
  es.className = 'esito';
  if (!p.antagonista) {
    es.innerHTML = `<b>${t('Contatti non controllati')}</b>${t("Carica anche l'arcata antagonista in occlusione.")}`;
  } else if (!p.contatti.length) {
    es.classList.add('ok');
    es.innerHTML = `<b>${t("Nessun contatto con l'antagonista")}</b>${t('Gioco minimo {g} mm dalla superficie del filo.', { g: p.gioco_min.toFixed(2) })}`;
  } else {
    es.classList.add('male');
    const sug = !p.suggerimento ? t('Rilascia il punto per la correzione proposta.')
      : p.suggerimento.sposta_mm != null
      ? t('Sposta il retainer di {s} mm verso la gengiva: spariscono i contatti.', { s: p.suggerimento.sposta_mm.toFixed(2) })
      : t('Contatti anche spostando di 3 mm: cambia inclinazione (punto centrale) o accorcia il retainer.');
    es.innerHTML = `<b>${t("Contatto con l'antagonista")}</b>${sug}`;
    if (p.suggerimento && p.suggerimento.vettore) {
      const b = document.createElement('button');
      b.className = 'bottone primario';
      b.textContent = t('Sposta di {s} mm', { s: p.suggerimento.sposta_mm.toFixed(2) });
      b.addEventListener('click', () => {
        const v = p.suggerimento.vettore;
        stato.punti[lato] = stato.punti[lato].map(q => q.map((x, i) => x + v[i]));
        progetta(lato);
      });
      es.append(b);
    }
  }
  $('#scaricaGcode').href = `/api/gcode/${lato}`;
}

$('#invia').addEventListener('click', async () => {
  try {
    const r = await api('POST', `/api/invia/${stato.attivo}`);
    avviso(r.ok ? 'Retainer inviato alla macchina' : r.motivo, 5000);
  } catch (e) { avviso(e.message); }
});

// ================================================================================================ macchina
let ultimoStato = null;
function durata(s) {
  const h = Math.floor(s / 3600), m = Math.floor(s / 60) % 60;
  return h ? `${h} h ${m} min` : `${m} min`;
}
function righe(el, voci) {
  el.innerHTML = voci.map(([a, b]) => `<div><dt>${a}</dt><dd>${b}</dd></div>`).join('');
}

async function aggiornaMacchina() {
  let s;
  try { s = await api('GET', '/api/macchina'); } catch { s = { collegata: false, esp: {}, pi: {} }; }
  ultimoStato = s;
  const on = s.collegata;
  $('#statoBarra').classList.toggle('on', on);
  if (impostaLingua(s.lingua)) {                     // lingua scelta sul display della macchina
    aggiornaScelte();
    for (const l of ['sup', 'inf']) aggiornaTessera(l, stato.info[l]);
  }
  $('#statoBarra span').textContent = t(on ? 'Macchina collegata' : 'Macchina non collegata');
  $('#mSotto').textContent = t(on ? 'Collegata via USB' : 'Non collegata: controlla il cavo USB');
  const e = s.esp || {};
  const segnale = e.rssi ? t(e.rssi > -60 ? 'ottimo' : e.rssi > -70 ? 'buono' : 'debole') + ` (${e.rssi} dBm)` : '–';
  righe($('#mRighe'), [
    ['Firmware', e.fw || '–'],
    ['Wi-Fi', s.wifi ? `${s.wifi}${e.ip ? ' · ' + e.ip : ''}` : t('non configurato')],
    [t('Segnale'), segnale],
    [t('Rotella'), e.ano === undefined ? '–' : t(e.ano ? 'collegata' : 'non trovata')],
    [t('Acceso da'), e.s ? durata(e.s) : '–'],
  ]);
  $('#mFirmware').textContent = e.fw ? t('Versione installata {v}', { v: e.fw }) : '–';
  $$('#lingua button').forEach(b => b.classList.toggle('attivo', b.dataset.l === s.lingua));
  if (document.activeElement !== $('#luci') && e.luci !== undefined) {
    $('#luci').value = e.luci;
    $('#luciVal').textContent = `${e.luci} %`;
  }
  $$('#macchina button').forEach(b => { if (!b.closest('#lingua, .disattiva')) b.disabled = !on; });
  $('#luci').disabled = !on;
}
setInterval(aggiornaMacchina, 2000);
aggiornaMacchina();

async function comando(c, extra = {}) {
  try { await api('POST', '/api/macchina/comando', { c, ...extra }); } catch (e) { avviso(e.message); }
}
let luciTimer;
$('#luci').addEventListener('input', e => {
  $('#luciVal').textContent = `${e.target.value} %`;
  clearTimeout(luciTimer);
  luciTimer = setTimeout(() => comando('luci', { v: +e.target.value }), 120);
});
$('#provaSuono').addEventListener('click', () => comando('suono', { hz: 1760, ms: 150 }));
$('#aggEsp').addEventListener('click', () => { comando('agg'); avviso('Controllo in corso: guarda il display della macchina'); });
$('#riavviaEsp').addEventListener('click', () => { if (confirm(t('Riavviare la macchina?'))) comando('riavvia'); });
$$('#lingua button').forEach(b => b.addEventListener('click', async () => {
  try { await api('POST', '/api/impostazioni', { lingua: b.dataset.l }); aggiornaMacchina(); } catch (e) { avviso(e.message); }
}));

// ================================================================================================ sistema
async function caricaSistema() {
  const pi = (ultimoStato && ultimoStato.pi) || {};
  righe($('#sRighe'), [[t('Versione'), pi.versione || '–'], [t('Indirizzo'), pi.ip ? `http://${pi.ip}` : '–']]);
}
$('#sControlla').addEventListener('click', async () => {
  $('#sNota').textContent = t('Controllo…');
  try {
    const r = await api('GET', '/api/sistema/aggiornamenti');
    righe($('#sRighe'), [[t('Versione installata'), r.attuale], [t('Ultima versione'), r.ultima || '–'],
      ...(r.ultimo_esito ? [[t('Ultimo aggiornamento'), `${r.ultimo_esito.esito} · ${r.ultimo_esito.quando}`]] : [])]);
    $('#sAggiorna').hidden = !r.nuova;
    $('#sAggiorna').textContent = t('Aggiorna a {v}', { v: r.ultima });
    $('#sNota').textContent = t(r.nuova ? 'È disponibile una versione nuova.' : 'Il software è aggiornato.');
  } catch (e) { $('#sNota').textContent = t(e.message); }
});
$('#sAggiorna').addEventListener('click', async () => {
  const prima = (await api('GET', '/api/salute').catch(() => ({}))).versione;
  try { await api('POST', '/api/sistema/aggiorna'); } catch (e) { return avviso(e.message); }
  $('#sAggiorna').disabled = true;
  $('#sNota').textContent = t('Aggiornamento in corso: non spegnere. La pagina si ricarica da sola.');
  const inizio = Date.now();
  const t = setInterval(async () => {
    const s = await api('GET', '/api/salute').catch(() => null);
    if (s && s.versione !== prima) { clearInterval(t); location.reload(); }
    if (Date.now() - inizio > 5 * 60000) { clearInterval(t); $('#sNota').textContent = t('Aggiornamento non riuscito: è rimasta la versione di prima.'); $('#sAggiorna').disabled = false; }
  }, 3000);
});

// ================================================================================================ avvio
(async () => {
  ridimensiona();
  const elenco = await api('GET', '/api/arcate').catch(() => ({}));
  for (const lato of Object.keys(elenco)) {
    aggiornaTessera(lato, null, true);
    const buffer = await (await fetch(`/api/arcata/${lato}.stl`)).arrayBuffer();
    mostraArcata(lato, buffer);
    stato.info[lato] = elenco[lato];
    aggiornaTessera(lato, elenco[lato]);
  }
  aggiornaScelte();
  adatta(new THREE.Vector3(0, -0.6, 0.8).normalize());
})();

window.plekto = { stato, camera, progetta, disegnaPunti, THREE };   // per le prove dalla console del browser

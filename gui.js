// gui.js - Eagle Script GUI (Shqip mbi eaglegui / Qt)
//
// Sintaksa eshte IDENTIKE me gui.js te bazuar ne @nodegui/nodegui: te njejtat
// funksione (krijo_dritare, buton_i_ri, ...), te njejtat metoda (.vendos_stil,
// .kur_klikohet, ...) dhe te njejtat eksporte (EMRAT_GUI, _dritaretAktive).
// Skriptet .es dhe interpretuesi nuk duhet te ndryshojne fare.
//
// Ndryshimi vetem ne backend: ketu nuk duhet 'qode' dhe nuk duhet @nodegui.
// Funksionon me Node te zakonshem, mbi addon-in native 'eaglegui' (Qt Widgets).
//
// Cikli i ngjarjeve niset VETE kur shfaqet dritarja e pare (shfaqe/ekran_i_plote/
// maksimizo) dhe ndalon kur mbyllet dritarja e fundit - pa thirrur asgje shtese.
//
// Moduli native ngarkohet VONE (kur therritet per here te pare nje funksion GUI),
// keshtu skriptet CLI nuk varen fare nga Qt dhe niset me shpejt.

'use strict';

let _nativ = null;
function nativ() {
    if (_nativ) return _nativ;
    try {
        _nativ = require('./build/Release/eaglegui_native.node');
    } catch (e) {
        throw new Error(
            "Moduli native 'eaglegui' nuk u ngarkua. Ndertoje me: npm run nderto " +
            "(kerkon Qt dhe CMake). Detaje: " + e.message
        );
    }
    _nativ.fillo();
    return _nativ;
}

// Krijon nje element native te llojit te dhene.
function el(tipi) { return new (nativ().Elementi)(tipi); }

// Metoda e vertete 'fshih' e addon-it (widget.fshih mbishkruhet nga sintaksa shqipe me poshte).
function fshihNativ(widget) { nativ().Elementi.prototype.fshih.call(widget); }

// Regjistri i dritareve aktive: mban referencat qe GC te mos shkaterroje objektet Qt.
const _dritaretAktive = new Set();

// Cdo widget qe krijojme futet ketu qe te mos mblidhet nga GC sa kohe ekziston dritarja.
const _mbajtes = new Set();
function mbaj(w) { _mbajtes.add(w); return w; }

// ─── Cikli i ngjarjeve (zevendeson 'qode') ──────────────────────────────────
// Qt ushqehet nga Node me nje kohematues; ndalon vetem kur s'ka me dritare te dukshme.
let _pompa = null;
function siguroPompen() {
    if (_pompa) return;
    _pompa = setInterval(() => {
        let kaDritare = false;
        try {
            kaDritare = nativ().procesoNgjarjet();
        } catch (e) {
            clearInterval(_pompa); _pompa = null;
            throw e;
        }
        if (!kaDritare) {
            clearInterval(_pompa); _pompa = null;
            _dritaretAktive.clear();
        }
    }, 8);
}

const RRESHTIMET = { qender: 'AlignCenter', majtas: 'AlignLeft', djathtas: 'AlignRight' };

// ─── Metoda te perbashketa per CDO widget ───────────────────────────────────
function baze(widget) {
    widget.vendos_stil = (css) => { widget.vendosStilin(css); return widget; };
    widget.vendos_madhesi = (w, h) => { widget.vendosMadhesineMin(w, h); return widget; };
    widget.vendos_madhesi_fikse = (w, h) => { widget.vendosMadhesineFikse(w, h); return widget; };
    widget.vendos_madhesi_maksimale = (w, h) => { widget.vendosMadhesineMaks(w, h); return widget; };
    widget.vendos_focus = () => { try { widget.vendosFokus(); } catch (e) {} return widget; };
    widget.vendos_id = (emri) => { widget.vendosId(emri); return widget; };
    widget.vendos_keshille = (t) => { widget.vendosKeshille(t); return widget; };
    widget.aktivizo = (po = true) => { widget.vendosAktiv(!!po); return widget; };
    widget.fshih = () => { fshihNativ(widget); return widget; };
    widget.shfaq_elementin = () => { widget.shfaq(); return widget; };
    widget.vendos_kursor_dore = () => { widget.kursorDore(); return widget; };

    // Tastiera: funksioni merr { tasti, teksti } (kodi i tastit dhe karakteri).
    widget.kur_shtypet = (funksioni) => {
        widget.ne('tast', (ev) => funksioni({ tasti: ev.tasti, teksti: ev.teksti }));
        return widget;
    };

    // Klikimi mbi cdo widget (p.sh. nje karte). Butonat e mbishkruajne me sinjalin 'clicked'.
    if (!widget.kur_klikohet) {
        widget.kur_klikohet = (funksioni) => {
            widget.ne('liberim_mausi', () => funksioni());
            widget.kursorDore();
            return widget;
        };
    }
    return mbaj(widget);
}

// Metoda per elemente me tekst (etikete, buton)
function metodaTekst(w) {
    w.vendos_tekst = (t) => { w.vendosTekstin(String(t)); return w; };
    return w;
}

// ─── Dritarja kryesore ──────────────────────────────────────────────────────
function krijo_dritare(titulli = 'Eagle Script App', gjeresia = 500, lartesia = 400) {
    const dritarja = el('dritare_kryesore');
    dritarja.vendosTitullin(String(titulli));
    dritarja.vendosMadhesine(gjeresia, lartesia);

    let rreshtiTjeter = 0;

    // .shto(el)  -> rreshti tjeter i lire;  .shto(el, rresht, kolone, rreshtaShtrire, kolonaShtrire) -> pozicion i sakte
    dritarja.shto = (element, rreshti, kolona, rS, kS) => {
        if (rreshti === undefined) {
            dritarja.shtoNeRrjet(element, rreshtiTjeter, 0, 1, 1);
            rreshtiTjeter++;
        } else {
            const rr = rS === undefined ? 1 : rS;
            dritarja.shtoNeRrjet(element, rreshti, kolona === undefined ? 0 : kolona, rr, kS === undefined ? 1 : kS);
            rreshtiTjeter = Math.max(rreshtiTjeter, rreshti + rr);
        }
        return dritarja;
    };

    // .vendos_permbajtjen(widget) -> widget-i zë gjithë dritaren (p.sh. 'ekrane()' ose 'skeda()').
    dritarja.vendos_permbajtjen = (widget) => {
        dritarja.vendosQendren(widget);
        return dritarja;
    };

    dritarja.shfaqe = () => {
        dritarja.shfaq(); _dritaretAktive.add(dritarja); siguroPompen(); return dritarja;
    };
    dritarja.mbylle = () => { _dritaretAktive.delete(dritarja); dritarja.mbyll(); return dritarja; };
    dritarja.ekran_i_plote = () => {
        dritarja.ekranIPlote(); _dritaretAktive.add(dritarja); siguroPompen(); return dritarja;
    };
    dritarja.maksimizo = () => {
        dritarja.maksimizoDritaren(); _dritaretAktive.add(dritarja); siguroPompen(); return dritarja;
    };
    dritarja.vendos_titull = (t) => { dritarja.vendosTitullin(String(t)); return dritarja; };
    dritarja.vendos_stil = (css) => { dritarja.vendosStilin(css); return dritarja; };

    // Ndryshon permasat; minimumi mbetet i arsyeshem qe dritarja te mos bllokohet.
    dritarja.vendos_madhesi = (w, h) => {
        dritarja.vendosMadhesine(w, h);
        dritarja.vendosMadhesineMin(Math.max(200, Math.floor(w * 0.5)), Math.max(200, Math.floor(h * 0.5)));
        return dritarja;
    };

    dritarja.kur_shtypet = (funksioni) => {
        dritarja.ne('tast', (ev) => funksioni({ tasti: ev.tasti, teksti: ev.teksti }));
        return dritarja;
    };

    // Kur dritarja mbyllet (edhe me X), e heqim nga regjistri.
    dritarja.ne('mbyllje', () => { _dritaretAktive.delete(dritarja); });
    return dritarja;
}

// ─── Elementet bazë ─────────────────────────────────────────────────────────
function buton_i_ri(teksti = 'Kliko') {
    const b = el('buton');
    b.vendosTekstin(String(teksti));
    b.kur_klikohet = (f) => { b.ne('klikim', () => f()); return b; };
    metodaTekst(b);
    return baze(b);
}

function tekst_i_ri(permbajtja = '') {
    const e = el('etikete');
    e.vendosTekstin(String(permbajtja));
    metodaTekst(e);
    e.mbulo_tekstin = (po = true) => { e.vendosMbulim(!!po); return e; };
    e.vendos_rreshtim = (emri) => { e.vendosRreshtim(RRESHTIMET[emri] || 'AlignLeft'); return e; };
    return baze(e);
}

function fushe_teksti(placeholder = '') {
    const f = el('fushe');
    f.vendosPlaceholder(String(placeholder));
    f.merr_vleren = () => f.merrTekstin();
    f.vendos_vleren = (v) => { f.vendosTekstin(String(v)); return f; };
    f.kur_ndryshon = (fn) => { f.ne('ndryshim', (t) => fn(t)); return f; };
    f.kur_shtypet_enter = (fn) => { f.ne('enter', () => fn()); return f; };
    f.beje_read_only = (po) => { f.vendosVetemLexim(!!po); return f; };
    return baze(f);
}

function fushe_fjalekalimi(placeholder = '') {
    const f = fushe_teksti(placeholder);
    f.vendosFjalekalim(true);
    f.shfaq_fjalekalimin = (po = true) => { f.vendosFjalekalim(!po); return f; };
    return f;
}

function zone_teksti(placeholder = '') {
    const z = el('zone_teksti');
    z.vendosPlaceholder(String(placeholder));
    z.merr_vleren = () => z.merrTekstin();
    z.vendos_vleren = (v) => { z.vendosTekstin(String(v)); return z; };
    z.kur_ndryshon = (fn) => { z.ne('ndryshim', (t) => fn(t)); return z; };
    return baze(z);
}

function imazh_i_ri(rruga, gjeresia, lartesia) {
    const e = el('etikete');
    e.vendosRreshtim('AlignCenter');

    const gj = (gjeresia && lartesia) ? gjeresia : 0;
    const la = (gjeresia && lartesia) ? lartesia : 0;

    // .vendos_imazh("foto.png")  |  .vendos_imazh_nga_te_dhena(buffer)  (p.sh. PNG i krijuar ne memorie)
    e.vendos_imazh = (r) => {
        if (!e.vendosImazhNgaSkedari(String(r), gj, la)) throw new Error(`Imazhi nuk u hap: ${r}`);
        return e;
    };
    e.vendos_imazh_nga_te_dhena = (buffer, formati) => {
        if (!e.vendosImazhNgaTeDhena(buffer, formati, gj, la)) {
            throw new Error('Te dhenat e imazhit nuk jane te vlefshme.');
        }
        return e;
    };
    if (rruga) e.vendos_imazh(rruga);
    return baze(e);
}

// ─── Kontejnere (rregullim vertikal / horizontal) ───────────────────────────
let _numeruesId = 0;

function ndertoKontejner(drejtimi, kartele) {
    const horizontal = drejtimi === 'horizontal' || drejtimi === 'horizontale';
    const w = el(horizontal ? 'kornize_h' : 'kornize_v');

    const femijet = [];
    // .shto(el, shtrirje)  -> shtrirje > 0 e le elementin te zgjerohet sa me shume
    w.shto = (element, shtrirje = 0) => { w.shtoNeKuti(element, shtrirje); femijet.push(element); return w; };
    w.shto_hapesire = (px) => { w.shtoHapesire(px); return w; };
    w.shto_shtrirje = () => { w.shtoShtrirje(); return w; };
    w.vendos_hapesira = (px) => { w.vendosHapesira(px); return w; };
    // .vendos_margina(16)  |  .vendos_margina(majtas, siper, djathtas, poshte)  |  .vendos_margina([m, s, d, p])
    w.vendos_margina = (majtas, siper = majtas, djathtas = majtas, poshte = siper) => {
        if (Array.isArray(majtas)) [majtas, siper, djathtas, poshte] = majtas;
        w.vendosMargina(majtas, siper, djathtas, poshte);
        return w;
    };
    // Heq elementet e shtuara me .shto (hapesirat/shtrirjet e shtuara mbeten).
    w.pastro = () => {
        while (femijet.length) {
            const element = femijet.pop();
            w.hiqNgaKuti(element);
            fshihNativ(element);
        }
        return w;
    };

    if (kartele) {
        const id = `eagle-karte-${++_numeruesId}`;
        w.vendosId(id);
        w.vendosStilin(
            `#${id} { background-color: white; border: 1px solid #E4E7F0; border-radius: 14px; }`
        );
        w.vendosMargina(14, 14, 14, 14);
    }
    return baze(w);
}

function kontejner(drejtimi = 'vertikal') { return ndertoKontejner(drejtimi, false); }
function karte(drejtimi = 'vertikal') { return ndertoKontejner(drejtimi, true); }

// ─── Ekrane me navigim (si Navigator.push/pop) ──────────────────────────────
function ekrane() {
    const s = el('pirg');
    const faqet = [];
    const histori = [];

    s.shto_ekran = (widget) => { s.shtoNePirg(widget); faqet.push(widget); return faqet.length - 1; };
    // .shfaq_ekranin(numri | widget)  -> kalon te ekrani dhe e kujton ekranin e meparshem
    s.shfaq_ekranin = (x) => {
        const idx = typeof x === 'number' ? x : faqet.indexOf(x);
        if (idx < 0 || idx >= faqet.length) throw new Error('Ekrani nuk ekziston: ' + x);
        const aktual = s.merrIndeksin();
        if (aktual >= 0 && aktual !== idx) histori.push(aktual);
        s.vendosIndeksin(idx);
        return s;
    };
    // --- Navigator me pirg (si Navigator.push / pop ne Flutter) ---
    // .shtyj(widget)  -> shton ekranin e ri dhe e shfaq (ekrani i meparshem mbetet poshte)
    // .terhiq()       -> kthehet te ekrani i meparshem dhe heq ekranin e fundit
    // .zevendeso_rrenjen(widget) -> fshin gjithe pirgun dhe vendos nje ekran te vetem
    const pirg = [];
    s.shtyj = (widget) => {
        s.shtoNePirg(widget);
        pirg.push(widget);
        s.aktivizoNePirg(widget);
        return widget;
    };
    s.terhiq = () => {
        if (pirg.length < 2) return s;
        const i_fundit = pirg.pop();
        s.aktivizoNePirg(pirg[pirg.length - 1]);
        s.hiqNgaPirg(i_fundit);
        fshihNativ(i_fundit);
        return s;
    };
    s.zevendeso_rrenjen = (widget) => {
        while (pirg.length) { const w = pirg.pop(); s.hiqNgaPirg(w); fshihNativ(w); }
        return s.shtyj(widget);
    };
    s.thellesia = () => pirg.length;
    s.kthehu = () => { if (histori.length) s.vendosIndeksin(histori.pop()); return s; };
    s.kthehu_te_fillimi = () => { histori.length = 0; if (faqet.length) s.vendosIndeksin(0); return s; };
    // Fshin historine e navigimit (p.sh. pas hyrjes, qe 'kthehu' te mos kthej te hyrja).
    s.pastro_historine = () => { histori.length = 0; return s; };
    s.ekrani_aktual = () => s.merrIndeksin();
    s.kur_ndryshon = (fn) => { s.ne('ndryshim', (i) => fn(i)); return s; };
    return baze(s);
}

function skeda() {
    const t = el('skeda');
    t.shto_skede = (widget, titulli) => { t.shtoSkede(widget, String(titulli)); return t; };
    t.shko_te = (i) => { t.vendosIndeksin(i); return t; };
    t.skeda_aktuale = () => t.merrIndeksin();
    t.kur_ndryshon = (fn) => { t.ne('ndryshim', (i) => fn(i)); return t; };
    return baze(t);
}

function zone_levizese(permbajtja) {
    const z = el('zone_levizese');
    if (permbajtja) z.vendosWidget(permbajtja);
    z.vendos_permbajtjen = (w) => { z.vendosWidget(w); return z; };
    return baze(z);
}

// ─── Lista, zgjedhes, kuti zgjedhjeje, progres ──────────────────────────────
function liste(elementet = []) {
    const l = el('liste');
    let tekstet = [];

    l.shto_element = (tekst) => {
        l.shtoRresht(String(tekst));
        tekstet.push(String(tekst));
        return l;
    };
    l.shto_elemente = (vargu) => { vargu.forEach((t) => l.shto_element(t)); return l; };
    l.pastro = () => { l.pastroListen(); tekstet = []; return l; };
    l.indeksi_zgjedhur = () => l.merrIndeksin();
    l.teksti_zgjedhur = () => tekstet[l.merrIndeksin()];
    // funksioni merr (indeksi, teksti)
    l.kur_zgjidhet = (fn) => {
        l.ne('zgjedhje', (r) => { if (r >= 0) fn(r, tekstet[r]); });
        return l;
    };
    l.shto_elemente(elementet);
    return baze(l);
}

function zgjedhes(opsionet = []) {
    const c = el('zgjedhes');
    c.shtoOpsione(opsionet.map(String));
    c.shto_opsione = (vargu) => { c.shtoOpsione(vargu.map(String)); return c; };
    c.vlera = () => c.merrTekstin();
    c.indeksi = () => c.merrIndeksin();
    c.vendos_indeksin = (i) => { c.vendosIndeksin(i); return c; };
    c.kur_ndryshon = (fn) => { c.ne('ndryshim', (t) => fn(t)); return c; };
    return baze(c);
}

function kuti_zgjedhjeje(teksti = '') {
    const c = el('kuti');
    c.vendosTekstin(String(teksti));
    c.eshte_zgjedhur = () => c.eshteZgjedhur();
    c.vendos_zgjedhur = (po) => { c.vendosZgjedhur(!!po); return c; };
    c.kur_ndryshon = (fn) => { c.ne('ndryshim', (v) => fn(v)); return c; };
    metodaTekst(c);
    return baze(c);
}

function progres(minimumi = 0, maksimumi = 100) {
    const p = el('progres');
    p.vendosInterval(minimumi, maksimumi);
    p.vendos_vlere = (v) => { p.vendosVlere(v); return p; };
    p.vendos_format = (f) => { p.vendosFormat(String(f)); return p; };
    return baze(p);
}

// ─── Dialogë ────────────────────────────────────────────────────────────────
// Shfaq nje mesazh informues me butonin "Ne rregull".
function shfaq_mesazh(titulli, teksti) {
    nativ().mesazh(String(titulli), String(teksti), 'Ne rregull');
}

// Pyet po/jo dhe kthen true/false.
function pyet(titulli, teksti, tekstiPo = 'Po', tekstiJo = 'Jo') {
    return nativ().pyet(String(titulli), String(teksti), String(tekstiPo), String(tekstiJo));
}

// Hap dialogun e zgjedhjes se skedarit; kthen rrugen ose null.
function zgjidh_skedar(titulli = 'Zgjidh skedarin', filtri = 'Te gjithe (*)') {
    return nativ().zgjidhSkedar(String(titulli), String(filtri));
}

// Emrat e funksioneve GUI (perdoren nga index.js, interpreter.js, packager.js per zbulim/injektim).
const FUNKSIONET = {
    krijo_dritare, buton_i_ri, tekst_i_ri, fushe_teksti, fushe_fjalekalimi, zone_teksti,
    imazh_i_ri, kontejner, karte, ekrane, skeda, zone_levizese, liste, zgjedhes,
    kuti_zgjedhjeje, progres, shfaq_mesazh, pyet, zgjidh_skedar,
};
const EMRAT_GUI = Object.keys(FUNKSIONET);

module.exports = {
    ...FUNKSIONET,
    EMRAT_GUI,
    _dritaretAktive, // vetem per debug/testim
};

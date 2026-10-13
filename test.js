'use strict';
// Test sjelljeje per sintaksen gui.js mbi eaglegui. Ekzekutohet pa ekran (offscreen) ne Linux.
//   npm test

if (!process.env.QT_QPA_PLATFORM && process.platform === 'linux' &&
    !process.env.DISPLAY && !process.env.WAYLAND_DISPLAY) {
    process.env.QT_QPA_PLATFORM = 'offscreen';
}

const assert = require('assert');
const gui = require('./gui.js');
const {
    krijo_dritare, buton_i_ri, tekst_i_ri, fushe_teksti, fushe_fjalekalimi, zone_teksti,
    imazh_i_ri, kontejner, karte, ekrane, skeda, zone_levizese, liste, zgjedhes,
    kuti_zgjedhjeje, progres,
} = gui;

const Qt = { Key_Return: 0x01000004, Key_A: 0x41 };
let numri = 0;
function ok(emri, fn) {
    try { fn(); numri++; console.log('  ok  ' + emri); }
    catch (e) { console.error('  GABIM  ' + emri + '\n    ' + e.message); process.exitCode = 1; }
}

console.log('Funksionet:', gui.EMRAT_GUI.length);
const dritarja = krijo_dritare('Test', 600, 500);

ok('butoni: kur_klikohet / vendos_tekst / aktivizo', () => {
    let klikime = 0;
    const b = buton_i_ri('A').kur_klikohet(() => klikime++);
    b.vendos_tekst('B');
    assert.strictEqual(b.merrTekstin(), 'B');
    b.kliko(); assert.strictEqual(klikime, 1);
    b.aktivizo(false); b.kliko(); assert.strictEqual(klikime, 1, 'butoni i caktivizuar nuk klikohet');
    b.aktivizo(); b.kliko(); assert.strictEqual(klikime, 2);
    b.vendos_madhesi_fikse(100, 30).vendos_keshille('ndihme').vendos_id('b1').vendos_stil('color:red');
    dritarja.shto(b);
});

ok('etiketa: tekst, rreshtim, mbulim', () => {
    const t = tekst_i_ri('Pershendetje 🦅 ë ç').vendos_rreshtim('qender').mbulo_tekstin();
    assert.strictEqual(t.merrTekstin(), 'Pershendetje 🦅 ë ç');
    t.vendos_tekst(42); assert.strictEqual(t.merrTekstin(), '42');
    dritarja.shto(t);
});

ok('fusha: vlera, ndryshim, enter, read-only', () => {
    let e = null, enter = 0;
    const f = fushe_teksti('emri').kur_ndryshon((t) => { e = t; }).kur_shtypet_enter(() => enter++);
    f.vendos_vleren('Arben');
    assert.strictEqual(f.merr_vleren(), 'Arben');
    assert.strictEqual(e, 'Arben');
    f.simuloTast(Qt.Key_Return, '\r');
    assert.strictEqual(enter, 1);
    f.beje_read_only(true);
    dritarja.shto(f);
});

ok('fjalekalimi', () => {
    const f = fushe_fjalekalimi('fjalekalimi');
    f.vendos_vleren('sekret'); assert.strictEqual(f.merr_vleren(), 'sekret');
    f.shfaq_fjalekalimin().shfaq_fjalekalimin(false);
    dritarja.shto(f);
});

ok('zona e tekstit', () => {
    let e = null;
    const z = zone_teksti('shenime').kur_ndryshon((t) => { e = t; });
    z.vendos_vleren('rreshti 1\nrreshti 2');
    assert.strictEqual(z.merr_vleren(), 'rreshti 1\nrreshti 2');
    assert.strictEqual(e, 'rreshti 1\nrreshti 2');
    dritarja.shto(z);
});

ok('imazhi nga te dhena / nga skedari', () => {
    const ppm = Buffer.concat([Buffer.from('P6\n2 2\n255\n'), Buffer.alloc(12, 200)]);
    const im = imazh_i_ri(undefined, 64, 64);
    im.vendos_imazh_nga_te_dhena(ppm, 'PPM');
    assert.throws(() => im.vendos_imazh_nga_te_dhena(Buffer.from('jo-imazh'), 'PNG'), /nuk jane te vlefshme/);
    assert.throws(() => imazh_i_ri('/nuk/ekziston.png'), /Imazhi nuk u hap/);
    dritarja.shto(im);
});

ok('kontejner dhe karte', () => {
    const k = kontejner('horizontal').vendos_hapesira(4).vendos_margina([1, 2, 3, 4]).vendos_margina(5);
    const a = buton_i_ri('1'), b = buton_i_ri('2');
    k.shto(a, 1).shto_hapesire(10).shto(b).shto_shtrirje();
    k.pastro();
    const c = karte();
    c.shto(tekst_i_ri('brenda kartes'));
    assert.ok(c.kur_klikohet(() => {}));
    dritarja.shto(k).shto(c);
});

ok('ekrane: indeks + historia', () => {
    const s = ekrane(); const hyrja = tekst_i_ri('hyrja'), kryefaqja = tekst_i_ri('kryefaqja');
    let nd = [];
    s.kur_ndryshon((i) => nd.push(i));
    assert.strictEqual(s.shto_ekran(hyrja), 0);
    assert.strictEqual(s.shto_ekran(kryefaqja), 1);
    s.shfaq_ekranin(1); assert.strictEqual(s.ekrani_aktual(), 1);
    s.kthehu(); assert.strictEqual(s.ekrani_aktual(), 0);
    s.shfaq_ekranin(kryefaqja); s.kthehu_te_fillimi(); assert.strictEqual(s.ekrani_aktual(), 0);
    assert.throws(() => s.shfaq_ekranin(7), /nuk ekziston/);
    assert.ok(nd.length >= 3, 'kur_ndryshon u therrit');
    dritarja.shto(s);
});

ok('ekrane: navigator me pirg (shtyj/terhiq)', () => {
    const s = ekrane(); const p1 = tekst_i_ri('p1'), p2 = tekst_i_ri('p2'), p3 = tekst_i_ri('p3');
    s.shtyj(p1); s.shtyj(p2);
    assert.strictEqual(s.thellesia(), 2);
    s.terhiq(); assert.strictEqual(s.thellesia(), 1);
    s.terhiq(); assert.strictEqual(s.thellesia(), 1, 'rrenja nuk hiqet');
    s.zevendeso_rrenjen(p3); assert.strictEqual(s.thellesia(), 1);
    assert.strictEqual(s.ekrani_aktual(), 0);
});

ok('skeda', () => {
    const t = skeda(); let nd = -1;
    t.kur_ndryshon((i) => { nd = i; });
    t.shto_skede(tekst_i_ri('A'), 'Skeda A').shto_skede(tekst_i_ri('B'), 'Skeda B');
    t.shko_te(1);
    assert.strictEqual(t.skeda_aktuale(), 1); assert.strictEqual(nd, 1);
    dritarja.shto(t);
});

ok('zona levizese', () => {
    const z = zone_levizese(tekst_i_ri('shume tekst'));
    z.vendos_permbajtjen(kontejner());
    dritarja.shto(z);
});

ok('lista', () => {
    let zgj = null;
    const l = liste(['a']).shto_elemente(['b', 'c']).kur_zgjidhet((i, t) => { zgj = [i, t]; });
    assert.strictEqual(l.indeksi_zgjedhur(), -1);
    l.vendosIndeksin(1);
    assert.deepStrictEqual(zgj, [1, 'b']);
    assert.strictEqual(l.teksti_zgjedhur(), 'b');
    l.pastro(); assert.strictEqual(l.indeksi_zgjedhur(), -1);
    dritarja.shto(l);
});

ok('zgjedhesi', () => {
    let t = null;
    const z = zgjedhes(['x', 'y']).kur_ndryshon((v) => { t = v; });
    z.shto_opsione(['z']);
    assert.strictEqual(z.vlera(), 'x'); assert.strictEqual(z.indeksi(), 0);
    z.vendos_indeksin(2);
    assert.strictEqual(z.vlera(), 'z'); assert.strictEqual(t, 'z');
    dritarja.shto(z);
});

ok('kutia e zgjedhjes', () => {
    let v = null;
    const c = kuti_zgjedhjeje('Pranoj').kur_ndryshon((x) => { v = x; });
    assert.strictEqual(c.eshte_zgjedhur(), false);
    c.kliko(); assert.strictEqual(c.eshte_zgjedhur(), true); assert.strictEqual(v, true);
    c.vendos_zgjedhur(false); assert.strictEqual(v, false);
    c.vendos_tekst('Tjeter'); assert.strictEqual(c.merrTekstin(), 'Tjeter');
    dritarja.shto(c);
});

ok('progresi', () => {
    dritarja.shto(progres(0, 10).vendos_vlere(5).vendos_format('%p% gati'));
});

ok('dritarja: tastiera, titulli, shto me pozicion, madhesia', () => {
    let ev = null;
    dritarja.kur_shtypet((e) => { ev = e; });
    dritarja.simuloTast(Qt.Key_A, 'a');
    assert.deepStrictEqual(ev, { tasti: Qt.Key_A, teksti: 'a' });
    dritarja.vendos_titull('Titull i ri').vendos_madhesi(640, 520);
    dritarja.shto(tekst_i_ri('ne pozicion'), 20, 1, 1, 2);
    dritarja.shto(tekst_i_ri('rreshti pas pozicionit'));
});

ok('fshih / shfaq_elementin', () => {
    const b = buton_i_ri('x');
    assert.strictEqual(b.fshih(), b);
    assert.strictEqual(b.shfaq_elementin(), b);
    b.fshih(); // nje widget pa prind qe shfaqet behet dritare vete; e fshehim qe cikli te mund te ndalet
});

ok('dritarja: vendos_permbajtjen zevendeson rrjeten', () => {
    const d2 = krijo_dritare('D2');
    d2.vendos_permbajtjen(ekrane());
    assert.throws(() => d2.shto(buton_i_ri('x')), /nuk ka rrjet/);
});

ok('shfaqja nis ciklin vete dhe ai ndalon kur mbyllet dritarja', () => {
    dritarja.shfaqe();
    assert.strictEqual(gui._dritaretAktive.size, 1);
    let mbyllur = false;
    dritarja.ne('mbyllje', () => { mbyllur = true; });
    setTimeout(() => {
        // pas disa cikleve Qt, dritarja mbyllet; procesi duhet te perfundoje vetvetiu
        dritarja.mbylle();
        setTimeout(() => {
            ok('dritarja u mbyll dhe u hoq nga regjistri', () => {
                assert.ok(mbyllur);
                assert.strictEqual(gui._dritaretAktive.size, 0);
            });
            console.log(`\n${numri} teste kaluan` + (process.exitCode ? ' (ka gabime)' : ''));
        }, 100);
    }, 200);
});

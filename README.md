# 🦅 EagleGUI (version demo)

Modul Node.js për ndërfaqe grafike desktop, i ndërtuar me **Qt Widgets** dhe **C++** (Node-API),
në frymën e NodeGUI, por me API në **gjuhën shqipe**.

> Ky është një version demo: mbulon vetëm disa widget-e bazë dhe nuk është testuar në të gjitha platformat.

## Kërkesat

- Node.js 18+
- CMake 3.15+ dhe një kompajler C++17 (g++, clang, MSVC)
- Qt 6 (ose Qt 5) me modulin Widgets
  - Ubuntu/Debian: `sudo apt install qt6-base-dev cmake build-essential`
  - macOS: `brew install qt cmake`
  - Windows: instalo Qt nga qt.io dhe CMake

## Ndërtimi dhe ekzekutimi

```bash
npm install
npm run nderto
npm test        # 20 teste sjelljeje (ne Linux pa ekran perdor QT_QPA_PLATFORM=offscreen)
npm run demo    # API me klasa
npm run demo:gui # sintaksa gui.js
```

Nëse CMake nuk e gjen Qt-në, jepi rrugën:

```bash
npx cmake-js compile --CDCMAKE_PREFIX_PATH=/rruga/drejt/Qt/6.x/gcc_64
```

## Shembull

```js
const { Aplikacion, Dritare, Buton, Etikete } = require('eaglegui');

const dritarja = new Dritare().vendosTitullin('Përshëndetje').vendosMadhesine(300, 150);
const etiketa = new Etikete('Kliko butonin');
const butoni = new Buton('Më kliko');

butoni.ne('klikim', () => etiketa.vendosTekstin('U klikova! 🦅'));

dritarja.shtoFemije(etiketa, butoni).shfaq();
Aplikacion.ekzekuto();
```

## API

| Klasa / Metoda | Përshkrim |
|---|---|
| `Aplikacion.ekzekuto()` | Nis ciklin e ngjarjeve; kthen `Promise` që zgjidhet kur mbyllen dritaret |
| `new Dritare()` | Dritare kryesore (me paraqitje vertikale) |
| `new Buton(tekst?)` | Buton |
| `new Etikete(tekst?)` | Etiketë teksti |
| `new FushaTeksti()` | Fushë për shkrim teksti |
| `.vendosTekstin(t)` / `.merrTekstin()` | Vendos / merr tekstin |
| `.vendosPlaceholder(t)` | Teksti i dobët te `FushaTeksti` |
| `.vendosTitullin(t)` | Titulli i dritares |
| `.vendosMadhesine(gjeresia, lartesia)` | Madhësia në piksel |
| `.vendosStilin(qss)` | Stil me Qt Style Sheets (si CSS) |
| `.shtoFemije(...elementet)` | Shton elementë brenda paraqitjes |
| `.ne(ngjarja, funksioni)` | Dëgjon ngjarje: `klikim`, `ndryshim`, `mbyllje` |
| `.shfaq()` / `.fshih()` / `.mbyll()` | Kontrolli i dukshmërisë |

## Sintaksa gui.js (pa rishkruar asgje)

`gui.js` ne kete repo ka **te njejtat funksione, metoda dhe eksporte** me `gui.js` te Eagle Script
te bazuar ne `@nodegui/nodegui` (`krijo_dritare`, `buton_i_ri`, `kontejner`, `ekrane`, `.kur_klikohet`,
`.vendos_stil`, `EMRAT_GUI`, `_dritaretAktive` ...). Skriptet `.es` dhe interpretuesi nuk ndryshojne:
mjafton qe `gui.js` i projektit te zevendesohet me kete, ose te behet nje rieksport:

```js
// gui.js i projektit Eagle Script
module.exports = require('eaglegui/gui');
```

Ndryshimet jane vetem ne backend:

- nuk duhet `qode` dhe nuk duhet `@nodegui/nodegui` - funksionon me Node te zakonshem;
- cikli i ngjarjeve niset **vete** me `shfaqe()` / `ekran_i_plote()` / `maksimizo()` dhe ndalon kur
  mbyllet dritarja e fundit (si me NodeGui, nuk therret asgje shtese);
- Qt ngarkohet **vone** (vetem kur therritet per here te pare nje funksion GUI), prandaj skriptet CLI
  nuk varen nga Qt.

Funksionet: `krijo_dritare`, `buton_i_ri`, `tekst_i_ri`, `fushe_teksti`, `fushe_fjalekalimi`,
`zone_teksti`, `imazh_i_ri`, `kontejner`, `karte`, `ekrane`, `skeda`, `zone_levizese`, `liste`,
`zgjedhes`, `kuti_zgjedhjeje`, `progres`, `shfaq_mesazh`, `pyet`, `zgjidh_skedar`.

### Shtresa native (per zgjerime)

`main.cpp` ofron nje klase te vetme `Elementi` per cdo lloj widget-i; `gui.js` ndertohet mbi te ne
JavaScript. Per te ndryshuar sintaksen shqipe **nuk duhet rikompilim** - vetem per widget-e te reja.

| Lloji (`new Elementi(tipi)`) | Widget Qt |
|---|---|
| `dritare` / `dritare_kryesore` | `QWidget` / `QMainWindow` me rrjete |
| `kornize_v` / `kornize_h` | `QFrame` + `QBoxLayout` |
| `buton`, `etikete`, `fushe`, `zone_teksti`, `kuti` | `QPushButton`, `QLabel`, `QLineEdit`, `QTextEdit`, `QCheckBox` |
| `zgjedhes`, `liste`, `progres` | `QComboBox`, `QListWidget`, `QProgressBar` |
| `pirg`, `skeda`, `zone_levizese` | `QStackedWidget`, `QTabWidget`, `QScrollArea` |

Ngjarjet e `ne(emri, fn)`: `klikim`, `ndryshim`, `enter`, `zgjedhje`, `tast`, `liberim_mausi`, `mbyllje`.
Per automatizim/teste: `kliko()` dhe `simuloTast(kodi, teksti)`.

### Kufizime te njohura

- Dialogët (`shfaq_mesazh`, `pyet`, `zgjidh_skedar`) jane bllokues si me NodeGui; gjate tyre
  nuk ekzekutohen kohematuesit e JS (`setTimeout`).
- Widget-et kthehen si objekte `Elementi` te eaglegui, jo si objekte NodeGui. Pra metodat e para
  Qt te NodeGui (p.sh. `.setText()`, `.show()`) nuk ekzistojne - vlen sintaksa shqipe.
- `dritarja.vendos_permbajtjen(w)` zevendeson rrjeten e dritares, ndaj pas saj `dritarja.shto(...)`
  hedh gabim (nuk ka me rrjete per t'u shtuar elemente).
- Nje widget pa prind qe shfaqet me `shfaq_elementin()` behet dritare me vete; edhe ai e mban ciklin ne pune.

## Si funksionon

Cikli i ngjarjeve të Qt integrohet me atë të Node pa e modifikuar Node-n: JS thërret çdo ~8 ms
funksionin native `procesoNgjarjet()`, i cili ekzekuton `QApplication::processEvents()`.
Callback-et JS thirren nga slot-et Qt brenda kësaj thirrjeje. NodeGUI përdor qasje më të avancuar
(binar `qode` me cikël të bashkuar), që është hapi i radhës nëse e zgjeron projektin.

## Hapat e ardhshëm

Menu, toolbar dhe dialogë me më shumë opsione, tabela (`QTableWidget`), si dhe paketim me `prebuildify`.

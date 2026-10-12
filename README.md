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
npm run demo
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

## Si funksionon

Cikli i ngjarjeve të Qt integrohet me atë të Node pa e modifikuar Node-n: JS thërret çdo ~8 ms
funksionin native `procesoNgjarjet()`, i cili ekzekuton `QApplication::processEvents()`.
Callback-et JS thirren nga slot-et Qt brenda kësaj thirrjeje. NodeGUI përdor qasje më të avancuar
(binar `qode` me cikël të bashkuar), që është hapi i radhës nëse e zgjeron projektin.

## Hapat e ardhshëm

Layout-e shtesë (horizontal, grid), më shumë widget-e (CheckBox, ComboBox, ListWidget),
ngjarje të tjera, menu dhe dialogë, si dhe paketim me `prebuildify`.

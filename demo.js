'use strict';

const { Aplikacion, Dritare, Buton, Etikete, FushaTeksti } = require('../lib');

const dritarja = new Dritare()
  .vendosTitullin('EagleGUI – Demo')
  .vendosMadhesine(380, 260)
  .vendosStilin(`
    QWidget { font-size: 15px; }
    QPushButton { padding: 8px; background: #1d4ed8; color: white; border-radius: 6px; }
    QPushButton:hover { background: #2563eb; }
    QLineEdit { padding: 6px; }
  `);

const etiketa = new Etikete('Përshëndetje nga EagleGUI! 🦅');
const fusha = new FushaTeksti().vendosPlaceholder('Shkruaj emrin tënd...');
const butoniPershendet = new Buton('Përshëndet');
const butoniNumero = new Buton('Numëro: 0');

let numri = 0;

fusha.ne('ndryshim', (tekst) => {
  console.log('Fusha u ndryshua:', tekst);
});

butoniPershendet.ne('klikim', () => {
  const emri = fusha.merrTekstin().trim() || 'mik';
  etiketa.vendosTekstin(`Përshëndetje, ${emri}!`);
});

butoniNumero.ne('klikim', () => {
  numri += 1;
  butoniNumero.vendosTekstin(`Numëro: ${numri}`);
});

dritarja.ne('mbyllje', () => console.log('Mirupafshim!'));

dritarja.shtoFemije(etiketa, fusha, butoniPershendet, butoniNumero).shfaq();

Aplikacion.ekzekuto().then(() => console.log('Aplikacioni përfundoi.'));

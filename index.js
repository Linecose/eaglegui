'use strict';

/**
 * EagleGUI – API në shqip mbi addon-in native Qt.
 *
 * Klasat:  Aplikacion, Dritare, Buton, Etikete, FushaTeksti
 * Metodat: vendosTekstin, merrTekstin, vendosTitullin, vendosMadhesine,
 *          vendosStilin, shtoFemije, ne, shfaq, fshih, mbyll
 * Ngjarjet: "klikim" (Buton), "ndryshim" (FushaTeksti), "mbyllje" (Dritare)
 */

const nativ = require('../build/Release/eaglegui_native.node');

let filluar = false;
function siguroFillimin() {
  if (!filluar) {
    nativ.fillo();
    filluar = true;
  }
}

class Elementi {
  constructor(tipi) {
    siguroFillimin();
    this._n = new nativ.Elementi(tipi);
  }

  vendosTekstin(tekst) { this._n.vendosTekstin(String(tekst)); return this; }
  merrTekstin() { return this._n.merrTekstin(); }
  vendosTitullin(titull) { this._n.vendosTitullin(String(titull)); return this; }
  vendosMadhesine(gjeresia, lartesia) { this._n.vendosMadhesine(gjeresia, lartesia); return this; }
  vendosStilin(qss) { this._n.vendosStilin(String(qss)); return this; }

  shtoFemije(...femije) {
    for (const f of femije) this._n.shtoFemije(f._n);
    return this;
  }

  ne(ngjarja, funksioni) { this._n.ne(ngjarja, funksioni); return this; }

  shfaq() { this._n.shfaq(); return this; }
  fshih() { this._n.fshih(); return this; }
  mbyll() { this._n.mbyll(); return this; }
}

class Dritare extends Elementi {
  constructor() { super('dritare'); }
}

class Buton extends Elementi {
  constructor(tekst = '') {
    super('buton');
    if (tekst) this.vendosTekstin(tekst);
  }
}

class Etikete extends Elementi {
  constructor(tekst = '') {
    super('etikete');
    if (tekst) this.vendosTekstin(tekst);
  }
}

class FushaTeksti extends Elementi {
  constructor() { super('fushe'); }
  vendosPlaceholder(tekst) { this._n.vendosPlaceholder(String(tekst)); return this; }
}

const Aplikacion = {
  /** Nis ciklin e ngjarjeve. Premtimi zgjidhet kur mbyllen të gjitha dritaret. */
  ekzekuto() {
    siguroFillimin();
    return new Promise((zgjidh) => {
      const kohmatesi = setInterval(() => {
        if (!nativ.procesoNgjarjet()) {
          clearInterval(kohmatesi);
          zgjidh();
        }
      }, 8);
    });
  },
};

module.exports = { Aplikacion, Dritare, Buton, Etikete, FushaTeksti };

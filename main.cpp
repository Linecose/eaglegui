// EagleGUI – addon native (Qt Widgets + Node-API)
//
// Ideja: nuk e zëvendësojmë ciklin e ngjarjeve të Node. Në vend të kësaj, JS
// thërret periodikisht `procesoNgjarjet()`, i cili ia jep kontrollin Qt-së për
// pak çaste. Kështu callback-et JS thirren gjithmonë brenda një thirrjeje
// legjitime Node-API, pa nevojë për Node të modifikuar.
//
// Kjo shtresë native është e përgjithshme (një klasë `Elementi` për çdo lloj
// widget-i). Sintaksa shqipe e `gui.js` (krijo_dritare, buton_i_ri, ...)
// ndërtohet mbi të në JavaScript, ndaj nuk kërkon rikompilim kur ndryshon.

#include <napi.h>

#include <QAbstractButton>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Aplikacioni Qt (një i vetëm për proces)
// ---------------------------------------------------------------------------
QApplication* g_aplikacioni = nullptr;
int g_argc = 1;
char g_emri[] = "eaglegui";
char* g_argv[] = {g_emri, nullptr};

// ---------------------------------------------------------------------------
// Ndihmëse
// ---------------------------------------------------------------------------
using Thirrja = std::shared_ptr<Napi::FunctionReference>;

Thirrja krijoThirrje(const Napi::Function& f) {
  auto ref = std::make_shared<Napi::FunctionReference>();
  *ref = Napi::Persistent(f);
  return ref;
}

void thirr(const Thirrja& f, std::vector<napi_value> args = {}) {
  f->Call(args);
}

QString ngaJS(const Napi::Value& v) {
  return QString::fromUtf8(v.As<Napi::String>().Utf8Value().c_str());
}

Napi::String drejtJS(Napi::Env env, const QString& t) {
  const QByteArray b = t.toUtf8();
  return Napi::String::New(env, b.constData(), static_cast<size_t>(b.size()));
}

Napi::Value gabim(Napi::Env env, const char* mesazhi) {
  Napi::Error::New(env, mesazhi).ThrowAsJavaScriptException();
  return env.Undefined();
}

Napi::Value gabimTipi(Napi::Env env, const char* mesazhi) {
  Napi::TypeError::New(env, mesazhi).ThrowAsJavaScriptException();
  return env.Undefined();
}

bool kaString(const Napi::CallbackInfo& info, size_t i) {
  return info.Length() > i && info[i].IsString();
}

bool kaNumra(const Napi::CallbackInfo& info, size_t n) {
  if (info.Length() < n) return false;
  for (size_t i = 0; i < n; ++i)
    if (!info[i].IsNumber()) return false;
  return true;
}

int numriNga(const Napi::CallbackInfo& info, size_t i) {
  return info[i].As<Napi::Number>().Int32Value();
}

// Filtron një lloj ngjarjeje Qt (mbyllje, tastierë, lirim i miut...) për një widget.
class FiltriNgjarjes : public QObject {
 public:
  FiltriNgjarjes(QObject* prindi, QEvent::Type tipi, std::function<void(QEvent*)> kthimi)
      : QObject(prindi), tipi_(tipi), kthimi_(std::move(kthimi)) {}

 protected:
  bool eventFilter(QObject* objekti, QEvent* ngjarja) override {
    if (ngjarja->type() == tipi_) kthimi_(ngjarja);
    return QObject::eventFilter(objekti, ngjarja);  // asnjëherë nuk e konsumojmë
  }

 private:
  QEvent::Type tipi_;
  std::function<void(QEvent*)> kthimi_;
};

#define GATI(env) \
  if (!gati(env)) return env.Undefined()
#define KERKO_NUMRA(n) \
  if (!kaNumra(info, n)) return gabimTipi(env, "Numer i gabuar argumentesh numerike.")
#define KERKO_STRING(i) \
  if (!kaString(info, i)) return gabimTipi(env, "Pritej tekst (string).")

// ---------------------------------------------------------------------------
// Elementi – një mbështjellës i vetëm për çdo lloj widget-i
// ---------------------------------------------------------------------------
class Elementi : public Napi::ObjectWrap<Elementi> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function ctor = DefineClass(
        env, "Elementi",
        {
            // --- bazë (API ekzistuese) ---
            InstanceMethod("vendosTekstin", &Elementi::VendosTekstin, napi_default_method),
            InstanceMethod("merrTekstin", &Elementi::MerrTekstin, napi_default_method),
            InstanceMethod("vendosPlaceholder", &Elementi::VendosPlaceholder, napi_default_method),
            InstanceMethod("vendosTitullin", &Elementi::VendosTitullin, napi_default_method),
            InstanceMethod("vendosMadhesine", &Elementi::VendosMadhesine, napi_default_method),
            InstanceMethod("vendosStilin", &Elementi::VendosStilin, napi_default_method),
            InstanceMethod("shtoFemije", &Elementi::ShtoFemije, napi_default_method),
            InstanceMethod("ne", &Elementi::Ne, napi_default_method),
            InstanceMethod("shfaq", &Elementi::Shfaq, napi_default_method),
            InstanceMethod("fshih", &Elementi::Fshih, napi_default_method),
            InstanceMethod("mbyll", &Elementi::Mbyll, napi_default_method),

            // --- madhësi, gjendje, dukje ---
            InstanceMethod("vendosMadhesineMin", &Elementi::VendosMadhesineMin, napi_default_method),
            InstanceMethod("vendosMadhesineFikse", &Elementi::VendosMadhesineFikse, napi_default_method),
            InstanceMethod("vendosMadhesineMaks", &Elementi::VendosMadhesineMaks, napi_default_method),
            InstanceMethod("vendosFokus", &Elementi::VendosFokus, napi_default_method),
            InstanceMethod("vendosId", &Elementi::VendosId, napi_default_method),
            InstanceMethod("vendosKeshille", &Elementi::VendosKeshille, napi_default_method),
            InstanceMethod("vendosAktiv", &Elementi::VendosAktiv, napi_default_method),
            InstanceMethod("kursorDore", &Elementi::KursorDore, napi_default_method),
            InstanceMethod("ekranIPlote", &Elementi::EkranIPlote, napi_default_method),
            InstanceMethod("maksimizoDritaren", &Elementi::MaksimizoDritaren, napi_default_method),

            // --- tekst / imazh / fusha ---
            InstanceMethod("vendosMbulim", &Elementi::VendosMbulim, napi_default_method),
            InstanceMethod("vendosRreshtim", &Elementi::VendosRreshtim, napi_default_method),
            InstanceMethod("vendosVetemLexim", &Elementi::VendosVetemLexim, napi_default_method),
            InstanceMethod("vendosFjalekalim", &Elementi::VendosFjalekalim, napi_default_method),
            InstanceMethod("vendosImazhNgaSkedari", &Elementi::VendosImazhNgaSkedari, napi_default_method),
            InstanceMethod("vendosImazhNgaTeDhena", &Elementi::VendosImazhNgaTeDhena, napi_default_method),

            // --- paraqitje (layout) ---
            InstanceMethod("shtoNeRrjet", &Elementi::ShtoNeRrjet, napi_default_method),
            InstanceMethod("shtoNeKuti", &Elementi::ShtoNeKuti, napi_default_method),
            InstanceMethod("shtoHapesire", &Elementi::ShtoHapesire, napi_default_method),
            InstanceMethod("shtoShtrirje", &Elementi::ShtoShtrirje, napi_default_method),
            InstanceMethod("vendosHapesira", &Elementi::VendosHapesira, napi_default_method),
            InstanceMethod("vendosMargina", &Elementi::VendosMargina, napi_default_method),
            InstanceMethod("hiqNgaKuti", &Elementi::HiqNgaKuti, napi_default_method),
            InstanceMethod("vendosQendren", &Elementi::VendosQendren, napi_default_method),

            // --- ekrane (stack), skeda, zonë lëvizëse ---
            InstanceMethod("shtoNePirg", &Elementi::ShtoNePirg, napi_default_method),
            InstanceMethod("aktivizoNePirg", &Elementi::AktivizoNePirg, napi_default_method),
            InstanceMethod("hiqNgaPirg", &Elementi::HiqNgaPirg, napi_default_method),
            InstanceMethod("shtoSkede", &Elementi::ShtoSkede, napi_default_method),
            InstanceMethod("vendosWidget", &Elementi::VendosWidget, napi_default_method),
            InstanceMethod("vendosIndeksin", &Elementi::VendosIndeksin, napi_default_method),
            InstanceMethod("merrIndeksin", &Elementi::MerrIndeksin, napi_default_method),

            // --- listë, zgjedhës, kuti zgjedhjeje, progres ---
            InstanceMethod("shtoRresht", &Elementi::ShtoRresht, napi_default_method),
            InstanceMethod("pastroListen", &Elementi::PastroListen, napi_default_method),
            InstanceMethod("shtoOpsione", &Elementi::ShtoOpsione, napi_default_method),
            InstanceMethod("eshteZgjedhur", &Elementi::EshteZgjedhur, napi_default_method),
            InstanceMethod("vendosZgjedhur", &Elementi::VendosZgjedhur, napi_default_method),
            InstanceMethod("vendosInterval", &Elementi::VendosInterval, napi_default_method),
            InstanceMethod("vendosVlere", &Elementi::VendosVlere, napi_default_method),
            InstanceMethod("vendosFormat", &Elementi::VendosFormat, napi_default_method),

            // --- simulim (automatizim / teste) ---
            InstanceMethod("kliko", &Elementi::Kliko, napi_default_method),
            InstanceMethod("simuloTast", &Elementi::SimuloTast, napi_default_method),
        });
    konstruktori = Napi::Persistent(ctor);
    konstruktori.SuppressDestruct();
    exports.Set("Elementi", ctor);
    return exports;
  }

  Elementi(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Elementi>(info) {
    Napi::Env env = info.Env();
    if (!g_aplikacioni) {
      gabim(env, "Thirr fillo() përpara se të krijosh elemente.");
      return;
    }
    if (info.Length() < 1 || !info[0].IsString()) {
      gabimTipi(env, "Pritej emri i llojit (string).");
      return;
    }
    const std::string tipi = info[0].As<Napi::String>().Utf8Value();

    if (tipi == "dritare") {
      auto* d = new QWidget();
      new QVBoxLayout(d);
      w_ = d;
    } else if (tipi == "dritare_kryesore") {
      // QMainWindow me një widget qendror që ka rrjetë (grid) – si krijo_dritare()
      auto* m = new QMainWindow();
      auto* qendra = new QWidget();
      qendra->setObjectName("eagle-rrenja");
      auto* grid = new QGridLayout();
      grid->setSpacing(8);
      grid->setContentsMargins(10, 10, 10, 10);
      qendra->setLayout(grid);
      m->setCentralWidget(qendra);
      w_ = m;
    } else if (tipi == "kornize_v" || tipi == "kornize_h") {
      auto* fr = new QFrame();
      auto* l = new QBoxLayout(tipi == "kornize_h" ? QBoxLayout::LeftToRight
                                                    : QBoxLayout::TopToBottom);
      l->setSpacing(8);
      l->setContentsMargins(0, 0, 0, 0);
      fr->setLayout(l);
      w_ = fr;
    } else if (tipi == "buton") {
      w_ = new QPushButton();
    } else if (tipi == "etikete") {
      w_ = new QLabel();
    } else if (tipi == "fushe") {
      w_ = new QLineEdit();
    } else if (tipi == "zone_teksti") {
      w_ = new QTextEdit();
    } else if (tipi == "kuti") {
      w_ = new QCheckBox();
    } else if (tipi == "zgjedhes") {
      w_ = new QComboBox();
    } else if (tipi == "liste") {
      w_ = new QListWidget();
    } else if (tipi == "progres") {
      w_ = new QProgressBar();
    } else if (tipi == "pirg") {
      w_ = new QStackedWidget();
    } else if (tipi == "skeda") {
      w_ = new QTabWidget();
    } else if (tipi == "zone_levizese") {
      auto* z = new QScrollArea();
      z->setWidgetResizable(true);
      w_ = z;
    } else {
      gabim(env, "Lloj elementi i panjohur.");
    }
  }

  ~Elementi() override {
    // Widget-et fëmijë i zotëron Qt. Dritaret e dukshme i lëmë të jetojnë,
    // që mbledhja e plehrave (GC) të mos i mbyllë papritur.
    if (w_ && !w_->parentWidget() && !w_->isVisible()) delete w_.data();
  }

 private:
  inline static Napi::FunctionReference konstruktori;
  QPointer<QWidget> w_;

  bool gati(Napi::Env env) {
    if (w_) return true;
    gabim(env, "Elementi nuk është më i vlefshëm.");
    return false;
  }

  // Kthen Elementin e argumentit (ose nullptr pasi hedh gabim në JS).
  static Elementi* nga(Napi::Env env, const Napi::Value& v) {
    if (!v.IsObject() || !v.As<Napi::Object>().InstanceOf(konstruktori.Value())) {
      gabimTipi(env, "Pritej një Elementi.");
      return nullptr;
    }
    Elementi* e = Napi::ObjectWrap<Elementi>::Unwrap(v.As<Napi::Object>());
    if (!e->w_) {
      gabim(env, "Elementi (fëmijë) nuk është më i vlefshëm.");
      return nullptr;
    }
    return e;
  }

  // ------------------------------------------------------------------ bazë
  Napi::Value VendosTekstin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_STRING(0);
    const QString t = ngaJS(info[0]);
    if (auto* b = qobject_cast<QAbstractButton*>(w_.data())) b->setText(t);
    else if (auto* l = qobject_cast<QLabel*>(w_.data())) l->setText(t);
    else if (auto* f = qobject_cast<QLineEdit*>(w_.data())) f->setText(t);
    else if (auto* te = qobject_cast<QTextEdit*>(w_.data())) te->setPlainText(t);
    else if (auto* c = qobject_cast<QComboBox*>(w_.data())) c->setCurrentText(t);
    return env.Undefined();
  }

  Napi::Value MerrTekstin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    QString t;
    if (auto* b = qobject_cast<QAbstractButton*>(w_.data())) t = b->text();
    else if (auto* l = qobject_cast<QLabel*>(w_.data())) t = l->text();
    else if (auto* f = qobject_cast<QLineEdit*>(w_.data())) t = f->text();
    else if (auto* te = qobject_cast<QTextEdit*>(w_.data())) t = te->toPlainText();
    else if (auto* c = qobject_cast<QComboBox*>(w_.data())) t = c->currentText();
    return drejtJS(env, t);
  }

  Napi::Value VendosPlaceholder(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_STRING(0);
    if (auto* f = qobject_cast<QLineEdit*>(w_.data())) f->setPlaceholderText(ngaJS(info[0]));
    else if (auto* te = qobject_cast<QTextEdit*>(w_.data())) te->setPlaceholderText(ngaJS(info[0]));
    return env.Undefined();
  }

  Napi::Value VendosTitullin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_STRING(0);
    w_->setWindowTitle(ngaJS(info[0]));
    return env.Undefined();
  }

  // Ndryshon permasat aktuale (resize)
  Napi::Value VendosMadhesine(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_NUMRA(2);
    w_->resize(numriNga(info, 0), numriNga(info, 1));
    return env.Undefined();
  }

  Napi::Value VendosStilin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_STRING(0);
    w_->setStyleSheet(ngaJS(info[0]));
    return env.Undefined();
  }

  Napi::Value ShtoFemije(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    Elementi* femija = nga(env, info[0]);
    if (!femija) return env.Undefined();
    QLayout* paraqitja = w_->layout();
    if (!paraqitja) paraqitja = new QVBoxLayout(w_.data());
    paraqitja->addWidget(femija->w_.data());
    return env.Undefined();
  }

  Napi::Value Ne(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
      return gabimTipi(env, "Përdorim: ne(ngjarje, funksion).");

    const std::string ngjarja = info[0].As<Napi::String>().Utf8Value();
    Thirrja thirrja = krijoThirrje(info[1].As<Napi::Function>());
    QWidget* w = w_.data();

    if (ngjarja == "klikim") {
      auto* b = qobject_cast<QAbstractButton*>(w);
      if (!b) return gabim(env, "Ngjarja 'klikim' mbështetet vetëm te butonat.");
      QObject::connect(b, &QAbstractButton::clicked, b, [env, thirrja]() {
        Napi::HandleScope scope(env);
        thirr(thirrja);
      });

    } else if (ngjarja == "ndryshim") {
      // Çdo lloj widget-i e jep vlerën e vet: tekst, tekst i plotë, bool ose indeks.
      if (auto* f = qobject_cast<QLineEdit*>(w)) {
        QObject::connect(f, &QLineEdit::textChanged, f, [env, thirrja](const QString& t) {
          Napi::HandleScope scope(env);
          thirr(thirrja, {drejtJS(env, t)});
        });
      } else if (auto* te = qobject_cast<QTextEdit*>(w)) {
        QObject::connect(te, &QTextEdit::textChanged, te, [env, thirrja, te]() {
          Napi::HandleScope scope(env);
          thirr(thirrja, {drejtJS(env, te->toPlainText())});
        });
      } else if (auto* c = qobject_cast<QComboBox*>(w)) {
        QObject::connect(c, &QComboBox::currentTextChanged, c, [env, thirrja](const QString& t) {
          Napi::HandleScope scope(env);
          thirr(thirrja, {drejtJS(env, t)});
        });
      } else if (auto* k = qobject_cast<QCheckBox*>(w)) {
        QObject::connect(k, &QCheckBox::toggled, k, [env, thirrja](bool v) {
          Napi::HandleScope scope(env);
          thirr(thirrja, {Napi::Boolean::New(env, v)});
        });
      } else if (auto* p = qobject_cast<QStackedWidget*>(w)) {
        QObject::connect(p, &QStackedWidget::currentChanged, p, [env, thirrja](int i) {
          Napi::HandleScope scope(env);
          thirr(thirrja, {Napi::Number::New(env, i)});
        });
      } else if (auto* s = qobject_cast<QTabWidget*>(w)) {
        QObject::connect(s, &QTabWidget::currentChanged, s, [env, thirrja](int i) {
          Napi::HandleScope scope(env);
          thirr(thirrja, {Napi::Number::New(env, i)});
        });
      } else {
        return gabim(env, "Ngjarja 'ndryshim' nuk mbështetet nga ky lloj elementi.");
      }

    } else if (ngjarja == "enter") {
      auto* f = qobject_cast<QLineEdit*>(w);
      if (!f) return gabim(env, "Ngjarja 'enter' mbështetet vetëm te FushaTeksti.");
      QObject::connect(f, &QLineEdit::returnPressed, f, [env, thirrja]() {
        Napi::HandleScope scope(env);
        thirr(thirrja);
      });

    } else if (ngjarja == "zgjedhje") {
      auto* l = qobject_cast<QListWidget*>(w);
      if (!l) return gabim(env, "Ngjarja 'zgjedhje' mbështetet vetëm te lista.");
      QObject::connect(l, &QListWidget::currentRowChanged, l, [env, thirrja](int r) {
        Napi::HandleScope scope(env);
        thirr(thirrja, {Napi::Number::New(env, r)});
      });

    } else if (ngjarja == "tast") {
      // funksioni merr { tasti, teksti }
      auto* filtri = new FiltriNgjarjes(w, QEvent::KeyPress, [env, thirrja](QEvent* e) {
        auto* k = static_cast<QKeyEvent*>(e);
        Napi::HandleScope scope(env);
        Napi::Object o = Napi::Object::New(env);
        o.Set("tasti", Napi::Number::New(env, k->key()));
        o.Set("teksti", drejtJS(env, k->text()));
        thirr(thirrja, {o});
      });
      w->installEventFilter(filtri);

    } else if (ngjarja == "liberim_mausi") {
      auto* filtri = new FiltriNgjarjes(w, QEvent::MouseButtonRelease, [env, thirrja](QEvent*) {
        Napi::HandleScope scope(env);
        thirr(thirrja);
      });
      w->installEventFilter(filtri);

    } else if (ngjarja == "mbyllje") {
      auto* filtri = new FiltriNgjarjes(w, QEvent::Close, [env, thirrja](QEvent*) {
        Napi::HandleScope scope(env);
        thirr(thirrja);
      });
      w->installEventFilter(filtri);

    } else {
      return gabim(env,
                   "Ngjarje e panjohur. Të mbështetura: klikim, ndryshim, enter, zgjedhje, "
                   "tast, liberim_mausi, mbyllje.");
    }
    return env.Undefined();
  }

  Napi::Value Shfaq(const Napi::CallbackInfo& info) {
    if (gati(info.Env())) w_->show();
    return info.Env().Undefined();
  }

  Napi::Value Fshih(const Napi::CallbackInfo& info) {
    if (gati(info.Env())) w_->hide();
    return info.Env().Undefined();
  }

  Napi::Value Mbyll(const Napi::CallbackInfo& info) {
    if (gati(info.Env())) w_->close();
    return info.Env().Undefined();
  }

  // ------------------------------------------------- madhësi, gjendje, dukje
  Napi::Value VendosMadhesineMin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_NUMRA(2);
    w_->setMinimumSize(numriNga(info, 0), numriNga(info, 1));
    return env.Undefined();
  }

  Napi::Value VendosMadhesineFikse(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_NUMRA(2);
    w_->setFixedSize(numriNga(info, 0), numriNga(info, 1));
    return env.Undefined();
  }

  Napi::Value VendosMadhesineMaks(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_NUMRA(2);
    w_->setMaximumSize(numriNga(info, 0), numriNga(info, 1));
    return env.Undefined();
  }

  Napi::Value VendosFokus(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    w_->setFocus();
    return env.Undefined();
  }

  Napi::Value VendosId(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_STRING(0);
    w_->setObjectName(ngaJS(info[0]));
    return env.Undefined();
  }

  Napi::Value VendosKeshille(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_STRING(0);
    w_->setToolTip(ngaJS(info[0]));
    return env.Undefined();
  }

  Napi::Value VendosAktiv(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    w_->setEnabled(info.Length() < 1 ? true : info[0].ToBoolean().Value());
    return env.Undefined();
  }

  Napi::Value KursorDore(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    w_->setCursor(Qt::PointingHandCursor);
    return env.Undefined();
  }

  Napi::Value EkranIPlote(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    w_->showFullScreen();
    return env.Undefined();
  }

  Napi::Value MaksimizoDritaren(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    w_->showMaximized();
    return env.Undefined();
  }

  // --------------------------------------------------- tekst / imazh / fusha
  Napi::Value VendosMbulim(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    if (auto* l = qobject_cast<QLabel*>(w_.data()))
      l->setWordWrap(info.Length() < 1 ? true : info[0].ToBoolean().Value());
    return env.Undefined();
  }

  // emri: AlignCenter | AlignLeft | AlignRight
  Napi::Value VendosRreshtim(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_STRING(0);
    const std::string emri = info[0].As<Napi::String>().Utf8Value();
    Qt::Alignment a = Qt::AlignLeft;
    if (emri == "AlignCenter") a = Qt::AlignCenter;
    else if (emri == "AlignRight") a = Qt::AlignRight;
    if (auto* l = qobject_cast<QLabel*>(w_.data())) l->setAlignment(a);
    return env.Undefined();
  }

  Napi::Value VendosVetemLexim(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    const bool po = info.Length() < 1 ? true : info[0].ToBoolean().Value();
    if (auto* f = qobject_cast<QLineEdit*>(w_.data())) f->setReadOnly(po);
    else if (auto* te = qobject_cast<QTextEdit*>(w_.data())) te->setReadOnly(po);
    return env.Undefined();
  }

  Napi::Value VendosFjalekalim(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    const bool po = info.Length() < 1 ? true : info[0].ToBoolean().Value();
    if (auto* f = qobject_cast<QLineEdit*>(w_.data()))
      f->setEchoMode(po ? QLineEdit::Password : QLineEdit::Normal);
    return env.Undefined();
  }

  static QPixmap shkallezo(const QPixmap& p, int gj, int lart) {
    if (gj > 0 && lart > 0)
      return p.scaled(gj, lart, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return p;
  }

  // (rruga, gjerësia, lartësia) -> bool
  Napi::Value VendosImazhNgaSkedari(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_STRING(0);
    auto* l = qobject_cast<QLabel*>(w_.data());
    if (!l) return gabim(env, "Imazhi mbështetet vetëm te etiketat.");
    QPixmap p;
    if (!p.load(ngaJS(info[0]))) return Napi::Boolean::New(env, false);
    const int gj = kaNumra(info, 3) ? numriNga(info, 1) : 0;
    const int lart = kaNumra(info, 3) ? numriNga(info, 2) : 0;
    l->setPixmap(shkallezo(p, gj, lart));
    return Napi::Boolean::New(env, true);
  }

  // (buffer, formati|undefined, gjerësia, lartësia) -> bool
  Napi::Value VendosImazhNgaTeDhena(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    if (info.Length() < 1 || !info[0].IsBuffer())
      return gabimTipi(env, "Pritej Buffer me te dhenat e imazhit.");
    auto* l = qobject_cast<QLabel*>(w_.data());
    if (!l) return gabim(env, "Imazhi mbështetet vetëm te etiketat.");
    auto buf = info[0].As<Napi::Buffer<uint8_t>>();
    std::string formati;
    if (info.Length() > 1 && info[1].IsString()) formati = info[1].As<Napi::String>().Utf8Value();
    QPixmap p;
    const bool ok = p.loadFromData(reinterpret_cast<const uchar*>(buf.Data()),
                                   static_cast<uint>(buf.Length()),
                                   formati.empty() ? nullptr : formati.c_str());
    if (!ok) return Napi::Boolean::New(env, false);
    const int gj = kaNumra(info, 4) ? numriNga(info, 2) : 0;
    const int lart = kaNumra(info, 4) ? numriNga(info, 3) : 0;
    l->setPixmap(shkallezo(p, gj, lart));
    return Napi::Boolean::New(env, true);
  }

  // --------------------------------------------------------------- paraqitje
  // (femija, rreshti, kolona, rreshtaShtrire, kolonaShtrire) në rrjetën e dritares kryesore
  Napi::Value ShtoNeRrjet(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    Elementi* f = nga(env, info[0]);
    if (!f) return env.Undefined();
    if (info.Length() < 5 || !info[1].IsNumber() || !info[2].IsNumber() ||
        !info[3].IsNumber() || !info[4].IsNumber())
      return gabimTipi(env, "Pritej: femija, rreshti, kolona, rreshtaShtrire, kolonaShtrire.");
    auto* m = qobject_cast<QMainWindow*>(w_.data());
    QGridLayout* g = (m && m->centralWidget())
                         ? qobject_cast<QGridLayout*>(m->centralWidget()->layout())
                         : nullptr;
    if (!g)
      return gabim(env, "Dritarja nuk ka rrjetë (përmbajtja u zëvendësua me vendos_permbajtjen).");
    g->addWidget(f->w_.data(), numriNga(info, 1), numriNga(info, 2), numriNga(info, 3),
                 numriNga(info, 4));
    return env.Undefined();
  }

  QBoxLayout* paraqitjaKuti() { return qobject_cast<QBoxLayout*>(w_->layout()); }

  // (femija, shtrirja)
  Napi::Value ShtoNeKuti(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    Elementi* f = nga(env, info[0]);
    if (!f) return env.Undefined();
    QBoxLayout* b = paraqitjaKuti();
    if (!b) return gabim(env, "Ky element nuk ka paraqitje vertikale/horizontale.");
    const int shtrirja = (info.Length() > 1 && info[1].IsNumber()) ? numriNga(info, 1) : 0;
    b->addWidget(f->w_.data(), shtrirja);
    return env.Undefined();
  }

  Napi::Value ShtoHapesire(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_NUMRA(1);
    if (QBoxLayout* b = paraqitjaKuti()) b->addSpacing(numriNga(info, 0));
    return env.Undefined();
  }

  Napi::Value ShtoShtrirje(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    if (QBoxLayout* b = paraqitjaKuti()) b->addStretch(1);
    return env.Undefined();
  }

  Napi::Value VendosHapesira(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_NUMRA(1);
    if (QLayout* l = w_->layout()) l->setSpacing(numriNga(info, 0));
    return env.Undefined();
  }

  // (majtas, siper, djathtas, poshte)
  Napi::Value VendosMargina(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_NUMRA(4);
    if (QLayout* l = w_->layout())
      l->setContentsMargins(numriNga(info, 0), numriNga(info, 1), numriNga(info, 2),
                            numriNga(info, 3));
    return env.Undefined();
  }

  Napi::Value HiqNgaKuti(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    Elementi* f = nga(env, info[0]);
    if (!f) return env.Undefined();
    if (QLayout* l = w_->layout()) l->removeWidget(f->w_.data());
    return env.Undefined();
  }

  // Zëvendëson widget-in qendror të QMainWindow (Qt e shkatërron të vjetrin).
  Napi::Value VendosQendren(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    Elementi* f = nga(env, info[0]);
    if (!f) return env.Undefined();
    auto* m = qobject_cast<QMainWindow*>(w_.data());
    if (!m) return gabim(env, "vendosQendren mbështetet vetëm te dritarja kryesore.");
    m->setCentralWidget(f->w_.data());
    return env.Undefined();
  }

  // ------------------------------------------ ekrane (stack), skeda, scroll
  Napi::Value ShtoNePirg(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    Elementi* f = nga(env, info[0]);
    if (!f) return env.Undefined();
    auto* p = qobject_cast<QStackedWidget*>(w_.data());
    if (!p) return gabim(env, "Ky element nuk është pirg ekranesh.");
    p->addWidget(f->w_.data());
    return env.Undefined();
  }

  Napi::Value AktivizoNePirg(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    Elementi* f = nga(env, info[0]);
    if (!f) return env.Undefined();
    auto* p = qobject_cast<QStackedWidget*>(w_.data());
    if (!p) return gabim(env, "Ky element nuk është pirg ekranesh.");
    p->setCurrentWidget(f->w_.data());
    return env.Undefined();
  }

  Napi::Value HiqNgaPirg(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    Elementi* f = nga(env, info[0]);
    if (!f) return env.Undefined();
    auto* p = qobject_cast<QStackedWidget*>(w_.data());
    if (!p) return gabim(env, "Ky element nuk është pirg ekranesh.");
    p->removeWidget(f->w_.data());
    return env.Undefined();
  }

  Napi::Value ShtoSkede(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    Elementi* f = nga(env, info[0]);
    if (!f) return env.Undefined();
    if (!kaString(info, 1)) return gabimTipi(env, "Pritej titulli i skedës (string).");
    auto* t = qobject_cast<QTabWidget*>(w_.data());
    if (!t) return gabim(env, "Ky element nuk është skedë.");
    t->addTab(f->w_.data(), ngaJS(info[1]));
    return env.Undefined();
  }

  Napi::Value VendosWidget(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    Elementi* f = nga(env, info[0]);
    if (!f) return env.Undefined();
    auto* z = qobject_cast<QScrollArea*>(w_.data());
    if (!z) return gabim(env, "Ky element nuk është zonë lëvizëse.");
    z->setWidget(f->w_.data());
    return env.Undefined();
  }

  // Pirg / skedë / zgjedhës: indeksi aktual. Listë: rreshti aktual.
  Napi::Value VendosIndeksin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_NUMRA(1);
    const int i = numriNga(info, 0);
    if (auto* p = qobject_cast<QStackedWidget*>(w_.data())) p->setCurrentIndex(i);
    else if (auto* t = qobject_cast<QTabWidget*>(w_.data())) t->setCurrentIndex(i);
    else if (auto* c = qobject_cast<QComboBox*>(w_.data())) c->setCurrentIndex(i);
    else if (auto* l = qobject_cast<QListWidget*>(w_.data())) l->setCurrentRow(i);
    return env.Undefined();
  }

  Napi::Value MerrIndeksin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    int i = -1;
    if (auto* p = qobject_cast<QStackedWidget*>(w_.data())) i = p->currentIndex();
    else if (auto* t = qobject_cast<QTabWidget*>(w_.data())) i = t->currentIndex();
    else if (auto* c = qobject_cast<QComboBox*>(w_.data())) i = c->currentIndex();
    else if (auto* l = qobject_cast<QListWidget*>(w_.data())) i = l->currentRow();
    return Napi::Number::New(env, i);
  }

  // ------------------------------------- listë, zgjedhës, kuti, progres
  Napi::Value ShtoRresht(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_STRING(0);
    auto* l = qobject_cast<QListWidget*>(w_.data());
    if (!l) return gabim(env, "Ky element nuk është listë.");
    l->addItem(ngaJS(info[0]));
    return env.Undefined();
  }

  Napi::Value PastroListen(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    if (auto* l = qobject_cast<QListWidget*>(w_.data())) l->clear();
    return env.Undefined();
  }

  Napi::Value ShtoOpsione(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    if (info.Length() < 1 || !info[0].IsArray()) return gabimTipi(env, "Pritej varg opsionesh.");
    auto* c = qobject_cast<QComboBox*>(w_.data());
    if (!c) return gabim(env, "Ky element nuk është zgjedhës.");
    Napi::Array a = info[0].As<Napi::Array>();
    for (uint32_t i = 0; i < a.Length(); ++i)
      c->addItem(QString::fromUtf8(a.Get(i).ToString().Utf8Value().c_str()));
    return env.Undefined();
  }

  Napi::Value EshteZgjedhur(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    auto* b = qobject_cast<QAbstractButton*>(w_.data());
    return Napi::Boolean::New(env, b && b->isChecked());
  }

  Napi::Value VendosZgjedhur(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    if (auto* b = qobject_cast<QAbstractButton*>(w_.data()))
      b->setChecked(info.Length() < 1 ? true : info[0].ToBoolean().Value());
    return env.Undefined();
  }

  Napi::Value VendosInterval(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_NUMRA(2);
    if (auto* p = qobject_cast<QProgressBar*>(w_.data()))
      p->setRange(numriNga(info, 0), numriNga(info, 1));
    return env.Undefined();
  }

  Napi::Value VendosVlere(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_NUMRA(1);
    if (auto* p = qobject_cast<QProgressBar*>(w_.data())) p->setValue(numriNga(info, 0));
    return env.Undefined();
  }

  Napi::Value VendosFormat(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_STRING(0);
    if (auto* p = qobject_cast<QProgressBar*>(w_.data())) p->setFormat(ngaJS(info[0]));
    return env.Undefined();
  }

  // ---------------------------------------------------------------- simulim
  // Klikim programatik i butonit/kutisë (njësoj sikur ta kishte shtypur përdoruesi).
  Napi::Value Kliko(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    if (auto* b = qobject_cast<QAbstractButton*>(w_.data())) b->click();
    return env.Undefined();
  }

  // Dërgon një ngjarje tasti (kodi Qt::Key + teksti) te elementi.
  Napi::Value SimuloTast(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GATI(env);
    KERKO_NUMRA(1);
    const QString teksti = kaString(info, 1) ? ngaJS(info[1]) : QString();
    QKeyEvent ngjarja(QEvent::KeyPress, numriNga(info, 0), Qt::NoModifier, teksti);
    QCoreApplication::sendEvent(w_.data(), &ngjarja);
    return env.Undefined();
  }
};

// ---------------------------------------------------------------------------
// Funksione globale
// ---------------------------------------------------------------------------
Napi::Value Fillo(const Napi::CallbackInfo& info) {
  if (!g_aplikacioni) {
    // I tregojmë Qt-së se ku ndodhet dosja platforms në folderin aktual
    QCoreApplication::addLibraryPath(QDir::currentPath());

    g_aplikacioni = new QApplication(g_argc, g_argv);
  }
  return info.Env().Undefined();
}

// Përpunon ngjarjet e Qt-së një herë. Kthen true nëse ka ende dritare të dukshme.
Napi::Value ProcesoNgjarjet(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!g_aplikacioni) return Napi::Boolean::New(env, false);
  QApplication::processEvents();
  bool kaDritare = false;
  const QWidgetList dritaret = QApplication::topLevelWidgets();
  for (QWidget* w : dritaret) {
    if (w->isVisible()) {
      kaDritare = true;
      break;
    }
  }
  return Napi::Boolean::New(env, kaDritare);
}

// --- Dialogë (bllokues: ekzekutojnë ciklin e tyre të Qt-së derisa mbyllen) ---

// mesazh(titulli, teksti, tekstiOk)
Napi::Value Mesazh(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!g_aplikacioni) return gabim(env, "Thirr fillo() përpara dialogëve.");
  if (!kaString(info, 0) || !kaString(info, 1) || !kaString(info, 2))
    return gabimTipi(env, "Përdorim: mesazh(titulli, teksti, tekstiOk).");
  QMessageBox kutia;
  kutia.setWindowTitle(ngaJS(info[0]));
  kutia.setText(ngaJS(info[1]));
  kutia.addButton(ngaJS(info[2]), QMessageBox::AcceptRole);
  kutia.exec();
  return env.Undefined();
}

// pyet(titulli, teksti, tekstiPo, tekstiJo) -> bool
Napi::Value Pyet(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!g_aplikacioni) return gabim(env, "Thirr fillo() përpara dialogëve.");
  if (!kaString(info, 0) || !kaString(info, 1) || !kaString(info, 2) || !kaString(info, 3))
    return gabimTipi(env, "Përdorim: pyet(titulli, teksti, tekstiPo, tekstiJo).");
  QMessageBox kutia;
  kutia.setWindowTitle(ngaJS(info[0]));
  kutia.setText(ngaJS(info[1]));
  QPushButton* po = kutia.addButton(ngaJS(info[2]), QMessageBox::AcceptRole);
  kutia.addButton(ngaJS(info[3]), QMessageBox::RejectRole);
  kutia.exec();
  return Napi::Boolean::New(env, kutia.clickedButton() == po);
}

// zgjidhSkedar(titulli, filtri) -> string | null
Napi::Value ZgjidhSkedar(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!g_aplikacioni) return gabim(env, "Thirr fillo() përpara dialogëve.");
  if (!kaString(info, 0) || !kaString(info, 1))
    return gabimTipi(env, "Përdorim: zgjidhSkedar(titulli, filtri).");
  QFileDialog d;
  d.setWindowTitle(ngaJS(info[0]));
  d.setNameFilter(ngaJS(info[1]));
  d.setFileMode(QFileDialog::ExistingFile);
  if (d.exec()) {
    const QStringList zgjedhur = d.selectedFiles();
    if (!zgjedhur.isEmpty()) return drejtJS(env, zgjedhur.first());
  }
  return env.Null();
}

}  // namespace

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Elementi::Init(env, exports);
  exports.Set("fillo", Napi::Function::New(env, Fillo));
  exports.Set("procesoNgjarjet", Napi::Function::New(env, ProcesoNgjarjet));
  exports.Set("mesazh", Napi::Function::New(env, Mesazh));
  exports.Set("pyet", Napi::Function::New(env, Pyet));
  exports.Set("zgjidhSkedar", Napi::Function::New(env, ZgjidhSkedar));
  return exports;
}

NODE_API_MODULE(eaglegui_native, Init)

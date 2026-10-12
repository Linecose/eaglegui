// EagleGUI – addon native (Qt Widgets + Node-API)
//
// Ideja: nuk e zëvendësojmë ciklin e ngjarjeve të Node. Në vend të kësaj, JS
// thërret periodikisht `procesoNgjarjet()`, i cili ia jep kontrollin Qt-së për
// pak çaste. Kështu callback-et JS thirren gjithmonë brenda një thirrjeje
// legjitime Node-API, pa nevojë për Node të modifikuar.

#include <napi.h>

#include <QApplication>
#include <QEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
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

Napi::Value gabim(Napi::Env env, const char* mesazhi) {
  Napi::Error::New(env, mesazhi).ThrowAsJavaScriptException();
  return env.Undefined();
}

Napi::Value gabimTipi(Napi::Env env, const char* mesazhi) {
  Napi::TypeError::New(env, mesazhi).ThrowAsJavaScriptException();
  return env.Undefined();
}

// Filtron ngjarjen e mbylljes së një widget-i
class FiltriMbylljes : public QObject {
 public:
  FiltriMbylljes(QObject* prindi, std::function<void()> kthimi)
      : QObject(prindi), kthimi_(std::move(kthimi)) {}

 protected:
  bool eventFilter(QObject* objekti, QEvent* ngjarja) override {
    if (ngjarja->type() == QEvent::Close) kthimi_();
    return QObject::eventFilter(objekti, ngjarja);
  }

 private:
  std::function<void()> kthimi_;
};

// ---------------------------------------------------------------------------
// Elementi – një mbështjellës i vetëm për çdo lloj widget-i
// ---------------------------------------------------------------------------
class Elementi : public Napi::ObjectWrap<Elementi> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function ctor = DefineClass(
        env, "Elementi",
        {
            InstanceMethod("vendosTekstin", &Elementi::VendosTekstin),
            InstanceMethod("merrTekstin", &Elementi::MerrTekstin),
            InstanceMethod("vendosPlaceholder", &Elementi::VendosPlaceholder),
            InstanceMethod("vendosTitullin", &Elementi::VendosTitullin),
            InstanceMethod("vendosMadhesine", &Elementi::VendosMadhesine),
            InstanceMethod("vendosStilin", &Elementi::VendosStilin),
            InstanceMethod("shtoFemije", &Elementi::ShtoFemije),
            InstanceMethod("ne", &Elementi::Ne),
            InstanceMethod("shfaq", &Elementi::Shfaq),
            InstanceMethod("fshih", &Elementi::Fshih),
            InstanceMethod("mbyll", &Elementi::Mbyll),
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
    } else if (tipi == "buton") {
      w_ = new QPushButton();
    } else if (tipi == "etikete") {
      w_ = new QLabel();
    } else if (tipi == "fushe") {
      w_ = new QLineEdit();
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

  Napi::Value VendosTekstin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!gati(env)) return env.Undefined();
    if (info.Length() < 1 || !info[0].IsString())
      return gabimTipi(env, "Pritej tekst (string).");
    const QString t = ngaJS(info[0]);
    if (auto* b = qobject_cast<QPushButton*>(w_.data())) b->setText(t);
    else if (auto* l = qobject_cast<QLabel*>(w_.data())) l->setText(t);
    else if (auto* f = qobject_cast<QLineEdit*>(w_.data())) f->setText(t);
    return env.Undefined();
  }

  Napi::Value MerrTekstin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!gati(env)) return env.Undefined();
    QString t;
    if (auto* b = qobject_cast<QPushButton*>(w_.data())) t = b->text();
    else if (auto* l = qobject_cast<QLabel*>(w_.data())) t = l->text();
    else if (auto* f = qobject_cast<QLineEdit*>(w_.data())) t = f->text();
    return Napi::String::New(env, t.toUtf8().toStdString());
  }

  Napi::Value VendosPlaceholder(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!gati(env)) return env.Undefined();
    if (info.Length() < 1 || !info[0].IsString())
      return gabimTipi(env, "Pritej tekst (string).");
    if (auto* f = qobject_cast<QLineEdit*>(w_.data()))
      f->setPlaceholderText(ngaJS(info[0]));
    return env.Undefined();
  }

  Napi::Value VendosTitullin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!gati(env)) return env.Undefined();
    if (info.Length() < 1 || !info[0].IsString())
      return gabimTipi(env, "Pritej titull (string).");
    w_->setWindowTitle(ngaJS(info[0]));
    return env.Undefined();
  }

  Napi::Value VendosMadhesine(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!gati(env)) return env.Undefined();
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
      return gabimTipi(env, "Pritej gjerësi dhe lartësi (numra).");
    w_->resize(info[0].As<Napi::Number>().Int32Value(),
               info[1].As<Napi::Number>().Int32Value());
    return env.Undefined();
  }

  Napi::Value VendosStilin(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!gati(env)) return env.Undefined();
    if (info.Length() < 1 || !info[0].IsString())
      return gabimTipi(env, "Pritej stil QSS/CSS (string).");
    w_->setStyleSheet(ngaJS(info[0]));
    return env.Undefined();
  }

  Napi::Value ShtoFemije(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!gati(env)) return env.Undefined();
    if (info.Length() < 1 || !info[0].IsObject() ||
        !info[0].As<Napi::Object>().InstanceOf(konstruktori.Value()))
      return gabimTipi(env, "Pritej një Elementi.");
    Elementi* femija = Napi::ObjectWrap<Elementi>::Unwrap(info[0].As<Napi::Object>());
    if (!femija->w_) return gabim(env, "Fëmija nuk është më i vlefshëm.");
    QLayout* paraqitja = w_->layout();
    if (!paraqitja) paraqitja = new QVBoxLayout(w_.data());
    paraqitja->addWidget(femija->w_.data());
    return env.Undefined();
  }

  Napi::Value Ne(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!gati(env)) return env.Undefined();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
      return gabimTipi(env, "Përdorim: ne(ngjarje, funksion).");

    const std::string ngjarja = info[0].As<Napi::String>().Utf8Value();
    Thirrja thirrja = krijoThirrje(info[1].As<Napi::Function>());

    if (ngjarja == "klikim") {
      auto* b = qobject_cast<QPushButton*>(w_.data());
      if (!b) return gabim(env, "Ngjarja 'klikim' mbështetet vetëm te Buton.");
      QObject::connect(b, &QPushButton::clicked, b, [env, thirrja]() {
        Napi::HandleScope scope(env);
        thirr(thirrja);
      });
    } else if (ngjarja == "ndryshim") {
      auto* f = qobject_cast<QLineEdit*>(w_.data());
      if (!f) return gabim(env, "Ngjarja 'ndryshim' mbështetet vetëm te FushaTeksti.");
      QObject::connect(f, &QLineEdit::textChanged, f, [env, thirrja](const QString& t) {
        Napi::HandleScope scope(env);
        thirr(thirrja, {Napi::String::New(env, t.toUtf8().toStdString())});
      });
    } else if (ngjarja == "mbyllje") {
      auto* filtri = new FiltriMbylljes(w_.data(), [env, thirrja]() {
        Napi::HandleScope scope(env);
        thirr(thirrja);
      });
      w_->installEventFilter(filtri);
    } else {
      return gabim(env, "Ngjarje e panjohur. Të mbështetura: klikim, ndryshim, mbyllje.");
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
};

// ---------------------------------------------------------------------------
// Funksione globale
// ---------------------------------------------------------------------------
Napi::Value Fillo(const Napi::CallbackInfo& info) {
  if (!g_aplikacioni) g_aplikacioni = new QApplication(g_argc, g_argv);
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

}  // namespace

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Elementi::Init(env, exports);
  exports.Set("fillo", Napi::Function::New(env, Fillo));
  exports.Set("procesoNgjarjet", Napi::Function::New(env, ProcesoNgjarjet));
  return exports;
}

NODE_API_MODULE(eaglegui_native, Init)

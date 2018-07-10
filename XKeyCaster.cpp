#include <QDesktopWidget>
#include <QApplication>
#include <QObject>
#include <QX11Info>
#include <QTimer>
#include <QAction>
#include <QMenu>
#include <QWidget>
#include <QObject>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QLabel>
#include <QPainter>

#include <X11/XKBlib.h>
#include <X11/keysym.h>

#include "PreferencesDialog.hpp"
#include "XKeyCaster.hpp"

///////////////////////////////////////
class XKeyCasterWindow : public QWidget
{
Q_OBJECT

public:
  XKeyCasterWindow(): QWidget() {
    this -> setWindowFlags(Qt::WindowStaysOnTopHint |
                           Qt::X11BypassWindowManagerHint |
                           Qt::WindowMinimizeButtonHint |
                           Qt::FramelessWindowHint |
                           Qt::WindowCloseButtonHint);
    this -> setObjectName("XKeyCasterWindow");
    this -> setStyleSheet("#XKeyCasterWindow {background-color: #303030; border: none} ");
    QVBoxLayout* baseLayout = new QVBoxLayout();
    this -> setLayout(baseLayout);

    label_ = new QLabel("");
    label_ -> setStyleSheet("QLabel { color : white; }");

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
    effect -> setBlurRadius(10);
    effect -> setColor(QColor("#000000"));
    effect -> setOffset(2, 2);
    label_ -> setGraphicsEffect(effect);

    setContextMenuPolicy(Qt::CustomContextMenu);

    baseLayout -> addWidget(label_);
    //setAttribute(Qt::WA_TranslucentBackground, true);
  }

  void paintEvent(QPaintEvent * event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setPen("#777777");
    painter.drawRect(0, 0, width() - 1, height() - 1);
  }

  void mouseMoveEvent(QMouseEvent* event) {
    if(oldButton_ == Qt::LeftButton) {
      auto gpos = event -> globalPos();
      auto geometry = this -> geometry();

      auto dx = geometry.x() - (gpos.x() - oldX_);
      auto dy = geometry.y() - (gpos.y() - oldY_);

      emit movePosition(dx, dy);
    }
  }

  void mousePressEvent(QMouseEvent* event) {
    QWidget::mousePressEvent(event);

    oldButton_ = event -> button();
    oldX_ = event -> x();
    oldY_ = event -> y();
  }

  QString text() const {
    return label_ -> text();
  }

  void setText(const QString &txt) {
    label_ -> setText(txt);
  }

  void setLabelStyleSheet(const QString &styleSheet) {
    label_ -> setStyleSheet(styleSheet);
  }

  void setLabelFont(const QFont &font) {
    label_ -> setFont(font);
  }

signals:
  void movePosition(int dx, int dy);

private:
  int oldX_, oldY_;
  Qt::MouseButton oldButton_;
  QLabel* label_;
};
///////////////////////////////////////

XKeyCaster::XKeyCaster() : QObject()
{
  preferences_ = initPreferences();

  shift_ = control_ = keycount = meta_ = 0;

  display_ = QX11Info::display();
  XQueryKeymap(display_, keys_last);

  for(int i = 0; i < preferences_.line; ++i) {
    auto win = new XKeyCasterWindow();
    connect(win, &QWidget::customContextMenuRequested,
            [=](const QPoint &point) { showContextMenu(point, win); }
            );
    connect(win, &XKeyCasterWindow::movePosition, this, &XKeyCaster::movePosition);
    windowList_.push_back(win);
  }

  auto timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(keyCapture()));
  timer -> start(10);

  auto desktop = QApplication::desktop();
  auto geometry = desktop -> screenGeometry(-1);

  windowList_[0] -> setText("intialize position...");
  windowList_[0] -> adjustSize();
  windowList_[0] -> move(geometry.width() / 2 - windowList_[0] -> width() / 2,
                         geometry.height() / 2 - windowList_[0] -> height() / 2);
  windowList_[0] -> show();
}

void XKeyCaster::keyCapture() {
  bool keyPush = false;

  XQueryKeymap(display_, keys_current);
  for (int i = 0; i < 32; i++) {
    if (keys_current[i] != keys_last[i]) {
      int buf[256];
      int len = getBitPositions(keys_current[i] ^ keys_last[i], buf);

      for(int j = 0; j < len; ++j) {
        int keycode = i * 8 + buf[j];
        int keysym = XkbKeycodeToKeysym(display_, keycode, 0, shift_);
        keyPush = true;

        // numlock
        if(keysym >= XK_KP_Home && keysym <= XK_KP_Delete) {
          if(isNumLock()) {
            keysym = XkbKeycodeToKeysym(display_, keycode, 0, 1);
          }
        }

        // capslock
        if(keysym >= XK_a && keysym <= XK_z) {
          if(isCapsLock()) {
            keysym = XkbKeycodeToKeysym(display_, keycode, 0, 1);
          }
        }

        // Push key
        if((unsigned char)keys_current[i] > (unsigned char)keys_last[i]) {
          switch (keysym) {
          case XK_Shift_R:
          case XK_Shift_L:
            shift_ = 1;
            keyPush = false;
            break;
          case XK_Control_L:
          case XK_Control_R:
            keyPush = false;
            control_ = 1;
            break;
          case XK_Alt_L:
          case XK_Alt_R:
            keyPush = false;
            meta_ = 1;
            break;
          default:
            keyPush = setWindowText(keycode, keysym);
            break;
          }
        }
        // Release key
        else {
          keyPush = false;
          switch (keysym) {
          case XK_Shift_R:
          case XK_Shift_L:
            shift_ = 0;
            break;
          case XK_Control_L:
          case XK_Control_R:
            control_ = 0;
            break;
          case XK_Alt_L:
          case XK_Alt_R:
            meta_ = 0;
            break;
          default:
            break;
          }
        }

      }
      keys_last[i] = keys_current[i];
    }
  }

  if(!keyPush) {
    ++keycount;
    if(keycount > 100) keycount = 100;
  }
  else keycount = 0;
}

void XKeyCaster::movePosition(int dx, int dy) {
  for(auto& window: windowList_) {
    auto geometry = window -> geometry();

    window -> setGeometry(geometry.x() - dx, geometry.y() - dy,
                          geometry.width(), geometry.height());
  }
}

bool XKeyCaster::setWindowText(int keycode, int keysym) {
  bool ret = true;

  if(windowList_[0] -> text() == "intialize position...") {
    windowList_[0] -> setText("");
    windowList_[0] -> adjustSize();
  }

  QString text = windowList_[0] -> text();
  QString nKey = "";
  if(keycount >= 50) {
    shiftWindowText();
    text = "";
    windowList_[0] -> setText("");
    windowList_[0] -> adjustSize();
  }
  if (control_) nKey += "C + ";
  if (meta_) nKey += "M + ";
  if (shift_) {
    auto keysym1 = XkbKeycodeToKeysym(display_, keycode, 0, 0);
    auto keysym2 = XkbKeycodeToKeysym(display_, keycode, 0, 1);

    if(keysym1 == keysym2) nKey += "Shift + ";
  }

  QString keyname = getKeyName(XKeysymToString(keysym));
  if(keyname.length() > 1) keyname = "[" + keyname + "]";
  nKey += keyname;

  if(preKey_ != nKey || text == "") {
    if(text != "") text += " ";
    windowList_[0] -> setText(text + nKey);
  }
  else {
    QString buf = " " + text;
    int idx = buf.lastIndexOf(" ", -1);
    QString lastKey = buf.mid(idx + 1);

    if(lastKey.left(1) != "*" || lastKey == "*") {
      windowList_[0] -> setText(text + " *2");
    }
    else {
      text.chop(lastKey.length() + 1);
      windowList_[0] -> setText(text + QString(" *%1").arg(lastKey.mid(1).toInt() + 1));
    }
  }
  windowList_[0] -> adjustSize();

  if (keysym == XK_Return) {
    keycount = 100;
    ret = false;
  }
  preKey_ = nKey;

  return ret;
}

QString XKeyCaster::getKeyName(const QString& name) const {
  QMap<QString, QString> map;

  map["Escape"]       = "ESC";
  map["BackSpace"]    = "BS";
  map["space"]        = "SP";
  map["Return"]       = "RET";

  map["exclam"]       = "!";
  map["at"]           = "@";
  map["numbersign"]   = "#";
  map["dollar"]       = "$";
  map["percent"]      = "%";
  map["asciicircum"]  = "^";
  map["ampersand"]    = "&";
  map["asterisk"]     = "*";
  map["parenleft"]    = "(";
  map["parenright"]   = ")";
  map["minus"]        = "-";
  map["underscore"]   = "_";
  map["equal"]        = "=";
  map["plus"]         = "+";
  map["bracketleft"]  = "[";
  map["bracketright"] = "]";
  map["braceleft"]    = "{";
  map["braceright"]   = "}";
  map["semicolon"]    = ";";
  map["colon"]        = ":";
  map["apostrophe"]   = "'";
  map["quotedbl"]     = "\"";
  map["grave"]        = "`";
  map["asciitilde"]   = "~";
  map["backslash"]    = "\\";
  map["bar"]          = "|";
  map["comma"]        = ",";
  map["less"]         = "<";
  map["greater"]      = ">";
  map["period"]       = ".";
  map["slash"]        = "/";
  map["question"]     = "?";

  auto r = map[name];
  if (r != "") return r;

  return name;
}

int XKeyCaster::getBitPositions(unsigned char x, int r[]) const
{
  int i = 0;

  for(int j = 0 ; j < 12; ++j) {
    if (x & (1 << j)) {
      r[i++] = j;
    }
  }

  return i;
}

void XKeyCaster::showPreferencesDialog()
{
  Preferences pref = preferences_;
  PreferencesDialog* dialog = new PreferencesDialog(&pref);

  if(dialog -> exec() == QDialog::Accepted) {
    preferences_ = pref;

    // Reset window list
    if(preferences_.line > windowList_.size()) {
      int l = preferences_.line - windowList_.size();
      for(int i = 0; i < l; ++i) {
        auto win = new XKeyCasterWindow();
        connect(win, &QWidget::customContextMenuRequested,
                [=](const QPoint &point) { showContextMenu(point, win); }
                );
        connect(win, &XKeyCasterWindow::movePosition, this, &XKeyCaster::movePosition);
        windowList_.push_back(win);
      }
    }
    else if(preferences_.line < windowList_.size()) {
      int l = windowList_.size() - preferences_.line;
      for(int i = 0; i < l; ++i) {
        delete windowList_.back();
        windowList_.pop_back();
      }
    }

    // Reset font & colors
    for(auto& window: windowList_) {
      window -> setStyleSheet(QString("#XKeyCasterWindow {background-color: %1;"
                                      " border: 1px solid #555555; }"
                                      ).arg(preferences_.backgroundColor.name()));
      window -> setLabelStyleSheet(QString("QLabel { color : %1; }")
                                   .arg(preferences_.textColor.name()));
      window -> setLabelFont(preferences_.font);
      window -> adjustSize();
    }

    // Reset position & size
    for(int i = windowList_.size() - 1; i != 0; --i) {
      auto geometry = windowList_[0] -> geometry();

      windowList_[i] -> setGeometry(geometry.x(),
                                    geometry.y() - ((geometry.height() + preferences_.margine) * (i)),
                                    1, 1);
      windowList_[i] -> adjustSize();
    }
  }
}

void XKeyCaster::showContextMenu(const QPoint &point, XKeyCasterWindow* window)
{
  QMenu menu;
  QAction* action;

  action = menu.addAction("&Preferences");
  connect(action, &QAction::triggered,
          [=] { showPreferencesDialog(); }
          );
  action = menu.addAction("&Quit");
  connect(action, &QAction::triggered,
          [=] { QApplication::quit(); }
          );

  menu.exec(window -> mapToGlobal(point));
}

bool XKeyCaster::isCapsLock() const
{
  bool caps_state = false;
  unsigned n;
  XkbGetIndicatorState(display_, XkbUseCoreKbd, &n);
  caps_state = ((n & 0x01) == 1);

  return caps_state;
}

bool XKeyCaster::isNumLock() const
{
  bool caps_state = false;
  unsigned n;
  XkbGetIndicatorState(display_, XkbUseCoreKbd, &n);
  caps_state = ((n & 0x02) == 2);

  return caps_state;
}

void XKeyCaster::shiftWindowText()
{
  for(int i = windowList_.size() - 1; i != 0; --i) {
    if(windowList_[i - 1] -> text() != "") {
      auto geometry = windowList_[0] -> geometry();

      windowList_[i] -> setGeometry(geometry.x(),
                                    geometry.y() - ((geometry.height() + preferences_.margine) * (i)),
                                    1, 1);

      windowList_[i] -> setText(windowList_[i - 1] -> text());
      windowList_[i] -> show();
      windowList_[i] -> adjustSize();
    }
  }
}

Preferences XKeyCaster::initPreferences() const
{
  Preferences pref;

  pref.textColor       = QColor(Qt::white);
  pref.backgroundColor = QColor("#303030");
  pref.line            = 5;
  pref.margine         = 5;

  return pref;
}

#include "XKeyCaster.moc"

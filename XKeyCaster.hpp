#ifndef XKEYCASTER_HPP
#define XKEYCASTER_HPP

#include <QObject>
#include <X11/Xlib.h>

#include "Preferences.hpp"

class XKeyCasterWindow;
class XKeyCaster : public QObject
{
Q_OBJECT
public:
  XKeyCaster();

public slots:
  void keyCapture();

private:
  void movePosition(int dx, int dy);
  bool setWindowText(int keycode, int keysym);
  QString getKeyName(const QString& name) const;
  int getBitPositions(unsigned char x, int r[]) const;
  void showPreferencesDialog();
  void showContextMenu(const QPoint &point, XKeyCasterWindow* window);

  bool isCapsLock() const;
  bool isNumLock() const;

  void shiftWindowText();
  Preferences initPreferences() const;

  char keys_current[32], keys_last[32];
  int shift_, control_, meta_;
  int keycount;

  QList<XKeyCasterWindow*> windowList_;
  QString preKey_;
  Preferences preferences_;
  Display* display_;
};

#endif

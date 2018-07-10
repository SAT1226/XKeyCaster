#ifndef PREFERENCESDIALOG_HPP
#define PREFERENCESDIALOG_HPP
#include <QtWidgets/QDialog>

namespace Ui {
  class Dialog;
}
struct Preferences;
class QColor;

class PreferencesDialog : public QDialog {
public:
  PreferencesDialog(Preferences* preferences);
  ~PreferencesDialog();

private:
  QPixmap CreateColorPixmap(const QColor& color);

  Preferences* preferences_;
  Ui::Dialog* ui_;
};

#endif

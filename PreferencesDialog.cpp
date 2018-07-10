#include <QFontDialog>
#include <QColor>
#include <QPainter>
#include <QIcon>
#include <iostream>
#include <QColorDialog>

#include "PreferencesDialog.hpp"
#include "ui_PreferencesDialog.h"
#include "Preferences.hpp"

PreferencesDialog::PreferencesDialog(Preferences* preferences):
  QDialog(), preferences_(preferences), ui_(new Ui::Dialog)
{
  ui_ -> setupUi(this);

  auto fontButton = findChild<QPushButton*>("FontButton");
  fontButton -> setText(preferences_ -> font.family() + ":" +
                        QString::number(preferences_ -> font.pointSize()));
  connect(fontButton, &QPushButton::clicked,
          [=] {
            bool ok;
            QFont font = QFontDialog::getFont(&ok, preferences_ -> font, this);
            if(ok) {
              preferences_ -> font = font;
              fontButton -> setText(preferences_ -> font.family() + ":" +
                                    QString::number(preferences_ -> font.pointSize()));
            }
          });

  auto textColorButton = findChild<QPushButton*>("TextColorButton");
  textColorButton -> setIcon(QIcon(CreateColorPixmap(preferences_ -> textColor)));
  connect(textColorButton, &QPushButton::clicked,
          [=] {
            auto color = QColorDialog::getColor(preferences_ -> textColor, this);
            if(color.isValid()) preferences_ -> textColor = color;

            textColorButton -> setIcon(QIcon(CreateColorPixmap(preferences_ -> textColor)));
          });

  auto backgroundColorButton = findChild<QPushButton*>("BackgroundButton");
  backgroundColorButton -> setIcon(QIcon(CreateColorPixmap(preferences_ -> backgroundColor)));
  connect(backgroundColorButton, &QPushButton::clicked,
          [=] {
            auto color = QColorDialog::getColor(preferences_ -> backgroundColor, this);
            if(color.isValid()) preferences_ -> backgroundColor = color;

            backgroundColorButton -> setIcon(QIcon(CreateColorPixmap(preferences_ -> backgroundColor)));
          });

  auto lineSpinBox = findChild<QSpinBox*>("LineSpinBox");
  lineSpinBox -> setValue(preferences_ -> line);
  connect(lineSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          [=] {
            preferences_ -> line = lineSpinBox -> value();
          });

  auto marginSpinBox = findChild<QSpinBox*>("MarginSpinBox");
  marginSpinBox -> setValue(preferences_ -> margine);
  connect(marginSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          [=] {
            preferences_ -> margine = marginSpinBox -> value();
          });

  auto buttonBox = findChild<QDialogButtonBox*>("buttonBox");
  connect(buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &PreferencesDialog::reject);
}

QPixmap PreferencesDialog::CreateColorPixmap(const QColor& color)
{
  QPixmap pixmap(100, 100);
  pixmap.fill(color);

  QPainter painter(&pixmap);
  painter.setPen(QColor(100,100,100,255));
  painter.drawRect(0, 0, pixmap.height() - 1, pixmap.width() - 1);

  return pixmap;
}

PreferencesDialog::~PreferencesDialog()
{
  delete ui_;
}

#include <QApplication>
#include "XKeyCaster.hpp"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  XKeyCaster win;

  return app.exec();
}

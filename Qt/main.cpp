#include "lasercutter.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/iconWindow.png"));
    a.setStyle(QStyleFactory::create("Fusion"));
    LaserCutter w;
    w.show();
    return a.exec();
}

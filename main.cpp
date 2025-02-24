#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w(nullptr, argv[1]);
    w.setWindowState(Qt::WindowFullScreen);
    w.show();
    return a.exec();
}

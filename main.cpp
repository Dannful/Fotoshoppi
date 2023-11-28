#include "mainwindow.h"

#include <QApplication>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    if(!w.requestImage()) {
        a.exit();
        return 0;
    }
    w.show();
    return a.exec();
}

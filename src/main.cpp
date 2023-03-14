#include "mainwindow.h"

#include <QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    MainWindow mainWindow(nullptr);
    mainWindow.show();

    return app.exec();
}

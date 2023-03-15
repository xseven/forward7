#include "mainwindow.h"

#include "logeventfilter.h"

#include <QApplication>

namespace {

void logOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    auto mainWindow = qApp->property("mainwindow").value<MainWindow*>();
    QCoreApplication::postEvent(mainWindow, new LogEvent(msg));
}

}

int main(int argc, char** argv)
{
    qInstallMessageHandler(logOutput);

    QApplication app(argc, argv);

    MainWindow mainWindow(nullptr);
    mainWindow.show();

    qApp->setProperty("mainwindow", QVariant::fromValue(&mainWindow));

    return app.exec();
}

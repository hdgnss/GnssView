#include "mainwindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application icon
    #ifdef Q_OS_MAC
    app.setWindowIcon(QIcon(":icons/icon.icns"));
    #else
    app.setWindowIcon(QIcon(":icons/icon.ico"));
    #endif
    
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

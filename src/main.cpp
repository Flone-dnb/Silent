// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

// Qt
#include <QApplication>
#include <QTimer>

// Custom
#include "../src/View/MainWindow/mainwindow.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.setWindowFlag(Qt::FramelessWindowHint);
    w.show();

    QTimer::singleShot(0, &w, &MainWindow::onExecCalled);

    return a.exec();
}

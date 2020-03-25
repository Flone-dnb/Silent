// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "globalmessagewindow.h"
#include "ui_globalmessagewindow.h"

GlobalMessageWindow::GlobalMessageWindow(QString sServerMessage, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GlobalMessageWindow)
{
    ui->setupUi(this);


    ui->plainTextEdit->setPlainText(sServerMessage);


    // Hide maximize & minimize buttons

    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowMaximizeButtonHint;
    flags &= ~Qt::WindowMinimizeButtonHint;
    setWindowFlags(flags);
}

GlobalMessageWindow::~GlobalMessageWindow()
{
    delete ui;
}

void GlobalMessageWindow::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_Return )
    {
        on_pushButton_clicked();
    }
}

void GlobalMessageWindow::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev)

    deleteLater();
}

void GlobalMessageWindow::on_pushButton_clicked()
{
    close();
}

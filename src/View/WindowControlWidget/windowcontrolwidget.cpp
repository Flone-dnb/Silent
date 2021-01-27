// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "windowcontrolwidget.h"
#include "ui_windowcontrolwidget.h"

#include "View/StyleAndInfoPaths.h"

WindowControlWidget::WindowControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WindowControlWidget)
{
    ui->setupUi(this);

    ui->pushButton_hide->setProperty("cssClass", "hide-icon");
    ui->pushButton_max->setProperty("cssClass", "max-icon");
    ui->pushButton_close->setProperty("cssClass", "close-icon");
}

WindowControlWidget::~WindowControlWidget()
{
    delete ui;
}

void WindowControlWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    deleteLater();
}

void WindowControlWidget::on_pushButton_close_clicked()
{
    emit signalClose();
}

void WindowControlWidget::on_pushButton_max_clicked()
{
    emit signalMaximize();
}

void WindowControlWidget::on_pushButton_hide_clicked()
{
    emit signalHide();
}

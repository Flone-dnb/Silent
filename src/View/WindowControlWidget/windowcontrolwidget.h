// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

#include <QWidget>

namespace Ui {
class WindowControlWidget;
}

class WindowControlWidget : public QWidget
{
    Q_OBJECT

signals:
    void signalClose();
    void signalMaximize();
    void signalHide();

public:
    explicit WindowControlWidget(QWidget *parent = nullptr);
    ~WindowControlWidget();

private slots:
    void on_pushButton_close_clicked();

    void on_pushButton_max_clicked();

    void on_pushButton_hide_clicked();

private:
    Ui::WindowControlWidget *ui;
};

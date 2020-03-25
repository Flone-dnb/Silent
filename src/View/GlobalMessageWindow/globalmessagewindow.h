// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

#include <QMainWindow>

namespace Ui {
class GlobalMessageWindow;
}

class GlobalMessageWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GlobalMessageWindow(QString sServerMessage, QWidget *parent = nullptr);
    ~GlobalMessageWindow() override;

protected:
    void keyPressEvent (QKeyEvent* event) override;
    void closeEvent(QCloseEvent* ev) override;

private slots:
    void on_pushButton_clicked();

private:
    Ui::GlobalMessageWindow *ui;
};

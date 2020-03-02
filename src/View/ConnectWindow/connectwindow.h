// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// Qt
#include <QMainWindow>

// STL
#include <string>


class QMouseEvent;

namespace Ui
{
    class connectWindow;
}


// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


class connectWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit connectWindow      (QWidget *parent = nullptr);



    void     setUserName        (std::string userName);
    void     setConnectString   (std::string sConnectString);
    void     setPort            (std::string sPort);
    void     setPassword        (std::wstring sPass);



    ~connectWindow              ();

signals:

    void  connectTo             (std::string adress, std::string port, std::string userName, std::wstring sPass);
    void  showMainWindow        ();

protected:

    void  keyPressEvent         (QKeyEvent* event);
    void  closeEvent            (QCloseEvent *event);

private slots:

    void  on_pushButton_clicked ();

private:

    Ui::connectWindow *ui;
};

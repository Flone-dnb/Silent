// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// Qt
#include <QPlainTextEdit>



class CustomQPlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:

    CustomQPlainTextEdit(QWidget* parent = nullptr);

signals:

    void signalReturnPressed();


protected:

    void keyPressEvent(QKeyEvent *event);
};

// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "customqplaintextedit.h"

// Qt
#include <QKeyEvent>


CustomQPlainTextEdit::CustomQPlainTextEdit(QWidget* parent) : QPlainTextEdit(parent)
{

}

void CustomQPlainTextEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return)
    {
        emit signalReturnPressed();
    }
    else
    {
        QPlainTextEdit::keyPressEvent(event);
    }
}

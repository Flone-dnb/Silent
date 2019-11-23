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

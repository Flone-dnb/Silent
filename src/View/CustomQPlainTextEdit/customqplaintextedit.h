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

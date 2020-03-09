#include "roompassinputwindow.h"
#include "ui_roompassinputwindow.h"

#include <QKeyEvent>

RoomPassInputWindow::RoomPassInputWindow(QString sRoomName, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RoomPassInputWindow)
{
    ui->setupUi(this);

    ui->label->setText("Please enter the password to enter the room \"" + sRoomName + "\".");

    this->sRoomName = sRoomName;
}

void RoomPassInputWindow::keyPressEvent(QKeyEvent *event)
{
    if ( event ->key() == Qt::Key_Return )
    {
        on_pushButton_clicked();
    }
}

void RoomPassInputWindow::on_pushButton_clicked()
{
    emit signalEnterRoomWithPassword(sRoomName, ui->lineEdit->text());

    close();
}

void RoomPassInputWindow::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev)

    deleteLater();
}


RoomPassInputWindow::~RoomPassInputWindow()
{
    delete ui;
}

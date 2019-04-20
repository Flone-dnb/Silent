#include "connectwindow.h"
#include "ui_connectwindow.h"

#include <QMessageBox>
#include <QMouseEvent>

connectWindow::connectWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::connectWindow)
{
    ui->setupUi(this);
    setFixedSize(width(), height());
}

connectWindow::~connectWindow()
{
    delete ui;
}

void connectWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
    emit showMainWindow();
}

void connectWindow::on_pushButton_clicked()
{
    if (ui->lineEdit_3->text().size() > 1)
    {
        bool bOnlyEnglish = true;
        for (unsigned int i = 0; i < ui->lineEdit_3->text().size() ;i++)
        {
            if ( (ui->lineEdit_3->text().at(i).unicode() > 122) || (ui->lineEdit_3->text().at(i).unicode() < 48))
            {
                bOnlyEnglish = false;
                break;
            }
            else
            {
                if ( (ui->lineEdit_3->text().at(i).unicode() >=58) && (ui->lineEdit_3->text().at(i).unicode() <= 64) )
                {
                    bOnlyEnglish = false;
                    break;
                }
                else
                {
                    if ( (ui->lineEdit_3->text().at(i).unicode() >= 91) && (ui->lineEdit_3->text().at(i).unicode() <= 96) )
                    {
                        bOnlyEnglish = false;
                        break;
                    }
                }
            }
        }

        if (bOnlyEnglish)
        {
            if (ui->lineEdit->text().size() >= 7)
            {
                hide();
                emit connectTo(ui->lineEdit->text().toStdString(),ui->lineEdit_2->text().toStdString(),ui->lineEdit_3->text().toStdString());
            }
            else
            {
               QMessageBox::information(nullptr,"IP","Please fill IPv4 adress field.\n");
            }
        }
        else
        {
            QMessageBox::information(nullptr,"Name","User name must consist only of A-Z, a-z, 0-9 characters.\n");
        }
    }
    else
    {
        QMessageBox::information(nullptr,"Name","User name must be 2 characters or longer.\n");
    }
}

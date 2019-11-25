#include "connectwindow.h"
#include "ui_connectwindow.h"


// Qt
#include <QMessageBox>
#include <QMouseEvent>


// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


connectWindow::connectWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::connectWindow)
{
    ui ->setupUi(this);
    setFixedSize(width(), height());


    ui ->lineEdit_username ->setMaxLength( 20 );
}





void connectWindow::setUserName(std::string userName)
{
    ui ->lineEdit_username ->setText( QString::fromStdString(userName) );
}

void connectWindow::closeEvent(QCloseEvent *event)
{
    event ->ignore();
    hide();

    emit showMainWindow();
}

void connectWindow::on_pushButton_clicked()
{
    // Check if the entered name has forbidden characters.
    // Allowed: A-Z, a-z, 0-9.

    const char cUserNameMinLength = 2;

    if ( ui ->lineEdit_username ->text() .size() <= cUserNameMinLength )
    {
        QMessageBox::information(this, "Input Error", "The user name must be "
                                 + QString::number( static_cast <int> (cUserNameMinLength) )
                                 + " characters or longer.");

        return;
    }




    // Check if name contains only allowed chars

    bool bOnlyAllowed = true;

    for (int i = 0;   i < ui ->lineEdit_username ->text() .size();   i++)
    {
        // Filter ASCII chars to only [48-122]

        if ( (ui ->lineEdit_username ->text() .at(i) .unicode() >= 123)
             ||
             (ui ->lineEdit_username ->text() .at(i) .unicode() <= 47) )
        {

            bOnlyAllowed = false;
            break;

        }
        else
        {

            // ASCII chars in [58-64] are forbidden

            if ( (ui ->lineEdit_username ->text() .at(i) .unicode() >= 58)
                 &&
                 (ui ->lineEdit_username ->text() .at(i) .unicode() <= 64) )
            {

                bOnlyAllowed = false;
                break;

            }
            else
            {
                // ASCII chars in [91-96] are forbidden

                if ( (ui ->lineEdit_username ->text() .at(i) .unicode() >= 91)
                     &&
                     (ui ->lineEdit_username ->text() .at(i) .unicode() <= 96) )
                {

                    bOnlyAllowed = false;
                    break;

                }
            }

        }
    }




    // Check if only allowed

    if ( bOnlyAllowed == false )
    {
        QMessageBox::information(this, "Input Error", "The user name must consist only of A-Z, a-z, 0-9 characters.");

        return;
    }




    // Check if the IP is entered

    if ( ui ->lineEdit_ip ->text() .size() <= 6 )
    {
        QMessageBox::information(this, "Input Error", "Please fill the IPv4 adress field.");

        return;
    }




    // Check if the port is entered
    if ( ui ->lineEdit_port ->text() .size() <= 1 )
    {
        QMessageBox::information(this, "Input Error", "Please fill the Port field.");

        return;
    }




    // Send signal to connect

    hide();
    emit connectTo( ui ->lineEdit_ip       -> text() .toStdString(),
                    ui ->lineEdit_port     -> text() .toStdString(),
                    ui ->lineEdit_username -> text() .toStdString(),
                    ui ->lineEdit_pass     -> text() .toStdWString() );
}


void connectWindow::keyPressEvent(QKeyEvent *event)
{
    if ( event ->key() == Qt::Key_Return )
    {
        on_pushButton_clicked();
    }
}


connectWindow::~connectWindow()
{
    delete ui;
}

#include "mainwindow.h"
#include "ui_mainwindow.h"

// Custom
#include "View/ConnectWindow/connectwindow.h"
#include "Controller/controller.h"

// Qt
#include <QMessageBox>
#include <QMouseEvent>
#include <QAction>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    pController = new Controller(this);
    pConnectWindow = nullptr;

    // Set ContextMenu to ListWidget
    ui->listWidget_2->setContextMenuPolicy(Qt::CustomContextMenu);

    // Connect window
    pConnectWindow = new connectWindow(nullptr);
    connect(pConnectWindow, &connectWindow::connectTo,      this, &MainWindow::connectTo);
    connect(pConnectWindow, &connectWindow::showMainWindow, this, &MainWindow::showThisWindow);

    // connect to 'this'
    // we use this to send signals to main thread when working with GI
    connect(this, &MainWindow::signalTypeOnScreen,                 this,&MainWindow::typeSomeOnScreen);
    connect(this, &MainWindow::signalEnableInteractiveElements,    this,&MainWindow::slotEnableInteractiveElements);
    connect(this, &MainWindow::signalShowMessage,                  this, &MainWindow::slotShowMessage);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    pController->stop();
}

void MainWindow::slotShowMessage(char type, std::string message)
{
    if (type == 0)
    {
        QMessageBox::warning(nullptr,"Warning",QString::fromStdString(message));
    }
    else if (type == 1)
    {
        QMessageBox::information(nullptr,"Information",QString::fromStdString(message));
    }
}

void MainWindow::typeSomeOnScreen(QString text)
{
    ui->plainTextEdit->appendPlainText(text);
}

void MainWindow::slotEnableInteractiveElements(bool bMenu, bool bTypeAndSend)
{
    if (bMenu)
    {
        ui->actionConnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(true);
    }
    else
    {
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(false);
    }

    if (bTypeAndSend)
    {
        ui->pushButton->setEnabled(true);
        ui->plainTextEdit_2->setEnabled(true);
    }
    else
    {
        ui->pushButton->setEnabled(false);
        ui->plainTextEdit_2->setEnabled(false);
    }
}

void MainWindow::connectTo(std::string adress, std::string port, std::string userName)
{
    show();
    ui->plainTextEdit->clear();
    pController->connectTo(adress,port,userName);
}

void MainWindow::showThisWindow()
{
    show();
}

void MainWindow::printOutput(std::string text, bool bEmitSignal)
{
    if (bEmitSignal)
    {
        // This function (printOutput) was called from another thread (not main thread)
        // so if we will append text to 'plaintTextEdit' we will have errors because you
        // cannot append info from another thread.
        // Because of that we will emit signal to main thread to append text
        // Right? I dunno "it just works". :p
        emit signalTypeOnScreen(QString::fromStdString(text));
    }
    else
    {
        ui->plainTextEdit->appendPlainText(QString::fromStdString(text));
    }
}

void MainWindow::printOutputW(std::wstring text, bool bEmitSignal)
{
    if (bEmitSignal)
    {
        // This function (printOutput) was called from another thread (not main thread)
        // so if we will append text to 'plaintTextEdit' we will have errors because you
        // cannot append info from another thread.
        // Because of that we will emit signal to main thread to append text
        // Right? I dunno "it just works". :p
        emit signalTypeOnScreen(QString::fromStdWString(text));
    }
    else
    {
        ui->plainTextEdit->appendPlainText(QString::fromStdWString(text));
    }
}

void MainWindow::printUserMessage(std::string timeInfo, std::wstring message, bool bEmitSignal)
{
    if (bEmitSignal)
    {
        emit signalTypeOnScreen( QString(QString::fromStdString(timeInfo) + QString::fromStdWString(message)) );
    }
    else
    {
        ui->plainTextEdit->appendPlainText( QString(QString::fromStdString(timeInfo) + QString::fromStdWString(message)) );
    }
}

void MainWindow::enableInteractiveElements(bool bMenu, bool bTypeAndSend)
{
    emit signalEnableInteractiveElements(bMenu,bTypeAndSend);
}

void MainWindow::setOnlineUsersCount(int onlineCount)
{
    ui->label_3->setText( "Main lobby: " + QString::number(onlineCount) );
}

void MainWindow::addNewUserToList(std::string name)
{
    ui->listWidget_2->addItem( QString::fromStdString(name) );
}

void MainWindow::deleteUserFromList(std::string name, bool bDeleteAll)
{
    if (bDeleteAll)
    {
        ui->listWidget_2->clear();
    }
    else
    {
        for (int i = 0; i < ui->listWidget_2->model()->rowCount(); i++)
        {
            if (ui->listWidget_2->item(i)->text() == QString::fromStdString(name))
            {
                delete ui->listWidget_2->item(i);
                break;
            }
        }
    }
}

void MainWindow::showMessageBox(char type, std::string message)
{
    emit signalShowMessage(type, message);
}

void MainWindow::clearTextEdit()
{
    ui->plainTextEdit_2->clear();
}

MainWindow::~MainWindow()
{
    delete pController;
    delete pConnectWindow;
    delete ui;
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(nullptr,"FChat","FChat (client). Version: 1.05 (09.05.2019)");
}

void MainWindow::on_actionConnect_triggered()
{
    hide();
    pConnectWindow->show();
}

void MainWindow::on_actionDisconnect_triggered()
{
    pController->disconnect();
}

void MainWindow::on_pushButton_clicked()
{
    if (ui->plainTextEdit_2->toPlainText().size() != 0)
    {
       pController->sendMessage(ui->plainTextEdit_2->toPlainText().toStdWString());
    }
}

void MainWindow::on_plainTextEdit_2_textChanged()
{
    if ( ui->plainTextEdit_2->toPlainText().size() > 0 )
    {
        if (ui->plainTextEdit_2->toPlainText()[0] == "\n")
        {
            ui->plainTextEdit_2->setPlainText("");
        }
        else if (ui->plainTextEdit_2->toPlainText()[ui->plainTextEdit_2->toPlainText().size() - 1] == "\n")
        {
            std::wstring text = ui->plainTextEdit_2->toPlainText().toStdWString();
            text = text.substr(0,text.size()-1);
            pController->sendMessage(text);
        }
    }
}

#include "mainwindow.h"
#include "ui_mainwindow.h"

// Custom
#include "../src/View/ConnectWindow/connectwindow.h"
#include "../src/Controller/controller.h"
#include "../src/View/SettingsWindow/settingswindow.h"
#include "../src/View/SingleUserSettings/singleusersettings.h"

// Qt
#include <QMessageBox>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QTimer>

// C++
#include <fstream>
#include <thread>
#include <shlobj.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    pController    = new Controller(this);
    pConnectWindow = nullptr;

    ui->listWidget_2->setContextMenuPolicy(Qt::CustomContextMenu);

    qRegisterMetaType<std::string>("std::string");

    // Connect window
    pConnectWindow = new connectWindow(nullptr);
    connect(pConnectWindow, &connectWindow::connectTo,      this, &MainWindow::connectTo);
    connect(pConnectWindow, &connectWindow::showMainWindow, this, &MainWindow::show);

    // connect to 'this'
    // we use this to send signals to main thread when working with GI
    connect(this, &MainWindow::signalTypeOnScreen,                 this, &MainWindow::typeSomeOnScreen);
    connect(this, &MainWindow::signalEnableInteractiveElements,    this, &MainWindow::slotEnableInteractiveElements);
    connect(this, &MainWindow::signalShowMessage,                  this, &MainWindow::slotShowMessage);
    connect(this, &MainWindow::signalSetPingToUser,                this, &MainWindow::slotSetPingToUser);

    // Context menu in list
    pMenuContextMenu = new QMenu(this);
    pMenuContextMenu->setStyleSheet("QMenu::item\n{\n	background-color: qlineargradient(spread:pad, x1:0.480769, y1:1, x2:0.476, y2:0, stop:0 rgba(17, 17, 17, 255), stop:1 rgba(19, 19, 19, 255));\n    color: rgb(200, 200, 200);\n}\nQMenu::item:selected\n{\n	background-color: rgb(50, 50, 50);\n	color: white;\n}");
    pActionChangeVolume = new QAction("Change Volume");
    pMenuContextMenu->addAction(pActionChangeVolume);
    connect(pActionChangeVolume, &QAction::triggered, this, &MainWindow::slotChangeUserVolume);


    // We use a timer because here (in the constructor) the main (from the main.cpp) event handler is not launched so it won't
    // show our SettingsWindow with opacity change (see 'checkIfSettingsExists() function). We will wait for some time and then show it.
    pTimer = new QTimer();
    pTimer->setInterval(600);
    connect(pTimer, &QTimer::timeout, this, &MainWindow::checkIfSettingsExist);
    pTimer->start();
}






void MainWindow::slotShowMessage(char type, std::string message)
{
    if (type == 0)
    {
        QMessageBox::warning(nullptr, "Warning", QString::fromStdString(message));
    }
    else if (type == 1)
    {
        QMessageBox::information(nullptr, "Information", QString::fromStdString(message));
    }
}

void MainWindow::slotSetPingToUser(std::string userName, int ping)
{
    for (int i = 0; i < ui->listWidget_2->model()->rowCount(); i++)
    {
        QString userNameWithPing = ui->listWidget_2->item(i)->text();
        QString userNameInList = "";
        for (int j = 0; j < userNameWithPing.size(); j++)
        {
            if (userNameWithPing[j] != " ")
            {
                userNameInList += userNameWithPing[j];
            }
            else break;
        }

        if ( userNameInList == QString::fromStdString(userName) )
        {
            if (ping >= 200)
            {
                userNameInList += (" [" + QString::number(ping) + " ms (!!!)]");
            }
            else
            {
                if (ping == 0)
                {
                    userNameInList += (" [-- ms]");
                }
                else userNameInList += (" [" + QString::number(ping) + " ms]");
            }

            ui->listWidget_2->item(i)->setText( userNameInList );

            break;
        }
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

void MainWindow::printOutput(std::string text, bool bEmitSignal)
{
    if (bEmitSignal)
    {
        // This function (printOutput) was called from another thread (not main thread)
        // so if we will append text to 'plaintTextEdit' crash can occur because you
        // cannot change GDI from another thread (it's something with how Windows and GDI works with threads)
        // Because of that we will emit signal to main thread to append text.
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
        // so if we will append text to 'plaintTextEdit' crash can occur because you
        // cannot change GDI from another thread (it's something with how Windows and GDI works with threads)
        // Because of that we will emit signal to main thread to append text.
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
        emit signalTypeOnScreen( QString::fromStdString(timeInfo) + QString::fromStdWString(message) + "\n" );
    }
    else
    {
        ui->plainTextEdit->appendPlainText( QString::fromStdString(timeInfo) + QString::fromStdWString(message) + "\n" );
    }
}

void MainWindow::enableInteractiveElements(bool bMenu, bool bTypeAndSend)
{
    emit signalEnableInteractiveElements(bMenu, bTypeAndSend);
}

void MainWindow::setOnlineUsersCount(int onlineCount)
{
    ui->label_3->setText( "Connected: " + QString::number(onlineCount) );
}

void MainWindow::setPingToUser(std::string userName, int ping)
{
    emit signalSetPingToUser(userName, ping);
}

void MainWindow::addNewUserToList(std::string name)
{
    name += " [-- ms]";
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
            QString userNameWithPing = ui->listWidget_2->item(i)->text();
            QString userName = "";
            for (int j = 0; j < userNameWithPing.size(); j++)
            {
                if (userNameWithPing[j] != " ")
                {
                    userName += userNameWithPing[j];
                }
                else break;
            }

            if ( userName == QString::fromStdString(name))
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

void MainWindow::saveUserName(std::string userName)
{
    TCHAR my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);

    if (result != S_OK)
    {
        QMessageBox::warning(nullptr, "Warning", "Error. Settings will not be saved.");
    }
    else
    {
        std::wstring adressToSettings = std::wstring(my_documents);
        std::wstring adressToTempSettings = std::wstring(my_documents);
        adressToSettings += L"\\FChatSettings.data";
        adressToTempSettings += L"\\FChatSettings~.data";

        // Check if settings file exists
        std::ifstream settingsFile(adressToSettings, std::ios::binary);
        if (settingsFile.is_open())
        {
            std::ofstream newSettingsFile(adressToTempSettings, std::ios::binary);

            int iKey = 0;
            settingsFile.read(reinterpret_cast<char*>(&iKey), 4);
            newSettingsFile.write(reinterpret_cast<char*>(&iKey), 4);

            unsigned short int iVolume = 0;
            settingsFile.read(reinterpret_cast<char*>(&iVolume), 2);
            newSettingsFile.write(reinterpret_cast<char*>(&iVolume), 2);

            unsigned char userNameSize = userName.size();
            newSettingsFile.write(reinterpret_cast<char*>(&userNameSize), 1);
            newSettingsFile.write(userName.c_str(), userNameSize);

            newSettingsFile.close();
            settingsFile.close();

            _wremove(adressToSettings.c_str());
            _wrename(adressToTempSettings.c_str(), adressToSettings.c_str());
        }
        else
        {
            std::ofstream newSettingFile(adressToSettings, std::ios::binary);

            // T button
            int iPushToTalkKey = 0x54;
            newSettingFile.write(reinterpret_cast<char*>(&iPushToTalkKey), 4);

            // 100%
            unsigned short int iVolume = 65535;
            newSettingFile.write(reinterpret_cast<char*>(&iVolume), 2);

            unsigned char nameSize = userName.size();
            newSettingFile.write(reinterpret_cast<char*>(&nameSize), 1);
            newSettingFile.write(userName.c_str(), nameSize);

            newSettingFile.close();
        }
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "FChat", "FChat Client. Version: " + QString::fromStdString(pController->getClientVersion()) + "."
                                       "\nCopyright (c) 2019 Aleksandr \"Flone\" Tretyakov (github.com/Flone-dnb).");
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

void MainWindow::checkIfSettingsExist()
{
    pTimer->stop();

    TCHAR my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);

    if (result != S_OK)
    {
        QMessageBox::warning(nullptr, "Warning", "Error. Settings will not be saved.");
    }
    else
    {
        std::wstring adressToSettings = std::wstring(my_documents);
        adressToSettings += L"\\FChatSettings.data";

        // Check if settings file exists
        std::ifstream settingsFile(adressToSettings, std::ios::binary);
        if (settingsFile.is_open())
        {
            int iKey = 0;

            settingsFile.read(reinterpret_cast<char*>(&iKey), 4);

            unsigned short int iVolume = 0;
            settingsFile.read(reinterpret_cast<char*>(&iVolume), 2);

            pController->setPushToTalkButtonAndVolume(iKey, iVolume);

            unsigned char userNameSize = 0;
            settingsFile.read(reinterpret_cast<char*>(&userNameSize), 1);
            if (userNameSize != 0)
            {
                char nameBuffer[21];
                memset(nameBuffer, 0, 21);

                settingsFile.read(nameBuffer, userNameSize);

                pConnectWindow->setUserName( std::string(nameBuffer) );
            }

            settingsFile.close();
        }
        else
        {
            // Show SettingsWindow
            SettingsWindow* pSettingsWindow = new SettingsWindow(this);
            connect(pSettingsWindow, &SettingsWindow::signalSetPushToTalkButton, this, &MainWindow::slotSetPushToTalkButton);
            pSettingsWindow->setWindowModality(Qt::ApplicationModal);
            pSettingsWindow->setWindowOpacity(0);
            pSettingsWindow->show();

            qreal opacity = 0.1;

            for (int i = 1; i < 11; i++)
            {
                pSettingsWindow->setWindowOpacity( opacity * i );
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }
}

void MainWindow::slotSetPushToTalkButton(int iKey, unsigned short int iVolume)
{
    pController->setPushToTalkButtonAndVolume(iKey, iVolume);
}

void MainWindow::on_actionSettings_triggered()
{
    // Show SettingsWindow
    SettingsWindow* pSettingsWindow = new SettingsWindow(this);
    connect(pSettingsWindow, &SettingsWindow::signalSetPushToTalkButton, this, &MainWindow::slotSetPushToTalkButton);
    pSettingsWindow->setWindowModality(Qt::ApplicationModal);
    pSettingsWindow->show();
}

void MainWindow::on_listWidget_2_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* pItem = ui->listWidget_2->itemAt(pos);
    if (pItem)
    {
        QString nameWithPing = pItem->text();
        QString nameWithoutPing = "";

        for (int i = 0; i < nameWithPing.size(); i++)
        {
            if (nameWithPing[i] == ' ')
            {
                break;
            }
            else
            {
                nameWithoutPing += nameWithPing[i];
            }
        }

        if ( nameWithoutPing.toStdString() != pController->getUserName() )
        {
            QPoint globalPos = ui->listWidget_2->mapToGlobal(pos);

            pMenuContextMenu->exec(globalPos);
        }
    }
}

void MainWindow::slotChangeUserVolume()
{
    if (ui->listWidget_2->currentRow() >= 0)
    {
        QString nameWithPing = ui->listWidget_2->item( ui->listWidget_2->currentRow() )->text();
        QString nameWithoutPing = "";

        for (int i = 0; i < nameWithPing.size(); i++)
        {
            if (nameWithPing[i] == ' ')
            {
                break;
            }
            else
            {
                nameWithoutPing += nameWithPing[i];
            }
        }

        SingleUserSettings* pUserSettings = new SingleUserSettings(nameWithoutPing, pController->getUserCurrentVolume(nameWithoutPing.toStdString()), this);
        connect(pUserSettings, &SingleUserSettings::signalChangeUserVolume, this, &MainWindow::slotSetNewUserVolume);
        pUserSettings->setWindowModality(Qt::WindowModality::WindowModal);
        pUserSettings->show();
    }
}

void MainWindow::slotSetNewUserVolume(QString userName, float fVolume)
{
    pController->setNewUserVolume(userName.toStdString(), fVolume);
}







void MainWindow::closeEvent(QCloseEvent *event)
{
    pController->stop();
}


MainWindow::~MainWindow()
{
    delete pActionChangeVolume;
    delete pMenuContextMenu;

    delete pTimer;
    delete pController;
    delete pConnectWindow;
    delete ui;
}

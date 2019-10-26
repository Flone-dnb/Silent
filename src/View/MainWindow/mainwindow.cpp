#include "mainwindow.h"
#include "ui_mainwindow.h"

// Custom
#include "../src/View/ConnectWindow/connectwindow.h"
#include "../src/Controller/controller.h"
#include "../src/View/SettingsWindow/settingswindow.h"
#include "../src/View/SingleUserSettings/singleusersettings.h"
#include "../src/Model/PingColor.h"

// Qt
#include <QMessageBox>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QHideEvent>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QSystemTrayIcon>

// STL
#include <fstream>
#include <thread>
#include <shlobj.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Tray icon
    pTrayIcon      = new QSystemTrayIcon(this);
    QIcon icon     = QIcon(":/appMainIcon.png");
    pTrayIcon->setIcon(icon);
    connect(pTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::slotTrayIconActivated);

    outputHTMLmessageStart = "<font color=\"";
    outputHTMLmessageEnd   = "</font>";

    pController    = new Controller(this);
    pConnectWindow = nullptr;

    ui->listWidget_2->setContextMenuPolicy(Qt::CustomContextMenu);

    qRegisterMetaType<SilentMessageColor>("SilentMessageColor");
    qRegisterMetaType<std::string>("std::string");

    // Connect window
    pConnectWindow = new connectWindow(this);
    pConnectWindow ->setWindowModality(Qt::WindowModality::WindowModal);
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
        QMessageBox::warning(this, "Warning", QString::fromStdString(message));
    }
    else if (type == 1)
    {
        QMessageBox::information(this, "Information", QString::fromStdString(message));
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

            if (ping <= PING_NORMAL_BELOW)
            {
                ui->listWidget_2->item(i)->setForeground(QColor(PING_NORMAL_R, PING_NORMAL_G, PING_NORMAL_B));
            }
            else if (ping <= PING_WARNING_BELOW)
            {
                ui->listWidget_2->item(i)->setForeground(QColor(PING_WARNING_R, PING_WARNING_G, PING_WARNING_B));
            }
            else
            {
                ui->listWidget_2->item(i)->setForeground(QColor(PING_BAD_R, PING_BAD_G, PING_BAD_B));
            }

            break;
        }
    }
}

void MainWindow::typeSomeOnScreen(QString text, SilentMessageColor messageColor, bool bUserMessage)
{
    mtxPrintOutput .lock   ();

    if (bUserMessage)
    {
        ui ->plainTextEdit ->appendHtml ( text );
    }
    else
    {
        text.replace("\n", "<br>");
        QString color = QString::fromStdString(messageColor.message);

        ui ->plainTextEdit ->appendHtml ( outputHTMLmessageStart + color + "\">" + text + outputHTMLmessageEnd );
    }


    mtxPrintOutput .unlock ();
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

void MainWindow::slotTrayIconActivated()
{
    pTrayIcon->hide();
    raise();
    activateWindow();
    showNormal();
}

void MainWindow::connectTo(std::string adress, std::string port, std::string userName)
{
    ui->plainTextEdit->clear();
    pController->connectTo(adress,port,userName);
}

void MainWindow::printOutput(std::string text, SilentMessageColor messageColor, bool bEmitSignal)
{
    if (bEmitSignal)
    {
        emit signalTypeOnScreen(QString::fromStdString(text), messageColor);
    }
    else
    {
        mtxPrintOutput .lock   ();

        QString message = QString::fromStdString(text);
        message.replace("\n", "<br>");
        QString color = QString::fromStdString(messageColor.message);

        ui ->plainTextEdit ->appendHtml ( outputHTMLmessageStart + color + "\">" + message + outputHTMLmessageEnd );

        mtxPrintOutput .unlock ();
    }
}

void MainWindow::printOutputW(std::wstring text, SilentMessageColor messageColor, bool bEmitSignal)
{
    if (bEmitSignal)
    {
        emit signalTypeOnScreen(QString::fromStdWString(text), messageColor);
    }
    else
    {
        mtxPrintOutput .lock   ();

        QString message = QString::fromStdWString(text);
        message.replace("\n", "<br>");
        QString color = QString::fromStdString(messageColor.message);

        ui ->plainTextEdit ->appendHtml ( outputHTMLmessageStart + color + "\">" + message + outputHTMLmessageEnd );

        mtxPrintOutput .unlock ();
    }
}

void MainWindow::printUserMessage(std::string timeInfo, std::wstring message, SilentMessageColor messageColor, bool bEmitSignal)
{
    QString qtimeRaw = QString::fromStdString(timeInfo);
    QString qtime = "";
    int inamepos = 0;
    for (int i = 0; i < qtimeRaw.size(); i++)
    {
        if (qtimeRaw[i] == ' ')
        {
            inamepos = i;
            break;
        }
        else
        {
            qtime += qtimeRaw[i];
        }
    }
    QString timeColor = QString::fromStdString(messageColor.time);


    QString qmessage = "";

    for (int i = inamepos; i < timeInfo.size(); i++)
    {
        qmessage += timeInfo[i];
    }

    qmessage += QString::fromStdWString(message);
    qmessage.replace("\n", "<br>");
    QString color = QString::fromStdString(messageColor.message);

    qmessage += "<br>";


    QString out = "<font style=\"color: " + timeColor + "\">" + qtime + outputHTMLmessageEnd;
    out += outputHTMLmessageStart + color + "\">" + qmessage + outputHTMLmessageEnd;

    if (bEmitSignal)
    {
        emit signalTypeOnScreen( out, messageColor, true);
    }
    else
    {
        ui ->plainTextEdit ->appendHtml ( out );
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
        QMessageBox::warning(this, "Warning", "Error. Settings will not be saved.");
    }
    else
    {
        std::wstring adressToSettings = std::wstring(my_documents);
        std::wstring adressToTempSettings = std::wstring(my_documents);
        adressToSettings += L"\\SilentSettings.data";
        adressToTempSettings += L"\\SilentSettings~.data";

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

void MainWindow::on_actionConnect_triggered()
{
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
        QMessageBox::warning(this, "Warning", "Error. Settings will not be saved.");
    }
    else
    {
        std::wstring adressToSettings = std::wstring(my_documents);
        adressToSettings += L"\\SilentSettings.data";

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

void MainWindow::hideEvent(QHideEvent *event)
{
    hide();
    pTrayIcon->show();
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

void MainWindow::on_actionAbout_2_triggered()
{
    QMessageBox::about(this, "Silent", "Silent. Version: " + QString::fromStdString(pController->getClientVersion()) + "."
                                       "\n\nCopyright (c) 2019 Aleksandr \"Flone\" Tretyakov (github.com/Flone-dnb).");
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

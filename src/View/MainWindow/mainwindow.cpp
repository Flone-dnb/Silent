#include "mainwindow.h"
#include "ui_mainwindow.h"

// Qt
#include <QMessageBox>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QHideEvent>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QFile>

// STL
#include <thread>


// Custom
#include "../src/View/ConnectWindow/connectwindow.h"
#include "../src/Controller/controller.h"
#include "../src/View/SettingsWindow/settingswindow.h"
#include "../src/View/SingleUserSettings/singleusersettings.h"
#include "../src/View/AboutWindow/aboutwindow.h"
#include "../src/View/StyleAndInfoPaths.h"
#include "../src/Model/SettingsManager/SettingsFile.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui ->setupUi(this);

    // Set CSS classes
    ui ->label_2         ->setProperty("cssClass", "mainwindowLabel");
    ui ->label_3         ->setProperty("cssClass", "mainwindowLabel");
    ui ->plainTextEdit_2 ->setProperty("cssClass", "userInput");
    ui ->plainTextEdit   ->setProperty("cssClass", "chatOutput");




    // Setup tray icon

    pTrayIcon      = new QSystemTrayIcon(this);

    QIcon icon     = QIcon(RES_ICONS_MAIN_PATH);
    pTrayIcon      ->setIcon(icon);

    connect(pTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::slotTrayIconActivated);




    // Output HTML

    outputHTMLmessageStart = "<font color=\"";
    outputHTMLmessageEnd   = "</font>";




    // Create Controller

    pController    = new Controller(this);




    // Setup context menu for 'connected users' list

    ui->listWidget_users->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->listWidget_users->setViewMode(QListView::ListMode);

    pMenuContextMenu    = new QMenu(this);
    pMenuContextMenu ->setStyleSheet("QMenu\n{\n	background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(26, 26, 26, 100), stop:0.605809 rgba(19, 19, 19, 255), stop:1 rgba(26, 26, 26, 100));\n	color: white;\n}\n\nQMenu::item::selected\n{\n	background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(156, 11, 11, 255), stop:1 rgba(168, 0, 0, 255));\n}\n\nQMenu::separator\n{\n	background-color: rgb(50, 0, 0);\n	height: 2px;\n    margin-left: 10px; \n    margin-right: 5px;\n}");

    pActionChangeVolume = new QAction("Change Volume");

    pMenuContextMenu ->addAction(pActionChangeVolume);

    connect(pActionChangeVolume, &QAction::triggered, this, &MainWindow::slotChangeUserVolume);




    // Register types

    qRegisterMetaType <SilentMessageColor> ("SilentMessageColor");
    qRegisterMetaType <std::string>        ("std::string");




    // Setup Connect window

    pConnectWindow = new connectWindow(this);
    pConnectWindow ->setWindowModality(Qt::WindowModality::WindowModal);

    connect(pConnectWindow, &connectWindow::connectTo,      this, &MainWindow::connectTo);
    connect(pConnectWindow, &connectWindow::showMainWindow, this, &MainWindow::show);




    // This to this connects.
    // We use this to send signals to main thread when working with GI.
    connect(this, &MainWindow::signalTypeOnScreen,                 this, &MainWindow::typeSomeOnScreen);
    connect(this, &MainWindow::signalEnableInteractiveElements,    this, &MainWindow::slotEnableInteractiveElements);
    connect(this, &MainWindow::signalShowMessage,                  this, &MainWindow::slotShowMessage);
    connect(this, &MainWindow::signalSetPingToUser,                this, &MainWindow::slotSetPingToUser);
    connect(this, &MainWindow::signalShowUserDisconnectNotice,     this, &MainWindow::slotShowUserDisconnectNotice);
    connect(this, &MainWindow::signalShowUserConnectNotice,        this, &MainWindow::slotShowUserConnectNotice);
    connect(this, &MainWindow::signalSetUserIsTalking,             this, &MainWindow::slotSetUserIsTalking);
    connect(this, &MainWindow::signalApplyTheme,                   this, &MainWindow::slotApplyTheme);




    slotApplyTheme();



    if ( pController ->isSettingsCreatedFirstTime() )
    {
        // Show SettingsWindow to the user

        // We use a timer because here (in the constructor) the main (from the main.cpp) event handler is not launched so it won't
        // show our SettingsWindow with opacity change (see 'checkIfSettingsExists() function). We will wait for some time and then show it.
        pTimer = new QTimer();
        pTimer->setInterval(650);
        connect(pTimer, &QTimer::timeout, this, &MainWindow::showSettingsWindow);
        pTimer->start();
    }
    else
    {
        pTimer = nullptr;
    }
}






void MainWindow::slotShowMessage(bool bWarningBox, std::string message)
{
    if (bWarningBox)
    {
        QMessageBox::warning(this, "Warning", QString::fromStdString(message));
    }
    else
    {
        QMessageBox::information(this, "Information", QString::fromStdString(message));
    }
}

void MainWindow::slotSetPingToUser(std::string userName, int ping)
{
    for (int i = 0; i < ui->listWidget_users->model()->rowCount(); i++)
    {
        QString userNameWithPing = ui->listWidget_users->item(i)->text();
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
            if (ping >= pController->getPingWarningBelow())
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

            ui->listWidget_users->item(i)->setText( userNameInList );


            for (size_t k = 0; k < users.size(); k++)
            {
                if (users[k].sUserName == userName)
                {
                    users[k].ping = ping;


                    if (users[k].bTalking)
                    {
                        if      (users[k].ping <= pController->getPingNormalBelow())
                        {
                            users[k].pItem->setIcon(QIcon(RES_ICONS_USERPING_NORMAL_TALK));
                        }
                        else if (users[k].ping <= pController->getPingWarningBelow())
                        {
                            users[k].pItem->setIcon(QIcon(RES_ICONS_USERPING_WARNING_TALK));
                        }
                        else
                        {
                            users[k].pItem->setIcon(QIcon(RES_ICONS_USERPING_BAD_TALK));
                        }
                    }
                    else
                    {
                        if      (users[k].ping <= pController->getPingNormalBelow())
                        {
                            users[k].pItem->setIcon(QIcon(RES_ICONS_USERPING_NORMAL));
                        }
                        else if (users[k].ping <= pController->getPingWarningBelow())
                        {
                            users[k].pItem->setIcon(QIcon(RES_ICONS_USERPING_WARNING));
                        }
                        else
                        {
                            users[k].pItem->setIcon(QIcon(RES_ICONS_USERPING_BAD));
                        }
                    }

                    break;
                }
            }

            break;
        }
    }
}

void MainWindow::slotSetUserIsTalking(std::string userName, bool bTalking)
{
    for (size_t i = 0; i < users.size(); i++)
    {
        if (users[i].sUserName == userName)
        {
            users[i].bTalking = bTalking;

            if (bTalking)
            {
                if      (users[i].ping <= pController->getPingNormalBelow())
                {
                    users[i].pItem->setIcon(QIcon(RES_ICONS_USERPING_NORMAL_TALK));
                }
                else if (users[i].ping <= pController->getPingWarningBelow())
                {
                    users[i].pItem->setIcon(QIcon(RES_ICONS_USERPING_WARNING_TALK));
                }
                else
                {
                    users[i].pItem->setIcon(QIcon(RES_ICONS_USERPING_BAD_TALK));
                }
            }
            else
            {
                if      (users[i].ping <= pController->getPingNormalBelow())
                {
                    users[i].pItem->setIcon(QIcon(RES_ICONS_USERPING_NORMAL));
                }
                else if (users[i].ping <= pController->getPingWarningBelow())
                {
                    users[i].pItem->setIcon(QIcon(RES_ICONS_USERPING_WARNING));
                }
                else
                {
                    users[i].pItem->setIcon(QIcon(RES_ICONS_USERPING_BAD));
                }
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
        QString color = QString::fromStdString(messageColor.sMessage);

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

void MainWindow::slotShowUserDisconnectNotice(std::string name, SilentMessageColor messageColor, bool bUserLost)
{
    mtxPrintOutput .lock   ();

    QString message = "";

    if (bUserLost)
    {
        message = "The server has lost connection with " + QString::fromStdString(name) + ".<br>";
    }
    else
    {
        message = QString::fromStdString(name) + " disconnected.<br>";
    }

    QString color = QString::fromStdString(messageColor.sTime);

    ui ->plainTextEdit ->appendHtml ( "<font style=\"color: " + color + "\">" + message + outputHTMLmessageEnd );

    mtxPrintOutput .unlock ();
}

void MainWindow::slotShowUserConnectNotice(std::string name, SilentMessageColor messageColor)
{
    mtxPrintOutput .lock   ();

    QString message = QString::fromStdString(name) + " just connected to the chat.<br>";

    QString color = QString::fromStdString(messageColor.sTime);

    ui ->plainTextEdit ->appendHtml ( "<font style=\"color: " + color + "\">" + message + outputHTMLmessageEnd );

    mtxPrintOutput .unlock ();
}

void MainWindow::slotApplyTheme()
{
    if ( pController ->getCurrentSettingsFile() )
    {
        // Apply style

        QFile File(QString(STYLE_THEMES_PATH_FROM_EXE)
                   + QString::fromStdString(pController ->getCurrentSettingsFile() ->sThemeName)
                   + ".css");

        if( File .exists()
            &&
            File .open(QFile::ReadOnly) )
        {
            QString StyleSheet = QLatin1String( File .readAll() );

            qApp->setStyleSheet(StyleSheet);

            File .close();
        }
        else
        {
            if ( pController ->getCurrentSettingsFile() ->sThemeName == STYLE_THEME_DEFAULT_NAME )
            {
                QMessageBox::warning(this, "Error", "Could not open theme file \"" + QString::fromStdString(pController ->getCurrentSettingsFile() ->sThemeName) + ".css\".");
            }
            else
            {
                // Set the default theme

                File .setFileName(QString(STYLE_THEMES_PATH_FROM_EXE)
                                  + QString(STYLE_THEME_DEFAULT_NAME)
                                  + ".css");

                if( File .exists()
                    &&
                    File .open(QFile::ReadOnly) )
                {
                    QString StyleSheet = QLatin1String( File .readAll() );

                    qApp->setStyleSheet(StyleSheet);

                    File .close();


                    SettingsFile* pNewSettings = new SettingsFile();
                    pNewSettings ->iPushToTalkButton = pController ->getCurrentSettingsFile() ->iPushToTalkButton;
                    pNewSettings ->iMasterVolume     = pController ->getCurrentSettingsFile() ->iMasterVolume;
                    pNewSettings ->sUsername         = pController ->getCurrentSettingsFile() ->sUsername;
                    pNewSettings ->sThemeName        = STYLE_THEME_DEFAULT_NAME;

                    pController ->saveSettings( pNewSettings );
                }
                else
                {
                    QMessageBox::warning(this, "Error", "Could not open theme file \"" + QString(STYLE_THEME_DEFAULT_NAME) + ".css\".");
                }
            }
        }
    }
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
        QString color = QString::fromStdString(messageColor.sMessage);

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
        QString color = QString::fromStdString(messageColor.sMessage);

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
    QString timeColor = QString::fromStdString(messageColor.sTime);


    QString qmessage = "";

    for (int i = inamepos; i < timeInfo.size(); i++)
    {
        qmessage += timeInfo[i];
    }

    qmessage += QString::fromStdWString(message);
    qmessage.replace("\n", "<br>");
    QString color = QString::fromStdString(messageColor.sMessage);

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

void MainWindow::setUserIsTalking(std::string userName, bool bTalking)
{
    emit signalSetUserIsTalking(userName, bTalking);
}

void MainWindow::addNewUserToList(std::string name)
{
    QString qName = QString::fromStdString(name) + " [-- ms]";
    QListWidgetItem* pNewItem = new QListWidgetItem(qName);
    ui->listWidget_users->addItem( pNewItem );

    users.push_back(User(name, 0, pNewItem));
}

void MainWindow::deleteUserFromList(std::string name, bool bDeleteAll)
{
    if (bDeleteAll)
    {
        ui->listWidget_users->clear();

        users.clear();
    }
    else
    {
        for (int i = 0; i < ui->listWidget_users->model()->rowCount(); i++)
        {
            QString userNameWithPing = ui->listWidget_users->item(i)->text();
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
                delete ui->listWidget_users->item(i);

                for (size_t j = 0; j < users.size(); j++)
                {
                    if (users[j].sUserName == userName.toStdString())
                    {
                        users.erase( users.begin() + i );
                        break;
                    }
                }

                break;
            }
        }
    }
}

void MainWindow::showUserDisconnectNotice(std::string name, SilentMessageColor messageColor, bool bUserLost)
{
    emit signalShowUserDisconnectNotice(name, messageColor, bUserLost);
}

void MainWindow::showUserConnectNotice(std::string name, SilentMessageColor messageColor)
{
    emit signalShowUserConnectNotice(name, messageColor);
}

void MainWindow::showMessageBox(bool bWarningBox, std::string message)
{
    emit signalShowMessage(bWarningBox, message);
}

void MainWindow::clearTextEdit()
{
    ui->plainTextEdit_2->clear();
}

void MainWindow::applyTheme()
{
    emit signalApplyTheme();
}

void MainWindow::on_actionConnect_triggered()
{
    pConnectWindow ->setUserName( pController ->getCurrentSettingsFile() ->sUsername );

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

void MainWindow::showSettingsWindow()
{
    pTimer->stop();




    // Show SettingsWindow

    SettingsWindow* pSettingsWindow = new SettingsWindow(pController ->getCurrentSettingsFile(), this);
    connect(pSettingsWindow, &SettingsWindow::signalSaveSettings, this, &MainWindow::slotSaveSettings);
    pSettingsWindow ->setWindowModality(Qt::ApplicationModal);
    pSettingsWindow ->setWindowOpacity(0);
    pSettingsWindow ->show();



    // Show "cool animation"

    qreal opacity = 0.1;

    for (int i = 1; i < 11; i++)
    {
        pSettingsWindow ->setWindowOpacity( opacity * i );

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void MainWindow::slotSaveSettings(SettingsFile* pSettingsFile)
{
    pController->saveSettings(pSettingsFile);
}

void MainWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)

    hide();
    pTrayIcon->show();
}

void MainWindow::on_actionSettings_triggered()
{
    // Show SettingsWindow
    SettingsWindow* pSettingsWindow = new SettingsWindow(pController ->getCurrentSettingsFile(), this);
    connect(pSettingsWindow, &SettingsWindow::signalSaveSettings, this, &MainWindow::slotSaveSettings);
    pSettingsWindow->setWindowModality(Qt::ApplicationModal);
    pSettingsWindow->show();
}

void MainWindow::on_listWidget_users_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* pItem = ui->listWidget_users->itemAt(pos);
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
            QPoint globalPos = ui->listWidget_users->mapToGlobal(pos);

            pMenuContextMenu->exec(globalPos);
        }
    }
}

void MainWindow::slotChangeUserVolume()
{
    if (ui->listWidget_users->currentRow() >= 0)
    {
        QString nameWithPing = ui->listWidget_users->item( ui->listWidget_users->currentRow() )->text();
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
    AboutWindow* pAboutWindow = new AboutWindow ( QString::fromStdString(pController->getClientVersion()), this );
    pAboutWindow ->setWindowModality (Qt::WindowModality::WindowModal);
    pAboutWindow ->show();
}





void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    pController->stop();
}


MainWindow::~MainWindow()
{
    delete pActionChangeVolume;
    delete pMenuContextMenu;

    if (pTimer)
    {
        delete pTimer;
    }
    delete pController;
    delete pConnectWindow;
    delete ui;
}

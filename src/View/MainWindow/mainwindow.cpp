// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

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
#include <QTextBlock>
#include <QScrollBar>
#include <QTextDocument>

// STL
#include <thread>


// Custom
#include "View/ConnectWindow/connectwindow.h"
#include "Controller/controller.h"
#include "View/SettingsWindow/settingswindow.h"
#include "View/SingleUserSettings/singleusersettings.h"
#include "View/AboutWindow/aboutwindow.h"
#include "View/StyleAndInfoPaths.h"
#include "Model/SettingsManager/SettingsFile.h"
#include "View/CustomQPlainTextEdit/customqplaintextedit.h"
#include "View/CustomList/SListItemUser/slistitemuser.h"
#include "View/CustomList/SListItemRoom/slistitemroom.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui ->setupUi(this);

    // Set CSS classes
    ui ->label_chatRoom          ->setProperty("cssClass", "mainwindowLabel");
    ui ->label_connectedCount    ->setProperty("cssClass", "mainwindowLabel");
    ui ->plainTextEdit_input     ->setProperty("cssClass", "userInput");
    ui ->plainTextEdit           ->setProperty("cssClass", "chatOutput");




    // Setup tray icon

    pTrayIcon      = new QSystemTrayIcon(this);

    QIcon icon     = QIcon(RES_ICONS_MAIN_PATH);
    pTrayIcon      ->setIcon(icon);

    connect(pTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::slotTrayIconActivated);




    // Output HTML

    outputHTMLtimeStart    = "<font style=\"color: ";
    outputHTMLmessageStart = "<font color=\"";
    outputHTMLmessageEnd   = "</font>";




    // Create Controller

    pController    = new Controller(this);




    // Setup context menu for 'connected users' list

    ui ->listWidget_users ->setContextMenuPolicy (Qt::CustomContextMenu);
    ui ->listWidget_users ->setViewMode          (QListView::ListMode);

    pMenuContextMenu    = new QMenu(this);
    connect(pMenuContextMenu, &QMenu::aboutToHide, this, &MainWindow::slotOnMenuClose);

    pActionChangeVolume = new QAction("Change Volume");
    pActionEnterRoom    = new QAction("Enter Room");

    pMenuContextMenu ->addAction(pActionChangeVolume);
    pMenuContextMenu ->addAction(pActionEnterRoom);

    connect(pActionChangeVolume, &QAction::triggered, this, &MainWindow::slotChangeUserVolume);
    connect(pActionEnterRoom, &QAction::triggered, this, &MainWindow::slotEnterRoom);




    // Register types

    qRegisterMetaType <SilentMessageColor> ("SilentMessageColor");
    qRegisterMetaType <std::string>        ("std::string");
    qRegisterMetaType <QTextBlock>         ("QTextBlock");
    qRegisterMetaType <QVector<int>>       ("QVector<int>");




    // Setup Connect window

    pConnectWindow = new connectWindow(this);
    pConnectWindow ->setWindowModality(Qt::WindowModality::WindowModal);

    connect(pConnectWindow, &connectWindow::connectTo,      this, &MainWindow::connectTo);
    connect(pConnectWindow, &connectWindow::showMainWindow, this, &MainWindow::show);




    // This to this connects

    connect(this, &MainWindow::signalTypeOnScreen,                 this, &MainWindow::typeSomeOnScreen);
    connect(this, &MainWindow::signalEnableInteractiveElements,    this, &MainWindow::slotEnableInteractiveElements);
    connect(this, &MainWindow::signalShowMessageBox,               this, &MainWindow::slotShowMessageBox);
    connect(this, &MainWindow::signalShowUserDisconnectNotice,     this, &MainWindow::slotShowUserDisconnectNotice);
    connect(this, &MainWindow::signalShowUserConnectNotice,        this, &MainWindow::slotShowUserConnectNotice);
    connect(this, &MainWindow::signalApplyTheme,                   this, &MainWindow::slotApplyTheme);
    connect(this, &MainWindow::signalClearTextEdit,                this, &MainWindow::slotClearTextEdit);
    connect(this, &MainWindow::signalShowOldText,                  this, &MainWindow::slotShowOldText);
    connect(this, &MainWindow::signalSetConnectDisconnectButton,   this, &MainWindow::slotSetConnectDisconnectButton);



    connect(ui ->plainTextEdit_input, &CustomQPlainTextEdit::signalReturnPressed, this, &MainWindow::customqplaintextedit_return_pressed);



    slotApplyTheme();



    if ( pController ->isSettingsCreatedFirstTime() )
    {
        // Show SettingsWindow to the user

        // We use a timer because here (in the constructor) the main (from the main.cpp) event handler is not launched so it won't
        // show our SettingsWindow with opacity change (see 'checkIfSettingsExists() function). We will wait for some time and then show it.
        pTimer = new QTimer();
        pTimer ->setInterval(650);
        connect(pTimer, &QTimer::timeout, this, &MainWindow::showSettingsWindow);
        pTimer ->start();
    }
    else
    {
        pTimer = nullptr;
    }
}






void MainWindow::slotShowMessageBox(bool bWarningBox, std::string message)
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
        ui ->actionConnect       ->setEnabled(true);
    }
    else
    {
        ui ->actionConnect       ->setEnabled(false);
    }

    if (bTypeAndSend)
    {
        ui ->pushButton          ->setEnabled(true);
        ui ->plainTextEdit_input ->setEnabled(true);
    }
    else
    {
        ui ->pushButton          ->setEnabled(false);
        ui ->plainTextEdit_input ->setEnabled(false);
    }
}

void MainWindow::slotSetConnectDisconnectButton(bool bConnect)
{
    if (bConnect)
    {
        ui ->actionConnect ->setText("Connect");
    }
    else
    {
        ui ->actionConnect ->setText("Disconnect");
    }
}

void MainWindow::slotTrayIconActivated()
{
    pTrayIcon ->hide();

    raise           ();
    activateWindow  ();
    showNormal      ();
}

void MainWindow::slotShowUserDisconnectNotice(std::string name, SilentMessageColor messageColor, char cUserLost)
{
    mtxPrintOutput .lock   ();



    QString message = "";

    if (cUserLost == 2)
    {
        message = "The server has kicked the user " + QString::fromStdString(name) + ".<br>";
    }
    else if (cUserLost == 1)
    {
        message = "The server has lost connection with " + QString::fromStdString(name) + ".<br>";
    }
    else if (cUserLost == 0)
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

void MainWindow::slotShowOldText(wchar_t *pText)
{
    mtxPrintOutput .lock();


    std::wstring sText(pText);

    delete[] pText;


    QString sNewText = "";
    sNewText += QString::fromStdWString(sText);

    sNewText += ui ->plainTextEdit ->toPlainText() .right( ui ->plainTextEdit ->toPlainText() .size() - 10 ); // 10: ".........."


    ui ->plainTextEdit ->clear();

    ui ->plainTextEdit ->appendHtml(sNewText);


    mtxPrintOutput .unlock();
}

void MainWindow::slotClearTextEdit()
{
    ui ->plainTextEdit_input ->clear();
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
                    QMessageBox::warning(this, "Error", "Could not open default theme file \"" + QString(STYLE_THEME_DEFAULT_NAME) + ".css\".");
                }
            }
        }
    }
}

void MainWindow::connectTo(std::string adress, std::string port, std::string userName, std::wstring sPass)
{
    ui ->plainTextEdit ->clear();

    pController ->connectTo(adress, port, userName, sPass);
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
    // 'timeInfo' example: "18:58. Flone: "


    // Get position in string where the name begins.

    QString sTime    = "";
    size_t  iNameStartPos = 0;

    for (size_t i = 0;   i < timeInfo .size();   i++)
    {
        if (timeInfo[i] == ' ')
        {
            iNameStartPos = i;

            break;
        }
        else
        {
            sTime += timeInfo[i];
        }
    }




    // Read userName from 'timeInfo'.

    QString sNameWithMessage = "";

    for (size_t i = iNameStartPos;   i < timeInfo .size();   i++)
    {
        sNameWithMessage += timeInfo[i];
    }

    sNameWithMessage += QString::fromStdWString(message);


    // Replace any '\n' to '<br>' because we will use "appendHtml()" function.
    sNameWithMessage .replace("\n", "<br>");


    // Add <br> in the end of the message.
    sNameWithMessage += "<br>";




    // Prepare colors.

    QString sMessageColor = QString::fromStdString(messageColor .sMessage);
    QString sTimeColor    = QString::fromStdString(messageColor .sTime);




    // Create final output string.

    QString sFinalMessage = outputHTMLtimeStart     + sTimeColor    + "\">" + sTime            + outputHTMLmessageEnd;
    sFinalMessage        += outputHTMLmessageStart  + sMessageColor + "\">" + sNameWithMessage + outputHTMLmessageEnd;




    // Print on screen.

    if (bEmitSignal)
    {
        emit signalTypeOnScreen( sFinalMessage, messageColor, true);
    }
    else
    {
        ui ->plainTextEdit ->appendHtml ( sFinalMessage );
    }
}

void MainWindow::enableInteractiveElements(bool bMenu, bool bTypeAndSend)
{
    emit signalEnableInteractiveElements(bMenu, bTypeAndSend);
}

void MainWindow::setOnlineUsersCount(int onlineCount)
{
    ui ->label_connectedCount ->setText( "Connected: " + QString::number(onlineCount) );
}

void MainWindow::setConnectDisconnectButton(bool bConnect)
{
    emit signalSetConnectDisconnectButton(bConnect);
}

void MainWindow::setPingAndTalkingToUser(SListItemUser* pListWidgetItem, int iPing, bool bTalking)
{
    pListWidgetItem->setPing(iPing);
    pListWidgetItem->setUserTalking(bTalking);
}


SListItemUser* MainWindow::addNewUserToList(std::string name)
{
    return ui ->listWidget_users ->addUser(QString::fromStdString(name), nullptr);
}

void MainWindow::addRoom(std::string sRoomName, std::wstring sPassword, size_t iMaxUsers, bool bFirstRoom)
{
    ui->listWidget_users->addRoom(QString::fromStdString(sRoomName), QString::fromStdWString(sPassword), iMaxUsers, bFirstRoom);
}

size_t MainWindow::getRoomCount()
{
    return ui->listWidget_users->getRoomCount();
}

SListItemUser* MainWindow::addUserToRoomIndex(std::string sName, size_t iRoomIndex)
{
    return ui->listWidget_users->addUser(QString::fromStdString(sName), ui->listWidget_users->getRooms()[iRoomIndex]);
}

void MainWindow::moveUserToRoom(SListItemUser *pUser, std::string sRoomName)
{
    ui ->listWidget_users ->moveUser(pUser, QString::fromStdString(sRoomName));
}

void MainWindow::deleteUserFromList(SListItemUser* pListWidgetItem, bool bDeleteAll)
{
    if (bDeleteAll)
    {
        ui->listWidget_users->deleteAll();
    }
    else
    {
        ui->listWidget_users->deleteUser(pListWidgetItem);
    }
}

void MainWindow::showUserDisconnectNotice(std::string name, SilentMessageColor messageColor, char cUserLost)
{
    emit signalShowUserDisconnectNotice(name, messageColor, cUserLost);
}

void MainWindow::showUserConnectNotice(std::string name, SilentMessageColor messageColor)
{
    emit signalShowUserConnectNotice(name, messageColor);
}

void MainWindow::showOldText(wchar_t *pText)
{
    emit signalShowOldText(pText);
}

void MainWindow::showMessageBox(bool bWarningBox, std::string message)
{
    emit signalShowMessageBox(bWarningBox, message);
}

void MainWindow::clearTextEdit()
{
    emit signalClearTextEdit();
}

void MainWindow::applyTheme()
{
    emit signalApplyTheme();
}

void MainWindow::on_actionConnect_triggered()
{
    if ( ui ->actionConnect ->text() == "Connect" )
    {
        pConnectWindow ->setUserName( pController ->getCurrentSettingsFile() ->sUsername );
        pConnectWindow ->setConnectString( pController ->getCurrentSettingsFile() ->sConnectString );
        pConnectWindow ->setPort( std::to_string(pController ->getCurrentSettingsFile() ->iPort) );
        pConnectWindow ->setPassword( pController ->getCurrentSettingsFile() ->sPassword );

        pConnectWindow->show();
    }
    else
    {
        pController->disconnect();
    }
}

void MainWindow::on_pushButton_clicked()
{
    if (ui ->plainTextEdit_input ->toPlainText() .size() != 0)
    {
       pController ->sendMessage( ui ->plainTextEdit_input ->toPlainText() .toStdWString() );
    }
}

void MainWindow::customqplaintextedit_return_pressed()
{
    if ( (ui ->pushButton ->isEnabled()) && (ui ->plainTextEdit_input ->toPlainText() != "") )
    {
        if (ui ->plainTextEdit_input ->toPlainText()[ ui ->plainTextEdit_input ->toPlainText() .size() - 1 ] == "\n")
        {
            std::wstring text = ui ->plainTextEdit_input ->toPlainText() .toStdWString();

            text = text .substr( 0, text .size() - 1 );

            pController ->sendMessage(text);
        }
        else
        {
            pController ->sendMessage( ui ->plainTextEdit_input ->toPlainText() .toStdWString() );
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

void MainWindow::slotOnMenuClose()
{
    ui ->listWidget_users ->clearSelection();
}

void MainWindow::slotEnterRoom()
{
    if (ui->listWidget_users->currentRow() >= 0)
    {
        SListItem* pItem = dynamic_cast<SListItem*>(ui->listWidget_users->currentItem());

        if (pItem->isRoom())
        {
            SListItemRoom* pRoom = dynamic_cast<SListItemRoom*>(pItem);

            if (pRoom == pController->getCurrentUserRoom())
            {
                ui ->listWidget_users ->clearSelection();
            }
            else
            {
                pController->enterRoom(pRoom->getRoomName().toStdString());
            }
        }
    }
}

void MainWindow::on_listWidget_users_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* pItem = ui->listWidget_users->itemAt(pos);

    if (pItem)
    {
        SListItem* pListItem = dynamic_cast<SListItem*>(pItem);

        if (pListItem->isRoom())
        {
            pActionChangeVolume->setVisible(false);
            pActionEnterRoom->setVisible(true);

            SListItemRoom* pRoom = dynamic_cast<SListItemRoom*>(pListItem);

            if (pRoom != pController->getCurrentUserRoom())
            {
                QPoint globalPos = ui->listWidget_users->mapToGlobal(pos);

                pMenuContextMenu->exec(globalPos);
            }
            else
            {
                ui->listWidget_users->clearSelection();
            }
        }
        else
        {
            pActionChangeVolume->setVisible(true);
            pActionEnterRoom->setVisible(false);

            SListItemUser* pUser = dynamic_cast<SListItemUser*>(pListItem);

            if (pUser->getName().toStdString() != pController->getUserName())
            {
                QPoint globalPos = ui->listWidget_users->mapToGlobal(pos);

                pMenuContextMenu->exec(globalPos);
            }
            else
            {
                ui->listWidget_users->clearSelection();
            }
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

        SingleUserSettings* pUserSettings = new SingleUserSettings(nameWithoutPing, pController ->getUserCurrentVolume(nameWithoutPing.toStdString()), this);
        connect(pUserSettings, &SingleUserSettings::signalChangeUserVolume, this, &MainWindow::slotSetNewUserVolume);
        pUserSettings->setWindowModality(Qt::WindowModality::WindowModal);
        pUserSettings->show();
    }
}

void MainWindow::slotSetNewUserVolume(QString userName, float fVolume)
{
    pController->setNewUserVolume(userName.toStdString(), fVolume);
}

void MainWindow::slotDeleteUserFromList(QListWidgetItem* pListWidgetItem, bool bDeleteAll)
{
    mtxList .lock();

    if (bDeleteAll)
    {
        ui ->listWidget_users ->clear();
    }
    else
    {
        delete pListWidgetItem;
    }

    mtxList .unlock();
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

void MainWindow::on_listWidget_users_itemDoubleClicked(QListWidgetItem *item)
{
    if (ui ->listWidget_users ->currentRow() >= 0)
    {
        SListItem* pItem = dynamic_cast<SListItem*>(item);

        if (pItem->isRoom())
        {
            SListItemRoom* pRoom = dynamic_cast<SListItemRoom*>(pItem);

            if (pRoom == pController->getCurrentUserRoom())
            {
                ui ->listWidget_users ->clearSelection();
            }
            else
            {
                pController->enterRoom(pRoom->getRoomName().toStdString());
            }
        }
    }
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

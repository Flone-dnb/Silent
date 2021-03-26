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
#include "Model/SettingsManager/settingsmanager.h"
#include "View/CustomQPlainTextEdit/customqplaintextedit.h"
#include "View/CustomList/SListItemUser/slistitemuser.h"
#include "View/CustomList/SListItemRoom/slistitemroom.h"
#include "View/RoomPassInputWindow/roompassinputwindow.h"
#include "View/WindowControlWidget/windowcontrolwidget.h"
#include "View/GlobalMessageWindow/globalmessagewindow.h"
#include "View/AboutQtWindow/aboutqtwindow.h"

// Other
#include "Windows.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    bAbleToSend = false;
    bMuteMicButtonRegistered = false;

    // Set CSS classes
    ui->label_chatRoom         ->setProperty("cssClass", "mainwindowLabel");
    ui->label_connectedCount   ->setProperty("cssClass", "mainwindowLabel");
    ui->plainTextEdit_input    ->setProperty("cssClass", "userInput");
    ui->plainTextEdit          ->setProperty("cssClass", "chatOutput");

    WindowControlWidget* pControlWindowWidget = new WindowControlWidget(this);
    connect(pControlWindowWidget, &WindowControlWidget::signalClose,    this, &MainWindow::close);
    connect(pControlWindowWidget, &WindowControlWidget::signalHide,     this, &MainWindow::slotHideWindow);
    connect(pControlWindowWidget, &WindowControlWidget::signalMaximize, this, &MainWindow::slotMaxWindow);

    connect(this, &MainWindow::signalAddRoom,            this, &MainWindow::slotAddRoom);
    connect(this, &MainWindow::signalDeleteRoom,         this, &MainWindow::slotDeleteRoom);
    connect(this, &MainWindow::signalMoveUserToRoom,     this, &MainWindow::slotMoveUserToRoom);
    connect(this, &MainWindow::signalDeleteUserFromList, this, &MainWindow::slotDeleteUserFromList);
    connect(this, &MainWindow::signalMoveRoom,           this, &MainWindow::slotMoveRoom);
    connect(this, &MainWindow::signalAddUserToRoomIndex, this, &MainWindow::slotAddUserToRoomIndex);
    connect(this, &MainWindow::signalAddNewUserToList,   this, &MainWindow::slotAddNewUserToList);

    ui->menuBar->setCornerWidget(pControlWindowWidget, Qt::Corner::TopRightCorner);
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

void MainWindow::slotShowPasswordInputWindow(std::string sRoomName)
{
    RoomPassInputWindow* pWindow = new RoomPassInputWindow(QString::fromStdString(sRoomName), this);

    pWindow->setWindowModality(Qt::WindowModality::WindowModal);

    connect(pWindow, &RoomPassInputWindow::signalEnterRoomWithPassword, this, &MainWindow::slotEnterRoomWithPassword);

    pWindow->show();
}

void MainWindow::slotShowServerMessage(QString sMessage)
{
    GlobalMessageWindow* pWindow = new GlobalMessageWindow(sMessage, this);

    pWindow->show();
}

void MainWindow::slotEnterRoomWithPassword(QString sRoomName, QString sPassword)
{
    pController->enterRoomWithPassword(sRoomName.toStdString(), sPassword.toStdWString());
}

void MainWindow::slotApplyAudioInputVolume(int iVolume)
{
    pController->applyAudioInputVolume(iVolume);
}

void MainWindow::slotApplyVoiceStartValue(int iValue)
{
    pController->applyVoiceStartValue(iValue);
}

void MainWindow::slotApplyShouldHearTestVoice(bool bHear)
{
    pController->applyShouldHearTestVoice(bHear);
}



void MainWindow::typeSomeOnScreen(QString sText, SilentMessage messageColor, bool bUserMessage)
{
    mtxPrintOutput.lock();

    if (bUserMessage)
    {
        ui->plainTextEdit->appendHtml (sText);
    }
    else
    {
        sText.replace("\n", "<br>");
        QString color = QString::fromStdString(messageColor.sMessage);

        ui->plainTextEdit->appendHtml (outputHTMLmessageStart + color + "\">" + sText + outputHTMLmessageEnd);
    }


    mtxPrintOutput.unlock();
}

void MainWindow::slotEnableInteractiveElements(bool bMenu, bool bTypeAndSend)
{
    if (bMenu)
    {
        ui->actionConnect      ->setEnabled(true);
    }
    else
    {
        ui->actionConnect      ->setEnabled(false);
    }

    if (bTypeAndSend)
    {
        bAbleToSend = true;
        ui->plainTextEdit_input->setEnabled(true);
    }
    else
    {
        bAbleToSend = false;
        ui->plainTextEdit_input->setEnabled(false);
    }
}

void MainWindow::slotSetConnectDisconnectButton(bool bConnect)
{
    if (bConnect)
    {
        ui->actionConnect->setText("Connect");
    }
    else
    {
        ui->actionConnect->setText("Disconnect");
    }
}

void MainWindow::slotCreateRoom(QString sName, QString sPassword, size_t iMaxUsers, std::promise<bool>* resultPromise)
{
    ui->listWidget_users->addRoom(sName, sPassword, iMaxUsers);

    resultPromise->set_value(false);
}

void MainWindow::slotTrayIconActivated()
{
    pTrayIcon->hide();

    raise           ();
    activateWindow  ();
    showNormal      ();
}

void MainWindow::slotShowUserDisconnectNotice(std::string sName, SilentMessage messageColor, char cUserLost)
{
    mtxPrintOutput.lock   ();



    QString message = "";

    if (cUserLost == 2)
    {
        message = "The server has kicked the user " + QString::fromStdString(sName) + ".<br>";
    }
    else if (cUserLost == 1)
    {
        message = "The server has lost connection with " + QString::fromStdString(sName) + ".<br>";
    }
    else if (cUserLost == 0)
    {
        message = QString::fromStdString(sName) + " disconnected.<br>";
    }

    QString color = QString::fromStdString(messageColor.sTime);

    ui->plainTextEdit->appendHtml ( "<font style=\"color: " + color + "\">" + message + outputHTMLmessageEnd );



    mtxPrintOutput.unlock ();
}

void MainWindow::slotShowUserConnectNotice(std::string sName, SilentMessage messageColor)
{
    mtxPrintOutput.lock   ();

    QString message = QString::fromStdString(sName) + " just connected to the chat.<br>";

    QString color = QString::fromStdString(messageColor.sTime);

    ui->plainTextEdit->appendHtml ( "<font style=\"color: " + color + "\">" + message + outputHTMLmessageEnd );

    mtxPrintOutput.unlock ();
}

void MainWindow::slotShowOldText(wchar_t *pText)
{
    mtxPrintOutput.lock();


    std::wstring sText(pText);

    delete[] pText;


    QString sNewText = "";
    sNewText += QString::fromStdWString(sText);

    sNewText += ui->plainTextEdit->toPlainText().right( ui->plainTextEdit->toPlainText().size() - 10 ); // 10: ".........."


    ui->plainTextEdit->clear();

    ui->plainTextEdit->appendHtml(sNewText);


    mtxPrintOutput.unlock();
}

void MainWindow::slotClearTextEdit()
{
    ui->plainTextEdit_input->clear();
}

void MainWindow::slotClearTextChatOutput()
{
    ui->plainTextEdit->clear();
}

void MainWindow::slotApplyTheme()
{
    if ( pController->getCurrentSettingsFile() )
    {
        // Apply style

        QFile File(QString(STYLE_THEMES_PATH_FROM_EXE)
                   + QString::fromStdString(pController->getCurrentSettingsFile()->sThemeName)
                   + ".css");

        if( File.exists()
            &&
            File.open(QFile::ReadOnly) )
        {
            QString StyleSheet = QLatin1String( File.readAll() );

            qApp->setStyleSheet(StyleSheet);

            File.close();
        }
        else
        {
            if ( pController->getCurrentSettingsFile()->sThemeName == STYLE_THEME_DEFAULT_NAME )
            {
                QMessageBox::warning(this, "Error", "Could not open theme file \"" + QString::fromStdString(pController->getCurrentSettingsFile()->sThemeName) + ".css\".");
            }
            else
            {
                // Set the default theme

                File.setFileName(QString(STYLE_THEMES_PATH_FROM_EXE)
                                  + QString(STYLE_THEME_DEFAULT_NAME)
                                  + ".css");

                if( File.exists()
                    &&
                    File.open(QFile::ReadOnly) )
                {
                    QString StyleSheet = QLatin1String( File.readAll() );

                    qApp->setStyleSheet(StyleSheet);

                    File.close();


                    SettingsFile* pSettings = pController->getSettingsManager()->getCurrentSettings();
                    pSettings->sThemeName        = STYLE_THEME_DEFAULT_NAME;

                    pController->getSettingsManager()->saveCurrentSettings();
                }
                else
                {
                    QMessageBox::warning(this, "Error", "Could not open default theme file \"" + QString(STYLE_THEME_DEFAULT_NAME) + ".css\".");
                }
            }
        }
    }
}

void MainWindow::slotApplyMasterVolume()
{
    pController->pauseTestRecording();

    pController->applyNewMasterVolumeFromSettings();
}

void MainWindow::slotSettingsWindowClosed()
{
    pController->pauseTestRecording();
}

void MainWindow::slotAddRoom(QString sRoomName, QString sPassword, size_t iMaxUsers, bool bFirstRoom, std::promise<int>* promiseRoomCount)
{
    ui->listWidget_users->addRoom(sRoomName, sPassword, iMaxUsers, bFirstRoom);

    promiseRoomCount->set_value(static_cast<int>(ui->listWidget_users->getRooms().size()));
}

void MainWindow::slotDeleteRoom(QString sRoomName, std::promise<int>* promiseRoomCount)
{
    std::vector<SListItemRoom*> vRooms = ui->listWidget_users->getRooms();

    for (size_t i = 0; i < vRooms.size(); i++)
    {
        if (vRooms[i]->getRoomName() == sRoomName)
        {
            ui->listWidget_users->deleteRoom(vRooms[i]);

            break;
        }
    }

    promiseRoomCount->set_value(static_cast<int>(vRooms.size()));
}

void MainWindow::slotMoveUserToRoom(SListItemUser *pUser, QString sRoomName, std::promise<bool>* promiseResult)
{
    ui->listWidget_users->moveUser(pUser, sRoomName);

    promiseResult->set_value(false);

    ui->plainTextEdit->clear();
}

void MainWindow::slotMoveRoom(QString sRoomName, bool bMoveUp, std::promise<bool> *promiseResult)
{
    std::vector<SListItemRoom*> vRooms = ui->listWidget_users->getRooms();

    for (size_t i = 0; i < vRooms.size(); i++)
    {
        if (vRooms[i]->getRoomName() == sRoomName)
        {
            if (bMoveUp)
            {
                ui->listWidget_users->moveRoomUp(vRooms[i]);
            }
            else
            {
                ui->listWidget_users->moveRoomDown(vRooms[i]);
            }

            break;
        }
    }

    promiseResult->set_value(false);
}

void MainWindow::slotAddUserToRoomIndex(QString sName, size_t iRoomIndex, std::promise<SListItemUser*> *promiseResult)
{
    if (iRoomIndex == 0)
    {
        ui->label_chatRoom->setText(ui->listWidget_users->getRoomNames()[0]);
    }

    SListItemUser* pUser = ui->listWidget_users->addUser(sName, ui->listWidget_users->getRooms()[iRoomIndex]);

    promiseResult->set_value(pUser);
}

void MainWindow::slotAddNewUserToList(QString sName, std::promise<SListItemUser *> *promiseResult)
{
    SListItemUser* pUser = ui->listWidget_users->addUser(sName, nullptr);

    promiseResult->set_value(pUser);
}

void MainWindow::slotRegisterMuteMicButton(int iButton)
{
    if (bMuteMicButtonRegistered)
    {
        UnregisterHotKey((HWND)MainWindow::winId(), 100);

        bMuteMicButtonRegistered = false;
    }

    if (iButton != 0)
    {
        RegisterHotKey((HWND)MainWindow::winId(), 100, MOD_NOREPEAT, static_cast<unsigned int>(iButton));

        bMuteMicButtonRegistered = true;
    }
}

void MainWindow::slotMaxWindow()
{
    if (windowState() == Qt::WindowState::WindowMaximized)
    {
        setWindowState(Qt::WindowState::WindowNoState);
    }
    else
    {
        setWindowState(Qt::WindowState::WindowMaximized);
    }
}

void MainWindow::slotHideWindow()
{
    setWindowState(Qt::WindowState::WindowMinimized);
}

void MainWindow::connectTo(std::string sAdress, std::string sPort, std::string sUserName, std::wstring sPass)
{
    ui->plainTextEdit->clear();

    pController->connectTo(sAdress, sPort, sUserName, sPass);
}

void MainWindow::printOutput(const std::string& sText, SilentMessage messageColor, bool bEmitSignal)
{
    if (bEmitSignal)
    {
        emit signalTypeOnScreen(QString::fromStdString(sText), messageColor);
    }
    else
    {
        mtxPrintOutput.lock   ();

        QString message = QString::fromStdString(sText);
        message.replace("\n", "<br>");
        QString color = QString::fromStdString(messageColor.sMessage);

        ui->plainTextEdit->appendHtml ( outputHTMLmessageStart + color + "\">" + message + outputHTMLmessageEnd );

        mtxPrintOutput.unlock ();
    }
}

void MainWindow::printOutputW(const std::wstring& sText, SilentMessage messageColor, bool bEmitSignal)
{
    if (bEmitSignal)
    {
        emit signalTypeOnScreen(QString::fromStdWString(sText), messageColor);
    }
    else
    {
        mtxPrintOutput.lock   ();

        QString message = QString::fromStdWString(sText);
        message.replace("\n", "<br>");
        QString color = QString::fromStdString(messageColor.sMessage);

        ui->plainTextEdit->appendHtml ( outputHTMLmessageStart + color + "\">" + message + outputHTMLmessageEnd );

        mtxPrintOutput.unlock ();
    }
}

void MainWindow::printUserMessage(const std::string& sTimeInfo, const std::wstring& sMessage, SilentMessage messageColor, bool bEmitSignal)
{
    // 'timeInfo' example: "18:58. Flone: "


    // Get position in string where the name begins.

    QString sTime    = "";
    size_t  iNameStartPos = 0;

    for (size_t i = 0;   i < sTimeInfo.size();   i++)
    {
        if (sTimeInfo[i] == ' ')
        {
            iNameStartPos = i;

            break;
        }
        else
        {
            sTime += sTimeInfo[i];
        }
    }




    // Read userName from 'timeInfo'.

    QString sNameWithMessage = "";

    for (size_t i = iNameStartPos;   i < sTimeInfo.size();   i++)
    {
        sNameWithMessage += sTimeInfo[i];
    }

    sNameWithMessage += QString::fromStdWString(sMessage);


    // Replace any '\n' to '<br>' because we will use "appendHtml()" function.
    sNameWithMessage.replace("\n", "<br>");
    // Replace any ' ' to '&nbsp;'
    sNameWithMessage.replace(" ", "&nbsp;");


    // Add <br> in the end of the message.
    sNameWithMessage += "<br>";




    // Prepare colors.

    QString sMessageColor = QString::fromStdString(messageColor.sMessage);
    QString sTimeColor    = QString::fromStdString(messageColor.sTime);




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
        ui->plainTextEdit->appendHtml ( sFinalMessage );
    }
}

void MainWindow::enableInteractiveElements(bool bMenu, bool bTypeAndSend)
{
    emit signalEnableInteractiveElements(bMenu, bTypeAndSend);
}

void MainWindow::setOnlineUsersCount(int iOnlineCount)
{
    ui->label_connectedCount->setText( "Connected: " + QString::number(iOnlineCount) );
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


SListItemUser* MainWindow::addNewUserToList(const std::string& sName)
{
    mtxList.lock();

    std::promise<SListItemUser*> resultPromise;
    std::future<SListItemUser*> resultFuture = resultPromise.get_future();

    emit signalAddNewUserToList(QString::fromStdString(sName), &resultPromise);

    SListItemUser* pUser = resultFuture.get();

    mtxList.unlock();

    return pUser;
}

void MainWindow::addRoom(std::string sRoomName, std::wstring sPassword, size_t iMaxUsers, bool bFirstRoom)
{
    mtxList.lock();

    std::promise<int> roomCountPromise;
    std::future<int> roomCountFuture = roomCountPromise.get_future();

    emit signalAddRoom(QString::fromStdString(sRoomName), QString::fromStdWString(sPassword), iMaxUsers, bFirstRoom, &roomCountPromise);

    roomCountFuture.get();

    mtxList.unlock();
}

size_t MainWindow::getRoomCount()
{
    return ui->listWidget_users->getRoomCount();
}

SListItemUser* MainWindow::addUserToRoomIndex(const std::string& sName, size_t iRoomIndex)
{
    mtxList.lock();

    std::promise<SListItemUser*> resultPromise;
    std::future<SListItemUser*> resultFuture = resultPromise.get_future();

    emit signalAddUserToRoomIndex(QString::fromStdString(sName), iRoomIndex, &resultPromise);

    SListItemUser* pUser = resultFuture.get();

    mtxList.unlock();

    return pUser;
}

void MainWindow::moveUserToRoom(SListItemUser *pUser, std::string sRoomName)
{
    mtxList.lock();


    std::promise<bool> resultPromise;
    std::future<bool> resultFuture = resultPromise.get_future();

    emit signalMoveUserToRoom(pUser, QString::fromStdString(sRoomName), &resultPromise);

    resultFuture.get();



    mtxList.unlock();
}

void MainWindow::moveRoom(const std::string& sRoomName, bool bMoveUp)
{
    mtxList.lock();

    std::promise<bool> resultPromise;
    std::future<bool> resultFuture = resultPromise.get_future();

    emit signalMoveRoom(QString::fromStdString(sRoomName), bMoveUp, &resultPromise);

    resultFuture.get();

    mtxList.unlock();
}

void MainWindow::deleteRoom(const std::string& sRoomName)
{
    mtxList.lock();

    std::promise<int> roomCountPromise;
    std::future<int> roomCountFuture = roomCountPromise.get_future();

    emit signalDeleteRoom(QString::fromStdString(sRoomName), &roomCountPromise);

    roomCountFuture.get();

    mtxList.unlock();
}

void MainWindow::createRoom(const std::string& sName, const std::u16string& sPassword, size_t iMaxUsers)
{
    mtxList.lock();

    std::promise<bool> resultPromise;
    std::future<bool> resultFuture = resultPromise.get_future();

    emit signalCreateRoom(QString::fromStdString(sName), QString::fromStdU16String(sPassword), iMaxUsers, &resultPromise);

    resultFuture.get();

    mtxList.unlock();
}

void MainWindow::changeRoomSettings(const std::string& sOldName, const std::string& sNewName, size_t iMaxUsers)
{
    mtxList.lock();

    std::vector<SListItemRoom*> vRooms = ui->listWidget_users->getRooms();

    QString sOldRoomName = QString::fromStdString(sOldName);
    QString sNewRoomName = QString::fromStdString(sNewName);


    for (size_t i = 0; i < vRooms.size(); i++)
    {
        if (vRooms[i]->getRoomName() == sOldRoomName)
        {
            if (sOldRoomName != sNewRoomName)
            {
                ui->listWidget_users->renameRoom(vRooms[i], sNewRoomName);
            }

            vRooms[i]->setRoomMaxUsers(iMaxUsers);

            break;
        }
    }

    mtxList.unlock();
}

void MainWindow::deleteUserFromList(SListItemUser* pListWidgetItem, bool bDeleteAll)
{
    mtxList.lock();

    std::promise<bool> resultPromise;
    std::future<bool> resultFuture = resultPromise.get_future();

    emit signalDeleteUserFromList(pListWidgetItem, bDeleteAll, &resultPromise);

    resultFuture.get();

    mtxList.unlock();
}

void MainWindow::showUserDisconnectNotice(std::string sName, SilentMessage messageColor, char cUserLost)
{
    emit signalShowUserDisconnectNotice(sName, messageColor, cUserLost);
}

void MainWindow::showUserConnectNotice(std::string sName, SilentMessage messageColor)
{
    emit signalShowUserConnectNotice(sName, messageColor);
}

void MainWindow::showOldText(wchar_t *pText)
{
    emit signalShowOldText(pText);
}

void MainWindow::showMessageBox(bool bWarningBox, std::string sMessage)
{
    emit signalShowMessageBox(bWarningBox, sMessage);
}

void MainWindow::showPasswordInputWindow(std::string sRoomName)
{
    emit signalShowPasswordInputWindow(sRoomName);
}

void MainWindow::showServerMessage(std::string sMessage)
{
    emit signalShowServerMessage(QString::fromStdString(sMessage));
}

void MainWindow::clearTextEdit()
{
    emit signalClearTextEdit();
}

void MainWindow::showVoiceVolumeValueInSettings(int iVolume)
{
    emit signalShowVoiceVolumeValueInSettings(iVolume);
}

void MainWindow::applyTheme()
{
    emit signalApplyTheme();
}

void MainWindow::on_actionConnect_triggered()
{
    if ( ui->actionConnect->text() == "Connect" )
    {
        pConnectWindow->setUserName( pController->getCurrentSettingsFile()->sUsername );
        pConnectWindow->setConnectString( pController->getCurrentSettingsFile()->sConnectString );
        pConnectWindow->setPort( std::to_string(pController->getCurrentSettingsFile()->iPort) );
        pConnectWindow->setPassword( pController->getCurrentSettingsFile()->sPassword );

        pConnectWindow->show();
    }
    else
    {
        pController->disconnect();
    }
}

void MainWindow::customqplaintextedit_return_pressed()
{
    if ( bAbleToSend && (ui->plainTextEdit_input->toPlainText() != "") )
    {
        std::wstring sMessage = ui->plainTextEdit_input->toPlainText().toStdWString();
        if (filterMessageText(sMessage))
        {
            showMessageBox(true, "Your message is too big!");
            return;
        }

        if (sMessage != L"")
        {
            pController->sendMessage(sMessage);
        }
        else
        {
            clearTextEdit();
        }
    }
}


void MainWindow::showSettingsWindow()
{
    pTimer->stop();



    std::vector<std::wstring> vInputDevices = pController->getInputDevices();
    std::vector<QString> vAudioInputDevices;

    for (size_t i = 0; i < vInputDevices.size(); i++)
    {
        vAudioInputDevices.push_back(QString::fromStdWString(vInputDevices[i]));
    }

    // Show SettingsWindow

    SettingsWindow* pSettingsWindow = new SettingsWindow(pController->getSettingsManager(), vAudioInputDevices, this);
    connect(pSettingsWindow, &SettingsWindow::applyNewMasterVolume, this, &MainWindow::slotApplyMasterVolume);
    connect(this, &MainWindow::signalShowVoiceVolumeValueInSettings, pSettingsWindow, &SettingsWindow::slotSetVoiceVolume);
    connect(pSettingsWindow, &SettingsWindow::closedSettingsWindow, this, &MainWindow::slotSettingsWindowClosed);
    connect(pSettingsWindow, &SettingsWindow::signalSetAudioInputVolume, this, &MainWindow::slotApplyAudioInputVolume);
    connect(pSettingsWindow, &SettingsWindow::signalSetVoiceStartValue, this, &MainWindow::slotApplyVoiceStartValue);
    connect(pSettingsWindow, &SettingsWindow::signalSetShouldHearTestVoice, this, &MainWindow::slotApplyShouldHearTestVoice);
    connect(pSettingsWindow, &SettingsWindow::signalRegisterMuteMicButton, this, &MainWindow::slotRegisterMuteMicButton);

    pController->unpauseTestRecording();
    pSettingsWindow->setWindowModality(Qt::ApplicationModal);
    pSettingsWindow->setWindowOpacity(0);
    pSettingsWindow->show();



    // Show "cool animation"

    qreal opacity = 0.1;

    for (int i = 1; i < 11; i++)
    {
        pSettingsWindow->setWindowOpacity( opacity * i );

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void MainWindow::applyDefaultTheme()
{
    // Set the default theme

    QFile File;
    File.setFileName(QString(STYLE_THEMES_PATH_FROM_EXE)
                      + QString(STYLE_THEME_DEFAULT_NAME)
                      + ".css");

    if( File.exists()
        &&
        File.open(QFile::ReadOnly) )
    {
        QString StyleSheet = QLatin1String( File.readAll() );

        qApp->setStyleSheet(StyleSheet);

        File.close();
    }
    else
    {
        QMessageBox::warning(this, "Error", "Could not open default theme file \"" + QString(STYLE_THEME_DEFAULT_NAME) + ".css\".");
    }
}

void MainWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)

    hide();
    pTrayIcon->show();
}

void MainWindow::on_actionSettings_triggered()
{
    std::vector<std::wstring> vInputDevices = pController->getInputDevices();
    std::vector<QString> vAudioInputDevices;

    for (size_t i = 0; i < vInputDevices.size(); i++)
    {
        vAudioInputDevices.push_back(QString::fromStdWString(vInputDevices[i]));
    }

    // Show SettingsWindow
    SettingsWindow* pSettingsWindow = new SettingsWindow(pController->getSettingsManager(), vAudioInputDevices, this);
    connect(pSettingsWindow, &SettingsWindow::applyNewMasterVolume, this, &MainWindow::slotApplyMasterVolume);
    connect(this, &MainWindow::signalShowVoiceVolumeValueInSettings, pSettingsWindow, &SettingsWindow::slotSetVoiceVolume);
    connect(pSettingsWindow, &SettingsWindow::closedSettingsWindow, this, &MainWindow::slotSettingsWindowClosed);
    connect(pSettingsWindow, &SettingsWindow::signalSetAudioInputVolume, this, &MainWindow::slotApplyAudioInputVolume);
    connect(pSettingsWindow, &SettingsWindow::signalSetVoiceStartValue, this, &MainWindow::slotApplyVoiceStartValue);
    connect(pSettingsWindow, &SettingsWindow::signalSetShouldHearTestVoice, this, &MainWindow::slotApplyShouldHearTestVoice);
    connect(pSettingsWindow, &SettingsWindow::signalRegisterMuteMicButton, this, &MainWindow::slotRegisterMuteMicButton);
    pController->unpauseTestRecording();
    pSettingsWindow->setWindowModality(Qt::ApplicationModal);
    pSettingsWindow->show();
}

void MainWindow::slotOnMenuClose()
{
    ui->listWidget_users->clearSelection();
}

void MainWindow::onExecCalled()
{
    // Setup tray icon

    pTrayIcon      = new QSystemTrayIcon(this);

    QIcon icon     = QIcon(RES_ICONS_MAIN_PATH);
    pTrayIcon     ->setIcon(icon);

    connect(pTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::slotTrayIconActivated);




    // Output HTML

    outputHTMLtimeStart    = "<font style=\"color: ";
    outputHTMLmessageStart = "<font color=\"";
    outputHTMLmessageEnd   = "</font>";


    // This to this connects

    connect(this, &MainWindow::signalTypeOnScreen,                 this, &MainWindow::typeSomeOnScreen);
    connect(this, &MainWindow::signalEnableInteractiveElements,    this, &MainWindow::slotEnableInteractiveElements);
    connect(this, &MainWindow::signalShowMessageBox,               this, &MainWindow::slotShowMessageBox);
    connect(this, &MainWindow::signalShowUserDisconnectNotice,     this, &MainWindow::slotShowUserDisconnectNotice);
    connect(this, &MainWindow::signalShowUserConnectNotice,        this, &MainWindow::slotShowUserConnectNotice);
    connect(this, &MainWindow::signalApplyTheme,                   this, &MainWindow::slotApplyTheme);
    connect(this, &MainWindow::signalClearTextEdit,                this, &MainWindow::slotClearTextEdit);
    connect(this, &MainWindow::signalClearTextChatOutput,          this, &MainWindow::slotClearTextChatOutput);
    connect(this, &MainWindow::signalShowOldText,                  this, &MainWindow::slotShowOldText);
    connect(this, &MainWindow::signalSetConnectDisconnectButton,   this, &MainWindow::slotSetConnectDisconnectButton);
    connect(this, &MainWindow::signalShowPasswordInputWindow,      this, &MainWindow::slotShowPasswordInputWindow);
    connect(this, &MainWindow::signalCreateRoom,                   this, &MainWindow::slotCreateRoom);
    connect(this, &MainWindow::signalShowServerMessage,            this, &MainWindow::slotShowServerMessage);


    // Setup context menu for 'connected users' list

    ui->listWidget_users->setContextMenuPolicy (Qt::CustomContextMenu);
    ui->listWidget_users->setViewMode          (QListView::ListMode);

    pMenuContextMenu    = new QMenu(this);
    connect(pMenuContextMenu, &QMenu::aboutToHide, this, &MainWindow::slotOnMenuClose);

    pActionChangeVolume = new QAction("Change User Volume");
    pActionEnterRoom    = new QAction("Enter Room");

    pMenuContextMenu->addAction(pActionChangeVolume);
    pMenuContextMenu->addAction(pActionEnterRoom);

    connect(pActionChangeVolume, &QAction::triggered, this, &MainWindow::slotChangeUserVolume);
    connect(pActionEnterRoom, &QAction::triggered, this, &MainWindow::slotEnterRoom);




    // Register types

    qRegisterMetaType <SilentMessage>        ("SilentMessage");
    qRegisterMetaType <std::string>          ("std::string");
    qRegisterMetaType <QTextBlock>           ("QTextBlock");
    qRegisterMetaType <QVector<int>>         ("QVector<int>");
    qRegisterMetaType <size_t>               ("size_t");
    qRegisterMetaType <std::vector<QString>> ("std::vector<QString>");




    // Setup Connect window

    pConnectWindow = new connectWindow(this);
    pConnectWindow->setWindowModality(Qt::WindowModality::WindowModal);

    connect(pConnectWindow, &connectWindow::connectTo,      this, &MainWindow::connectTo);
    connect(pConnectWindow, &connectWindow::showMainWindow, this, &MainWindow::show);


    connect(ui->plainTextEdit_input, &CustomQPlainTextEdit::signalReturnPressed, this, &MainWindow::customqplaintextedit_return_pressed);

    applyDefaultTheme();

    // Create Controller

    pController    = new Controller(this);

    slotApplyTheme();

    if ( pController->isSettingsCreatedFirstTime() || pController->isSettingsFileInOldFormat() )
    {
        // Show SettingsWindow to the user.

        pTimer = new QTimer();
        pTimer->setInterval(500);
        connect(pTimer, &QTimer::timeout, this, &MainWindow::showSettingsWindow);
        pTimer->start();
    }
    else
    {
        pTimer = nullptr;
    }


    if (pController->getCurrentSettingsFile()->iMuteMicrophoneButton != 0)
    {
        RegisterHotKey((HWND)MainWindow::winId(), 100, MOD_NOREPEAT, static_cast<unsigned int>(pController->getCurrentSettingsFile()->iMuteMicrophoneButton));

        bMuteMicButtonRegistered = true;
    }
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
                ui->listWidget_users->clearSelection();
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
        SListItem* pItem = dynamic_cast<SListItem*>(ui->listWidget_users->currentItem());

        if (pItem->isRoom() == false)
        {
            SListItemUser* pUser = dynamic_cast<SListItemUser*>(pItem);

            SingleUserSettings* pUserSettings = new SingleUserSettings(pUser->getName(), pController->getUserCurrentVolume(pUser->getName().toStdString()), this);
            connect(pUserSettings, &SingleUserSettings::signalChangeUserVolume, this, &MainWindow::slotSetNewUserVolume);
            pUserSettings->setWindowModality(Qt::WindowModality::WindowModal);
            pUserSettings->show();
        }
    }
}

void MainWindow::slotSetNewUserVolume(QString sUserName, float fVolume)
{
    pController->setNewUserVolume(sUserName.toStdString(), fVolume);
}

void MainWindow::slotDeleteUserFromList(SListItemUser* pListWidgetItem, bool bDeleteAll, std::promise<bool>* resultPromise)
{
    if (bDeleteAll)
    {
        ui->listWidget_users->deleteAll();
    }
    else
    {
        ui->listWidget_users->deleteUser(pListWidgetItem);
    }

    resultPromise->set_value(false);
}

void MainWindow::on_actionAbout_2_triggered()
{
    AboutWindow* pAboutWindow = new AboutWindow ( QString::fromStdString(pController->getClientVersion()), this );
    pAboutWindow->setWindowModality (Qt::WindowModality::WindowModal);
    pAboutWindow->show();
}





void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    pController->stop();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - dragPosition);
        event->accept();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::on_listWidget_users_itemDoubleClicked(QListWidgetItem *item)
{
    if (ui->listWidget_users->currentRow() >= 0)
    {
        SListItem* pItem = dynamic_cast<SListItem*>(item);

        if (pItem->isRoom())
        {
            SListItemRoom* pRoom = dynamic_cast<SListItemRoom*>(pItem);

            if (pRoom == pController->getCurrentUserRoom())
            {
                ui->listWidget_users->clearSelection();
            }
            else
            {
                pController->enterRoom(pRoom->getRoomName().toStdString());
                ui->listWidget_users->clearSelection();
            }
        }
        else
        {
            ui->listWidget_users->clearSelection();
        }
    }
}

void MainWindow::on_listWidget_users_itemClicked(QListWidgetItem *item)
{
    if (ui->listWidget_users->currentRow() >= 0)
    {
        SListItem* pItem = dynamic_cast<SListItem*>(item);

        if (pItem->isRoom() == false)
        {
            ui->listWidget_users->clearSelection();
        }
    }
}

bool MainWindow::filterMessageText(std::wstring &sMessage)
{
    // Delete empty new lines at the end.

    for (int i = static_cast<int>(sMessage.size() - 1); i >= 0; i--)
    {
        if (sMessage[static_cast<size_t>(i)] == L'\n')
        {
            sMessage.pop_back();
        }
        else
        {
            break;
        }
    }



    // Check if the first char is '\n'.

    if (sMessage[0] == L'\n')
    {
        sMessage.erase( sMessage.begin() );
    }



    // Delete empty new lines in a row.

    bool bWasNewLine = false;

    for (size_t i = 0; i < sMessage.size(); i++)
    {
        if (sMessage[i] == L'\n')
        {
            if (bWasNewLine)
            {
                sMessage.erase( sMessage.begin() + static_cast<long long>(i) );
                i--;
            }
            else
            {
                bWasNewLine = true;
            }
        }
        else if (bWasNewLine)
        {
            bWasNewLine = false;
        }
    }



    // Count how much new lines there are.

    int iNewLineCount = 0;

    for (size_t i = 0; i < sMessage.size(); i++)
    {
        if (sMessage[i] == L'\n')
        {
            iNewLineCount++;
        }
    }

    if (iNewLineCount > MAX_NEW_LINE_COUNT_IN_MESSAGE)
    {
        return true;
    }



    // Check if the message contains more than one line.

    bool bIsMultiline = false;

    for (size_t i = 0; i < sMessage.size(); i++)
    {
        if (sMessage[i] == L'\n')
        {
            bIsMultiline = true;
            break;
        }
    }

    if (bIsMultiline)
    {
        sMessage.insert(sMessage.begin(), '\n');
    }

    return false;
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)

    MSG* msg = reinterpret_cast<MSG*>(message);

    if(msg->message == WM_HOTKEY)
    {
        if(msg->wParam == 100)
        {
            if (pController->getCurrentSettingsFile()->iMuteMicrophoneButton != 0)
            {
                pController->setMuteMic( !pController->getMuteMic() );

                pController->playMuteMicSound( pController->getMuteMic() );
            }

            return true;
        }
    }

    return false;
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    AboutQtWindow* pAboutQtWindow = new AboutQtWindow(this);
    pAboutQtWindow->setWindowModality(Qt::WindowModality::WindowModal);

    pAboutQtWindow->show();
}

MainWindow::~MainWindow()
{
    delete pActionChangeVolume;
    delete pActionEnterRoom;
    delete pMenuContextMenu;

    if (pTimer)
    {
        delete pTimer;
    }
    delete pController;
    delete pConnectWindow;

    delete pTrayIcon;
    delete ui;
}

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
#include "../src/View/CustomQPlainTextEdit/customqplaintextedit.h"


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




    // This to this connects

    connect(this, &MainWindow::signalTypeOnScreen,                 this, &MainWindow::typeSomeOnScreen);
    connect(this, &MainWindow::signalEnableInteractiveElements,    this, &MainWindow::slotEnableInteractiveElements);
    connect(this, &MainWindow::signalShowMessageBox,               this, &MainWindow::slotShowMessageBox);
    connect(this, &MainWindow::signalPingAndTalkingToUser,         this, &MainWindow::slotPingAndTalkingToUser);
    connect(this, &MainWindow::signalShowUserDisconnectNotice,     this, &MainWindow::slotShowUserDisconnectNotice);
    connect(this, &MainWindow::signalShowUserConnectNotice,        this, &MainWindow::slotShowUserConnectNotice);
    connect(this, &MainWindow::signalApplyTheme,                   this, &MainWindow::slotApplyTheme);
    connect(this, &MainWindow::signalDeleteUserFromList,           this, &MainWindow::slotDeleteUserFromList);



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

void MainWindow::slotPingAndTalkingToUser(std::string sUserName, QListWidgetItem* pListWidgetItem, int iPing, bool bTalking)
{
    mtxList .lock();


    QString sUserNameInList = QString::fromStdString( sUserName );

    if (iPing >= pController ->getPingWarningBelow())
    {
        sUserNameInList += " [" + QString::number(iPing) + " ms (!!!)]";
    }
    else
    {
        if (iPing == 0)
        {
            sUserNameInList += " [-- ms]";
        }
        else
        {
            sUserNameInList += " [" + QString::number(iPing) + " ms]";
        }
    }


    pListWidgetItem ->setText( sUserNameInList );





    // Set color on ping circle

    if (bTalking)
    {
        if      (iPing <= pController->getPingNormalBelow())
        {
            pListWidgetItem ->setIcon(QIcon(RES_ICONS_USERPING_NORMAL_TALK));
        }
        else if (iPing <= pController->getPingWarningBelow())
        {
            pListWidgetItem ->setIcon(QIcon(RES_ICONS_USERPING_WARNING_TALK));
        }
        else
        {
            pListWidgetItem ->setIcon(QIcon(RES_ICONS_USERPING_BAD_TALK));
        }
    }
    else
    {
        if      (iPing <= pController->getPingNormalBelow())
        {
            pListWidgetItem ->setIcon(QIcon(RES_ICONS_USERPING_NORMAL));
        }
        else if (iPing <= pController->getPingWarningBelow())
        {
            pListWidgetItem ->setIcon(QIcon(RES_ICONS_USERPING_WARNING));
        }
        else
        {
            pListWidgetItem ->setIcon(QIcon(RES_ICONS_USERPING_BAD));
        }
    }


    mtxList .unlock();
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
        ui ->actionDisconnect    ->setEnabled(true);
    }
    else
    {
        ui ->actionConnect       ->setEnabled(false);
        ui ->actionDisconnect    ->setEnabled(false);
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

void MainWindow::slotTrayIconActivated()
{
    pTrayIcon ->hide();

    raise           ();
    activateWindow  ();
    showNormal      ();
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

void MainWindow::setPingAndTalkingToUser(std::string sUserName, QListWidgetItem* pListWidgetItem, int iPing, bool bTalking)
{
    emit signalPingAndTalkingToUser(sUserName, pListWidgetItem, iPing, bTalking);
}


QListWidgetItem* MainWindow::addNewUserToList(std::string name)
{
    QString qName = QString::fromStdString(name) + " [-- ms]";

    QListWidgetItem* pNewItem = new QListWidgetItem(qName);

    ui->listWidget_users->addItem( pNewItem );

    return pNewItem;
}

void MainWindow::deleteUserFromList(QListWidgetItem* pListWidgetItem, bool bDeleteAll)
{
    emit signalDeleteUserFromList(pListWidgetItem, bDeleteAll);
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
    emit signalShowMessageBox(bWarningBox, message);
}

void MainWindow::clearTextEdit()
{
    ui ->plainTextEdit_input ->clear();
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

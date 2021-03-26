// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

// Qt
#include <QMainWindow>
#include <QTimer>

// STL
#include <vector>
#include <string>
#include <mutex>
#include <future>

// Custom
#include "Model/OutputTextType.h"


class connectWindow;
class Controller;
class QMouseEvent;
class QTimer;
class QMenu;
class QAction;
class QSystemTrayIcon;
class QListWidgetItem;
class SettingsFile;
class SListItemUser;

namespace Ui
{
    class MainWindow;
}

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

#define MAX_NEW_LINE_COUNT_IN_MESSAGE 10


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow(QWidget *parent = nullptr);


    // Print on Chat Room QPlainTextEdit.

        void              printUserMessage               (const std::string& sTimeInfo, const std::wstring& sMessage, SilentMessage messageColor, bool bEmitSignal = false);
        void              printOutput                    (const std::string& sText, SilentMessage messageColor, bool bEmitSignal = false);
        void              printOutputW                   (const std::wstring& sText, SilentMessage messageColor, bool bEmitSignal = false);
        void              showUserDisconnectNotice       (std::string sName, SilentMessage messageColor, char cUserLost);
        void              showUserConnectNotice          (std::string sName, SilentMessage messageColor);
        void              showOldText                    (wchar_t* pText);


    // Update UI elements.

        void              setPingAndTalkingToUser        (SListItemUser* pListWidgetItem, int iPing, bool bTalking);
        void              deleteUserFromList             (SListItemUser* pListWidgetItem, bool bDeleteAll = false);
        void              enableInteractiveElements      (bool bMenu, bool bTypeAndSend);
        void              setOnlineUsersCount            (int iOnlineCount);
        void              setConnectDisconnectButton     (bool bConnect);
        SListItemUser*    addNewUserToList               (const std::string& sName);
        void              addRoom                        (std::string sRoomName, std::wstring sPassword = L"", size_t iMaxUsers = 0, bool bFirstRoom = false);
        size_t            getRoomCount                   ();
        SListItemUser*    addUserToRoomIndex             (const std::string& sName, size_t iRoomIndex);
        void              moveUserToRoom                 (SListItemUser* pUser, std::string sRoomName);
        void              moveRoom                       (const std::string& sRoomName, bool bMoveUp);
        void              deleteRoom                     (const std::string& sRoomName);
        void              createRoom                     (const std::string& sName, const std::u16string& sPassword, size_t iMaxUsers);
        void              changeRoomSettings             (const std::string& sOldName, const std::string& sNewName, size_t iMaxUsers);


    // Input message QPlainTextEdit.

        void              clearTextEdit                  ();


    // Other.

        void              showVoiceVolumeValueInSettings (int iVolume);
        void              showMessageBox                 (bool bWarningBox, std::string sMessage);
        void              showPasswordInputWindow        (std::string sRoomName);
        void              showServerMessage              (std::string sMessage);
        void              applyTheme                     ();

    ~MainWindow();


signals:

    // Print on Chat Room QPlainTextEdit.

        void signalTypeOnScreen                    (QString sText, SilentMessage messageColor, bool bUserMessage = false);
        void signalShowUserDisconnectNotice        (std::string sName, SilentMessage messageColor, char cUserLost);
        void signalShowUserConnectNotice           (std::string sName, SilentMessage messageColor);
        void signalShowOldText                     (wchar_t* pText);
        void signalClearTextEdit                   ();
        void signalClearTextChatOutput             ();


    // Update UI elements.

        void signalPingAndTalkingToUser            (std::string sUserName, QListWidgetItem* pListWidgetItem, int iPing, bool bTalking);
        void signalEnableInteractiveElements       (bool bMenu, bool bTypeAndSend);
        void signalSetConnectDisconnectButton      (bool bConnect);
        void signalCreateRoom                      (QString sName, QString sPassword, size_t iMaxUsers, std::promise<bool>* resultPromise);
        void signalDeleteUserFromList              (SListItemUser* pListWidgetItem, bool bDeleteAll, std::promise<bool>* resultPromise);
        void signalMoveUserToRoom                  (SListItemUser *pUser, QString sRoomName, std::promise<bool>* promiseResult);
        void signalAddRoom                         (QString sRoomName, QString sPassword, size_t iMaxUsers, bool bFirstRoom, std::promise<int>* promiseRoomCount);


    // Other.

        void signalShowVoiceVolumeValueInSettings  (int iVolume);
        void signalShowMessageBox                  (bool bWarningBox, std::string sMessage);
        void signalShowPasswordInputWindow         (std::string sRoomName);
        void signalShowServerMessage               (QString sMessage);
        void signalDeleteRoom                      (QString sRoomName, std::promise<int>* promiseRoomCount);
        void signalMoveRoom                        (QString sRoomName, bool bMoveUp, std::promise<bool>* promiseResult);
        void signalAddUserToRoomIndex              (QString sName, size_t iRoomIndex, std::promise<SListItemUser*>* promiseResult);
        void signalAddNewUserToList                (QString sName, std::promise<SListItemUser*>* promiseResult);
        void signalApplyTheme                      ();


protected:

    bool nativeEvent     (const QByteArray &eventType, void *message, long *result);

    void hideEvent       (QHideEvent  *event);
    void closeEvent      (QCloseEvent *event);
    void mouseMoveEvent  (QMouseEvent *event);
    void mousePressEvent (QMouseEvent *event);


public slots:

    void onExecCalled    ();


private slots:

    // Called from ConnectWindow

        void connectTo                          (std::string sAdress, std::string sPort, std::string sUserName, std::wstring sPass);


    // Context menu in QListWidget

        void  on_listWidget_users_customContextMenuRequested (const QPoint &pos);
        void  slotSetNewUserVolume                           (QString sUserName, float fVolume);
        void  slotChangeUserVolume                           ();
        void  slotEnterRoom                                  ();
        void  on_listWidget_users_itemDoubleClicked          (QListWidgetItem *item);


    // Update UI elements

        void  slotDeleteUserFromList            (SListItemUser* pListWidgetItem, bool bDeleteAll, std::promise<bool>* resultPromise);
        void  slotEnableInteractiveElements     (bool bMenu, bool bTypeAndSend);
        void  slotSetConnectDisconnectButton    (bool bConnect);
        void  slotCreateRoom                    (QString sName, QString sPassword, size_t iMaxUsers, std::promise<bool>* resultPromise);


    // Print on Chat Room QPlainTextEdit

        void  typeSomeOnScreen                  (QString sText, SilentMessage messageColor, bool bUserMessage = false);
        void  slotShowUserDisconnectNotice      (std::string sName, SilentMessage messageColor, char cUserLost);
        void  slotShowUserConnectNotice         (std::string sName, SilentMessage messageColor);
        void  slotShowOldText                   (wchar_t* pText);
        void  slotClearTextEdit                 ();
        void  slotClearTextChatOutput           ();


    // Tray Icon

        void  slotTrayIconActivated             ();


    // QMenu

        void  on_actionAbout_2_triggered        ();
        void  on_actionConnect_triggered        ();
        void  on_actionSettings_triggered       ();
        void  on_actionAbout_Qt_triggered       ();

        void  slotOnMenuClose                   ();


    // Other

        void  slotShowMessageBox                (bool bWarningBox, std::string message);
        void  slotShowPasswordInputWindow       (std::string sRoomName);
        void  slotShowServerMessage             (QString sMessage);
        void  slotEnterRoomWithPassword         (QString sRoomName, QString sPassword);
        void  slotApplyAudioInputVolume         (int iVolume);
        void  slotApplyVoiceStartValue          (int iValue);
        void  slotApplyShouldHearTestVoice      (bool bHear);
        void  slotApplyTheme                    ();
        void  slotApplyMasterVolume             ();
        void  slotSettingsWindowClosed          ();
        void  slotAddRoom                       (QString sRoomName, QString sPassword, size_t iMaxUsers, bool bFirstRoom, std::promise<int>* promiseRoomCount);
        void  slotDeleteRoom                    (QString sRoomName, std::promise<int>* promiseRoomCount);
        void  slotMoveUserToRoom                (SListItemUser *pUser, QString sRoomName, std::promise<bool>* promiseResult);
        void  slotMoveRoom                      (QString sRoomName, bool bMoveUp, std::promise<bool>* promiseResult);
        void  slotAddUserToRoomIndex            (QString sName, size_t iRoomIndex, std::promise<SListItemUser*>* promiseResult);
        void  slotAddNewUserToList              (QString sName, std::promise<SListItemUser*>* promiseResult);
        void  slotRegisterMuteMicButton         (int iButton);

        void  slotMaxWindow                     ();
        void  slotHideWindow                    ();


    // UI

        void  customqplaintextedit_return_pressed ();
        void  on_listWidget_users_itemClicked     (QListWidgetItem *item);



private:

    bool filterMessageText  (std::wstring& sMessage);
    void showSettingsWindow ();
    void closeApp           ();
    void applyDefaultTheme  ();



    // ------------------------------------------


    Ui::MainWindow*  ui;
    connectWindow*   pConnectWindow;
    Controller*      pController;


    // Context menu in list
    QMenu*           pMenuContextMenu;
    QAction*         pActionChangeVolume;
    QAction*         pActionEnterRoom;


    QTimer*          pTimer;


    QSystemTrayIcon* pTrayIcon;


    std::mutex       mtxPrintOutput;
    std::mutex       mtxList;


    QString          outputHTMLtimeStart;
    QString          outputHTMLmessageStart;
    QString          outputHTMLmessageEnd;


    QPoint           dragPosition;


    bool             bAbleToSend;
    bool             bMuteMicButtonRegistered;
};

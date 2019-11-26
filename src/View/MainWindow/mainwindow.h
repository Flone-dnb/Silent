#pragma once

// Qt
#include <QMainWindow>
#include <QTimer>

// STL
#include <vector>
#include <string>
#include <mutex>

// Custom
#include "../src/Model/OutputTextType.h"



class connectWindow;
class Controller;
class QMouseEvent;
class QTimer;
class QMenu;
class QAction;
class QSystemTrayIcon;
class QListWidgetItem;
class SettingsFile;

namespace Ui
{
    class MainWindow;
}


// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow(QWidget *parent = nullptr);


    // Print on Chat Room QPlainTextEdit

        void              printUserMessage           (std::string timeInfo,  std::wstring message,             SilentMessageColor messageColor, bool bEmitSignal = false);
        void              printOutput                (std::string text,      SilentMessageColor messageColor,  bool bEmitSignal = false);
        void              printOutputW               (std::wstring text,     SilentMessageColor messageColor,  bool bEmitSignal = false);
        void              showUserDisconnectNotice   (std::string name,      SilentMessageColor messageColor,  char cUserLost);
        void              showUserConnectNotice      (std::string name,      SilentMessageColor messageColor);


    // Update UI elements

        void              setPingAndTalkingToUser    (std::string sUserName,             QListWidgetItem* pListWidgetItem, int iPing, bool bTalking);
        void              deleteUserFromList         (QListWidgetItem* pListWidgetItem,  bool bDeleteAll = false);
        void              enableInteractiveElements  (bool bMenu,                        bool bTypeAndSend);
        void              setOnlineUsersCount        (int onlineCount);
        QListWidgetItem*  addNewUserToList           (std::string name);


    // Input message QPlainTextEdit

        void              clearTextEdit              ();


    // Other

        void              showMessageBox             (bool bWarningBox, std::string message);
        void              applyTheme                 ();

    ~MainWindow();


signals:

    // Print on Chat Room QPlainTextEdit

        void signalTypeOnScreen                      (QString text,     SilentMessageColor messageColor, bool bUserMessage = false);
        void signalShowUserDisconnectNotice          (std::string name, SilentMessageColor messageColor, char cUserLost);
        void signalShowUserConnectNotice             (std::string name, SilentMessageColor messageColor);
        void signalClearTextEdit                     ();


    // Update UI elements

        void signalPingAndTalkingToUser              (std::string sUserName,            QListWidgetItem* pListWidgetItem, int iPing, bool bTalking);
        void signalEnableInteractiveElements         (bool bMenu,                       bool bTypeAndSend);
        void signalDeleteUserFromList                (QListWidgetItem* pListWidgetItem, bool bDeleteAll = false);


    // Other

        void signalShowMessageBox                    (bool bWarningBox, std::string message);
        void signalApplyTheme                        ();


protected:

    void hideEvent     (QHideEvent* event);
    void closeEvent    (QCloseEvent *event);


private slots:

    // Called from ConnectWindow

        void connectTo                          (std::string adress, std::string port, std::string userName, std::wstring sPass);


    // Context menu in QListWidget

        void  on_listWidget_users_customContextMenuRequested (const QPoint &pos);
        void  slotSetNewUserVolume                           (QString userName, float fVolume);
        void  slotChangeUserVolume                           ();


    // Called from SettingsWindow

        void  slotSaveSettings                  (SettingsFile* pSettingsFile);


    // Update UI elements

        void  slotPingAndTalkingToUser          (std::string sUserName,            QListWidgetItem* pListWidgetItem, int iPing, bool bTalking);
        void  slotDeleteUserFromList            (QListWidgetItem* pListWidgetItem, bool bDeleteAll = false);
        void  slotEnableInteractiveElements     (bool bMenu,                       bool bTypeAndSend);


    // Print on Chat Room QPlainTextEdit

        void  typeSomeOnScreen                  (QString text,     SilentMessageColor messageColor, bool bUserMessage = false);
        void  slotShowUserDisconnectNotice      (std::string name, SilentMessageColor messageColor, char cUserLost);
        void  slotShowUserConnectNotice         (std::string name, SilentMessageColor messageColor);
        void  slotClearTextEdit                 ();


    // Tray Icon

        void  slotTrayIconActivated             ();


    // QMenu

        void  on_actionAbout_2_triggered        ();
        void  on_actionConnect_triggered        ();
        void  on_actionDisconnect_triggered     ();
        void  on_actionSettings_triggered       ();


    // Other

        void  slotShowMessageBox                (bool bWarningBox, std::string message);
        void  slotApplyTheme                    ();


    // UI

        void  on_pushButton_clicked             ();
        void  customqplaintextedit_return_pressed();


private:

    void showSettingsWindow ();
    void closeApp           ();



    // ------------------------------------------


    Ui::MainWindow*  ui;
    connectWindow*   pConnectWindow;
    Controller*      pController;


    // Context menu in list
    QMenu*           pMenuContextMenu;
    QAction*         pActionChangeVolume;


    QTimer*          pTimer;


    QSystemTrayIcon* pTrayIcon;


    std::mutex       mtxPrintOutput;
    std::mutex       mtxList;


    QString          outputHTMLtimeStart;
    QString          outputHTMLmessageStart;
    QString          outputHTMLmessageEnd;
};

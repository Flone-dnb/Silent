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


class User
{
public:

    User(std::string sUserName, int ping, QListWidgetItem* pItem)
    {
        this->sUserName = sUserName;
        this->ping      = ping;
        this->pItem     = pItem;
        bTalking        = false;
    }

    QListWidgetItem* pItem;
    std::string sUserName;
    int ping;
    bool bTalking;
};


// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow(QWidget *parent = nullptr);

    void connectTo                      (std::string adress, std::string port, std::string userName);

        void  printOutput               (std::string text, SilentMessageColor messageColor, bool bEmitSignal = false);
        void  printOutputW              (std::wstring text, SilentMessageColor messageColor, bool bEmitSignal = false);
        void  printUserMessage          (std::string timeInfo, std::wstring message, SilentMessageColor messageColor, bool bEmitSignal = false);
        void  enableInteractiveElements (bool bMenu, bool bTypeAndSend);
        void  setOnlineUsersCount       (int onlineCount);
        void  setPingToUser             (std::string userName, int ping);
        void  setUserIsTalking          (std::string userName, bool bTalking);
        void  addNewUserToList          (std::string name);
        void  deleteUserFromList        (std::string name, bool bDeleteAll = false);
        void  showUserDisconnectNotice  (std::string name, SilentMessageColor messageColor, bool bUserLost);
        void  showUserConnectNotice     (std::string name, SilentMessageColor messageColor);
        void  showMessageBox            (bool bWarningBox, std::string message);
        void  clearTextEdit             ();

    ~MainWindow();

signals:

    void signalShowUserConnectNotice(std::string name, SilentMessageColor messageColor);
    void signalShowUserDisconnectNotice(std::string name, SilentMessageColor messageColor, bool bUserLost);
    void signalTypeOnScreen(QString text, SilentMessageColor messageColor, bool bUserMessage = false);
    void signalShowMessage(bool bWarningBox, std::string message);
    void signalSetPingToUser(std::string userName, int ping);
    void signalSetUserIsTalking(std::string userName, bool bTalking);
    void signalEnableInteractiveElements(bool bMenu, bool bTypeAndSend);

protected:

    void hideEvent(QHideEvent* event);
    void closeEvent(QCloseEvent *event);


private slots:

    // Context menu in list
    void slotChangeUserVolume();

    void slotSaveSettings(SettingsFile* pSettingsFile);

    void slotSetNewUserVolume(QString userName, float fVolume);

    void slotShowMessage(bool bWarningBox, std::string message);
    void slotSetPingToUser(std::string userName, int ping);
    void slotSetUserIsTalking(std::string userName, bool bTalking);
    void typeSomeOnScreen(QString text, SilentMessageColor messageColor, bool bUserMessage = false);
    void slotEnableInteractiveElements(bool bMenu, bool bTypeAndSend);
    void slotTrayIconActivated();
    void slotShowUserDisconnectNotice(std::string name, SilentMessageColor messageColor, bool bUserLost);
    void slotShowUserConnectNotice(std::string name, SilentMessageColor messageColor);

    void on_actionAbout_2_triggered();
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void on_actionSettings_triggered();

    void on_pushButton_clicked();
    void on_plainTextEdit_2_textChanged();
    void on_listWidget_users_customContextMenuRequested(const QPoint &pos);

private:

    void showSettingsWindow();
    void closeApp();



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


    QString          outputHTMLmessageStart;
    QString          outputHTMLmessageEnd;


    std::vector<User> users;
};

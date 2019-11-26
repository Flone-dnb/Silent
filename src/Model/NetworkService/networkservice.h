#pragma once


// STL
#include <string>
#include <vector>
#include <ctime>
#include <mutex>


class MainWindow;
class AudioService;
class SettingsManager;

class User;


#define  CLIENT_VERSION                "2.22.4"




// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



class NetworkService
{
public:

    NetworkService(MainWindow* pMainWindow, AudioService* pAudioService, SettingsManager* pSettingsManager);



    // Start

        void  start                            (std::string adress,   std::string port,   std::string userName,  std::wstring sPass = L"");
        void  connectTo                        (std::string adress,   std::string port,   std::string userName,  std::wstring sPass = L"");
        void  setupVoiceConnection             ();


    // Listen

        void  listenTCPFromServer              ();
        void  listenUDPFromServer              ();


    // Receive

        void  receiveInfoAboutNewUser          ();
        void  receiveMessage                   ();
        void  receivePing                      ();


    // Send

        void  sendVoiceMessage                 (char* pVoiceMessage,   int iMessageSize,    bool bLast);
        void  sendMessage                      (std::wstring message);


    // Stop / Delete / Disconnect

        void  deleteDisconnectedUserFromList   ();
        void  disconnect                       ();
        void  lostConnection                   ();
        void  answerToFIN                      ();
        void  stop                             ();



    // GET functions

        std::string    getClientVersion        () const;
        std::string    getUserName             () const;
        unsigned short getPingNormalBelow      () const;
        unsigned short getPingWarningBelow     () const;

        size_t         getOtherUsersVectorSize () const;
        User*          getOtherUser            (size_t i) const;

        std::mutex*    getOtherUsersMutex      ();


private:


    // User in "Stop / Delete / Disconnect" functions

        void  eraseDisconnectedUser            (std::string sUserName, char cDisconnectType);
        void  clearWinsockAndThisUser          ();
        void  cleanUp                          ();


    // Checks if the server is dead

    void serverMonitor();



    // ------------------------------------



    MainWindow*        pMainWindow;
    AudioService*      pAudioService;
    SettingsManager*   pSettingsManager;
    User*              pThisUser;



    // Users
    std::vector<User*> vOtherUsers;


    // Ping
    unsigned short     iPingNormalBelow;
    unsigned short     iPingWarningBelow;


    std::mutex         mtxOtherUsers;


    clock_t            lastTimeServerKeepAliveCame;


    std::string        clientVersion;


    bool               bWinSockLaunched;
    bool               bTextListen;
    bool               bVoiceListen;
};

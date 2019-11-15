#pragma once

// C++
#include <string>
#include <vector>
#include <ctime>

// ============== Network ==============
// Sockets and stuff
#include <winsock2.h>

// Adress translation
#include <ws2tcpip.h>

// Winsock 2 Library
#pragma comment(lib,"Ws2_32.lib")


class MainWindow;
class AudioService;
class SettingsManager;


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



class NetworkService
{
public:

    NetworkService(MainWindow* pMainWindow, AudioService* pAudioService, SettingsManager* pSettingsManager);



    // Start

        void  start                          (std::string adress,   std::string port,   std::string userName);
        void  connectTo                      (std::string adress,   std::string port,   std::string userName);
        void  setupVoiceConnection           ();


    // Listen

        void  listenTCPFromServer            ();
        void  listenUDPFromServer            ();


    // Receive

        void  receiveInfoAboutNewUser        ();
        void  receiveMessage                 ();
        void  receivePing                    ();


    // Send

        void  sendVoiceMessage               (char* pVoiceMessage,   int iMessageSize,    bool bLast);
        void  sendMessage                    (std::wstring message);


    // Stop / Delete / Disconnect

        void  deleteDisconnectedUserFromList ();
        void  disconnect                     ();
        void  lostConnection                 ();
        void  answerToFIN                    ();
        void  stop                           ();


    // GET functions

        std::string    getClientVersion      ();
        std::string    getUserName           ();
        unsigned short getPingNormalBelow    ();
        unsigned short getPingWarningBelow   ();


private:

    void serverMonitor();



    // ------------------------------------



    MainWindow*      pMainWindow;
    AudioService*    pAudioService;
    SettingsManager* pSettingsManager;


    // Ping
    unsigned short iPingNormalBelow;
    unsigned short iPingWarningBelow;


    clock_t        lastTimeServerKeepAliveCame;


    SOCKET         userTCPSocket;
    SOCKET         userUDPSocket;
    sockaddr_in    serverAddr;


    std::string    userName;
    std::string    clientVersion;


    bool           bWinSockLaunched;
    bool           bTextListen;
    bool           bVoiceListen;
};

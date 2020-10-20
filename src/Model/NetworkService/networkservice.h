// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// STL
#include <string>
#include <vector>
#include <ctime>
#include <mutex>
#include <random>

// Other
#include "basetsd.h"


class MainWindow;
class AudioService;
class SettingsManager;

class User;
class SListItemRoom;

class AES;


#define  CLIENT_VERSION                "3.3.0"




// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



class NetworkService
{
public:

    NetworkService(MainWindow* pMainWindow, AudioService* pAudioService, SettingsManager* pSettingsManager);
    ~NetworkService();



    // Start

        void  start                            (std::string adress,   std::string port,   std::string userName,  std::wstring sPass = L"");
        void  connectTo                        (std::string adress,   std::string port,   std::string userName,  std::wstring sPass = L"");


    // Listen

        void  listenTCPFromServer              ();
        void  listenUDPFromServer              ();


    // Send

        void  sendVoiceMessage                 (char* pVoiceMessage,   int iMessageSize,    bool bLast);
        void  sendMessage                      (std::wstring message);


    // Rooms

        void  enterRoom                        (std::string sName);
        void  enterRoomWithPassword            (std::string sRoomName, std::wstring sPassword);


    // Stop / Delete / Disconnect

        void  deleteDisconnectedUserFromList   ();
        void  disconnect                       ();
        void  lostConnection                   ();
        void  answerToFIN                      ();
        void  stop                             ();



    // GET functions

        std::string    getClientVersion        () const;
        std::string    getUserName             () const;
        SListItemRoom* getUserRoom             () const;

        size_t         getOtherUsersVectorSize () const;
        User*          getOtherUser            (size_t i) const;

        std::mutex*    getOtherUsersMutex      ();


private:

    // Startup

        void  generateKeys                     ();


    // Receive

        void  receiveInfoAboutNewUser          ();
        void  receiveMessage                   ();
        void  receivePing                      ();
        void  receiveServerMessage             ();


    // User in "Stop / Delete / Disconnect" functions.

        void  eraseDisconnectedUser            (std::string sUserName, char cDisconnectType);
        void  clearWinsockAndThisUser          ();
        void  cleanUp                          ();


    // Checks if the server is dead.

        void serverMonitor                     ();


    // Erases spaces at the borders of the address string.

        std::string formatAdressString         (const std::string& sAdress);


    // Rooms

        void  canMoveToRoom                    ();
        void  userEntersRoom                   ();
        void  serverMovedRoom                  ();
        void  serverDeletesRoom                ();
        void  serverCreatesRoom                ();
        void  serverChangesRoom                ();


    // VOIP

        void setupVoiceConnection              ();
        bool sendVOIPReadyPacket               ();


    // ------------------------------------



    MainWindow*        pMainWindow;
    AudioService*      pAudioService;
    SettingsManager*   pSettingsManager;
    User*              pThisUser;
    AES*               pAES;
    std::mt19937_64*   pRndGen;


    std::vector<User*> vOtherUsers;


    std::mutex         mtxOtherUsers;
    std::mutex         mtxTCPRead;
    std::mutex         mtxUDPRead;
    std::mutex         mtxRooms;


    clock_t            lastTimeServerKeepAliveCame;


    std::string        clientVersion;
    char               vSecretAESKey[16];


    bool               bWinSockLaunched;
    bool               bTextListen;
    bool               bVoiceListen;
};

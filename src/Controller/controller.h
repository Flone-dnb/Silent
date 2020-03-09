// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// STL
#include <string>
#include <mutex>


class NetworkService;
class AudioService;
class MainWindow;
class SettingsManager;
class SettingsFile;
class SListItemRoom;


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class Controller
{
public:

    Controller(MainWindow* pMainWindow);



    // Start/stop

        void           connectTo                  (std::string sAdress,    std::string sPort,  std::string sUserName,  std::wstring sPass = L"");
        void           disconnect                 ();
        void           stop                       ();


    // Chat functions

        void           sendMessage                (std::wstring sMessage);
        void           enterRoom                  (std::string sRoomName);
        void           enterRoomWithPassword      (std::string sRoomName, std::wstring sPassword);


    // Settings

        void           saveSettings               (SettingsFile* pSettingsFile);
        void           setNewUserVolume           (std::string sUserName,  float fVolume);


    // GET functions

        float          getUserCurrentVolume       (std::string sUserName);
        std::string    getClientVersion           ();
        std::string    getUserName                ();
        unsigned short getPingNormalBelow         ();
        unsigned short getPingWarningBelow        ();
        SettingsFile*  getCurrentSettingsFile     ();
        bool           isSettingsCreatedFirstTime ();
        SListItemRoom* getCurrentUserRoom         ();




    ~Controller();

private:


    NetworkService*  pNetworkService;
    AudioService*    pAudioService;
    SettingsManager* pSettingsManager;
};

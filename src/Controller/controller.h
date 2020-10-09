// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// STL
#include <string>
#include <mutex>
#include <vector>


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
        void           unpauseTestRecording       ();
        void           pauseTestRecording         ();


    // Chat functions

        void           sendMessage                (std::wstring sMessage);
        void           enterRoom                  (std::string sRoomName);
        void           enterRoomWithPassword      (std::string sRoomName, std::wstring sPassword);


    // Settings

        void           applyNewMasterVolumeFromSettings       ();
        void           applyAudioInputVolume      (int iVolume);
        void           applyVoiceStartValue       (int iValue);
        void           applyShouldHearTestVoice   (bool bHear);
        void           setNewUserVolume           (std::string sUserName,  float fVolume);


    // GET functions

        SettingsManager* getSettingsManager       ();
        float          getUserCurrentVolume       (std::string sUserName);
        std::string    getClientVersion           ();
        std::string    getUserName                ();
        unsigned short getPingNormalBelow         ();
        unsigned short getPingWarningBelow        ();
        SettingsFile*  getCurrentSettingsFile     ();
        bool           isSettingsCreatedFirstTime ();
        bool           isSettingsFileInOldFormat  ();
        SListItemRoom* getCurrentUserRoom         ();
        std::vector<std::wstring> getInputDevices ();




    ~Controller();

private:


    NetworkService*  pNetworkService;
    AudioService*    pAudioService;
    SettingsManager* pSettingsManager;

    std::mutex       mtxSettings;
};

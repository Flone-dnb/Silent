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

        void           connectTo                  (const std::string& sAddress, const std::string& sPort, const std::string& sUserName, const std::wstring& sPass);
        void           disconnect                 ();
        void           stop                       ();
        void           unpauseTestRecording       ();
        void           pauseTestRecording         ();


    // Chat functions

        void           sendMessage                (const std::wstring& sMessage);
        void           enterRoom                  (const std::string& sRoomName);
        void           enterRoomWithPassword      (const std::string& sRoomName, const std::wstring& sPassword);


    // Settings

        void           applyNewMasterVolumeFromSettings       ();
        void           applyAudioInputVolume      (int iVolume);
        void           applyVoiceStartValue       (int iValue);
        void           applyShouldHearTestVoice   (bool bHear);
        void           setNewUserVolume           (const std::string& sUserName,  float fVolume);


    // Other

        void           playMuteMicSound           (bool bMuteSound);
        void           setMuteMic                 (bool bMute);
        bool           getMuteMic                 ();


    // GET functions

        SettingsManager* getSettingsManager         ();
        float            getUserCurrentVolume       (const std::string& sUserName);
        std::string      getClientVersion           ();
        std::string      getUserName                ();
        SettingsFile*    getCurrentSettingsFile     ();
        bool             isSettingsCreatedFirstTime ();
        bool             isSettingsFileInOldFormat  ();
        SListItemRoom*   getCurrentUserRoom         ();
        std::vector<std::wstring> getInputDevices ();




    ~Controller();

private:


    NetworkService*  pNetworkService;
    AudioService*    pAudioService;
    SettingsManager* pSettingsManager;

    std::mutex       mtxSettings;
};

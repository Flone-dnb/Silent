#pragma once


// STL
#include <string>
#include <mutex>


class NetworkService;
class AudioService;
class MainWindow;
class SettingsManager;
class SettingsFile;


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class Controller
{
public:

    Controller(MainWindow* pMainWindow);



    // Start/stop

        void           connectTo                  (std::string sAdress,    std::string sPort,  std::string sUserName);
        void           disconnect                 ();
        void           stop                       ();


    // Chat functions

        void           sendMessage                (std::wstring sMessage);


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




    ~Controller();

private:


    NetworkService*  pNetworkService;
    AudioService*    pAudioService;
    SettingsManager* pSettingsManager;
};

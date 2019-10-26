#pragma once

// STL
#include <string>

class NetworkService;
class AudioService;
class MainWindow;


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class Controller
{
public:

    Controller(MainWindow* pMainWindow);



    // Start/stop

        void         connectTo                     (std::string sAdress,    std::string sPort,  std::string sUserName);
        void         disconnect                    ();
        void         stop                          ();


    // Chat functions

        void         sendMessage                   (std::wstring sMessage);


    // SET functions

        void         setPushToTalkButtonAndVolume  (int iKey,              unsigned short int iVolume);
        void         setNewUserVolume              (std::string sUserName,  float fVolume);


    // GET functions

        float        getUserCurrentVolume          (std::string sUserName);
        std::string  getClientVersion              ();
        std::string  getUserName                   ();




    ~Controller();

private:


    NetworkService* pNetworkService;
    AudioService*   pAudioService;
};

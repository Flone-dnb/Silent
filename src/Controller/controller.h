#pragma once

// C++
#include <string>

class NetworkService;
class AudioService;
class MainWindow;

class Controller
{
public:

    Controller(MainWindow* pMainWindow);


    void connectTo(std::string adress, std::string port, std::string userName);
    void sendMessage(std::wstring message);

    void disconnect();
    void stop();


    // set
    void setPushToTalkButtonAndVolume(int iKey, unsigned short int volume);
    void setNewUserVolume(std::string userName, float fVolume);

    // get
    std::string getClientVersion();
    std::string getUserName();
    float getUserCurrentVolume(std::string userName);


    ~Controller();

private:

    NetworkService* pNetworkService;
    AudioService*   pAudioService;
};

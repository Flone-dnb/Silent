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

    std::string getClientVersion();
    std::string getUserName();

    void setPushToTalkButtonAndVolume(int iKey, unsigned short int volume);


    void connectTo(std::string adress, std::string port, std::string userName);

    void sendMessage(std::wstring message);

    void disconnect();

    void stop();

    ~Controller();

private:

    NetworkService* pNetworkService;
    AudioService*   pAudioService;
};

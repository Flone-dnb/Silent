#include "controller.h"

// Custom
#include "Model/AudioService/audioservice.h"
#include "Model/NetworkService/networkservice.h"

Controller::Controller(MainWindow* pMainWindow)
{
    pAudioService  = new AudioService(pMainWindow);
    pNetworkService = new NetworkService(pMainWindow, pAudioService);
    pAudioService->setNetworkService(pNetworkService);
}




std::string Controller::getClientVersion()
{
    return pNetworkService->getClientVersion();
}

std::string Controller::getUserName()
{
    return pNetworkService->getUserName();
}

void Controller::connectTo(std::string adress, std::string port, std::string userName)
{
    pNetworkService->start(adress,port,userName);
}

void Controller::setPushToTalkButtonAndVolume(int iKey, unsigned short int volume)
{
    pAudioService->setPushToTalkButtonAndVolume(iKey, volume);
}
void Controller::sendMessage(std::wstring message)
{
    pNetworkService->sendMessage(message);
}

void Controller::disconnect()
{
    pNetworkService->disconnect();
}

void Controller::stop()
{
    pNetworkService->stop();
}




Controller::~Controller()
{
    delete pNetworkService;
    delete pAudioService;
}

#include "controller.h"

// Custom
#include "../src/Model/AudioService/audioservice.h"
#include "../src/Model/NetworkService/networkservice.h"

Controller::Controller(MainWindow* pMainWindow)
{
    pAudioService   = new AudioService(pMainWindow);
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

float Controller::getUserCurrentVolume(std::string userName)
{
    return pAudioService->getUserCurrentVolume(userName);
}

void Controller::connectTo(std::string adress, std::string port, std::string userName)
{
    pNetworkService->start(adress,port,userName);
}

void Controller::setPushToTalkButtonAndVolume(int iKey, unsigned short int volume)
{
    pAudioService->setPushToTalkButtonAndVolume(iKey, volume);
}

void Controller::setNewUserVolume(std::string userName, float fVolume)
{
    pAudioService->setNewUserVolume(userName, fVolume);
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

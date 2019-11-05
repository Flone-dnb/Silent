#include "controller.h"

// Custom
#include "../src/Model/AudioService/audioservice.h"
#include "../src/Model/NetworkService/networkservice.h"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


Controller::Controller(MainWindow* pMainWindow)
{
    pAudioService    = new AudioService   (pMainWindow);
    pNetworkService  = new NetworkService (pMainWindow, pAudioService);

    pAudioService    ->setNetworkService  (pNetworkService);
}





std::string Controller::getClientVersion()
{
    return pNetworkService ->getClientVersion ();
}

std::string Controller::getUserName()
{
    return pNetworkService ->getUserName ();
}

unsigned short Controller::getPingNormalBelow()
{
    return pNetworkService ->getPingNormalBelow();
}

unsigned short Controller::getPingWarningBelow()
{
    return pNetworkService ->getPingWarningBelow();
}

float Controller::getUserCurrentVolume(std::string sUserName)
{
    return pAudioService ->getUserCurrentVolume (sUserName);
}

void Controller::connectTo(std::string sAdress, std::string sPort, std::string sUserName)
{
    pNetworkService ->start (sAdress, sPort, sUserName);
}

void Controller::setPushToTalkButtonAndVolume(int iKey, unsigned short int iVolume)
{
    pAudioService ->setPushToTalkButtonAndVolume (iKey, iVolume);
}

void Controller::setNewUserVolume(std::string sUserName, float fVolume)
{
    pAudioService ->setNewUserVolume (sUserName, fVolume);
}

void Controller::sendMessage(std::wstring sMessage)
{
    pNetworkService ->sendMessage (sMessage);
}

void Controller::disconnect()
{
    pNetworkService ->disconnect ();
}

void Controller::stop()
{
    pNetworkService ->stop ();
}





Controller::~Controller()
{
    delete pNetworkService;
    delete pAudioService;
}

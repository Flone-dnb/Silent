#include "controller.h"


// Custom
#include "../src/View/MainWindow/mainwindow.h"
#include "../src/Model/NetworkService/networkservice.h"
#include "../src/Model/AudioService/audioservice.h"
#include "../src/Model/SettingsManager/settingsmanager.h"
#include "../src/Model/SettingsManager/SettingsFile.h"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


Controller::Controller(MainWindow* pMainWindow)
{
    pSettingsManager = new SettingsManager (pMainWindow);
    if (pSettingsManager ->getCurrentSettings())
    {
        pAudioService    = new AudioService    (pMainWindow, pSettingsManager);
        pNetworkService  = new NetworkService  (pMainWindow, pAudioService, pSettingsManager);

        pAudioService    ->setNetworkService   (pNetworkService);
    }
    else
    {
        // The AudioService calls getSettings() very often.
        // If the SettingsManager returns nullptr then AudioService will crash
        // because it's not checking if the settings is nullptr.

        pMainWindow ->showMessageBox(true, "The application cannot continue execution.\n"
                                           "Please, tell the developers about this problem.\n"
                                           "You still can use menu item \"Help\" - \"About\" to get in touch with the devs.");
    }
}





std::string Controller::getClientVersion()
{
    return pNetworkService ->getClientVersion();
}

std::string Controller::getUserName()
{
    return pNetworkService ->getUserName();
}

unsigned short Controller::getPingNormalBelow()
{
    return pNetworkService ->getPingNormalBelow();
}

unsigned short Controller::getPingWarningBelow()
{
    return pNetworkService ->getPingWarningBelow();
}

SettingsFile *Controller::getCurrentSettingsFile()
{
    return pSettingsManager ->getCurrentSettings();
}

bool Controller::isSettingsCreatedFirstTime()
{
    return pSettingsManager ->isSettingsCreatedFirstTime();
}

float Controller::getUserCurrentVolume(std::string sUserName)
{
    return pAudioService ->getUserCurrentVolume(sUserName);
}

void Controller::connectTo(std::string sAdress, std::string sPort, std::string sUserName)
{
    pNetworkService ->start(sAdress, sPort, sUserName);
}

void Controller::saveSettings (SettingsFile* pSettingsFile)
{
    pSettingsManager ->saveSettings(pSettingsFile);
    pAudioService    ->setNewMasterVolume( pSettingsManager ->getCurrentSettings() ->iMasterVolume );
}

void Controller::setNewUserVolume(std::string sUserName, float fVolume)
{
    pAudioService ->setNewUserVolume(sUserName, fVolume);
}

void Controller::sendMessage(std::wstring sMessage)
{
    pNetworkService ->sendMessage(sMessage);
}

void Controller::disconnect()
{
    pNetworkService ->disconnect();
}

void Controller::stop()
{
    pNetworkService ->stop();
}





Controller::~Controller()
{
    delete pNetworkService;
    delete pAudioService;
}

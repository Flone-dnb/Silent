// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "controller.h"


// Custom
#include "View/MainWindow/mainwindow.h"
#include "Model/NetworkService/networkservice.h"
#include "Model/AudioService/audioservice.h"
#include "Model/SettingsManager/settingsmanager.h"
#include "Model/SettingsManager/SettingsFile.h"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


Controller::Controller(MainWindow* pMainWindow)
{
    pAudioService    = nullptr;
    pNetworkService  = nullptr;
    pSettingsManager = nullptr;


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

SListItemRoom *Controller::getCurrentUserRoom()
{
    return pNetworkService->getUserRoom();
}

float Controller::getUserCurrentVolume(std::string sUserName)
{
    return pAudioService ->getUserCurrentVolume(sUserName);
}

void Controller::connectTo(std::string sAdress, std::string sPort, std::string sUserName,  std::wstring sPass)
{
    pNetworkService ->start(sAdress, sPort, sUserName, sPass);
}

void Controller::saveSettings (SettingsFile* pSettingsFile)
{
    pSettingsManager ->saveSettings(pSettingsFile);

    if (pAudioService)
    {
        pAudioService ->setNewMasterVolume( pSettingsManager ->getCurrentSettings() ->iMasterVolume );
    }
}

void Controller::setNewUserVolume(std::string sUserName, float fVolume)
{
    pAudioService ->setNewUserVolume(sUserName, fVolume);
}

void Controller::sendMessage(std::wstring sMessage)
{
    pNetworkService ->sendMessage(sMessage);
}

void Controller::enterRoom(std::string sRoomName)
{
    pNetworkService->enterRoom(sRoomName);
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
    if (pAudioService)
    {
       delete pAudioService;
    }

    if (pNetworkService)
    {
       delete pNetworkService;
    }

    delete pSettingsManager;
}

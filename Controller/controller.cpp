#include "controller.h"

// Custom
#include "Model/NetworkService/networkservice.h"

Controller::Controller(MainWindow* pMainWindow)
{
    pNetworkService = new NetworkService(pMainWindow);
}




std::string Controller::getClientVersion()
{
    return pNetworkService->getClientVersion();
}

void Controller::connectTo(std::string adress, std::string port, std::string userName)
{
    pNetworkService->start(adress,port,userName);
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
}

#pragma once

// C++
#include <string>

class NetworkService;
class MainWindow;


class Controller
{
public:

    Controller(MainWindow* pMainWindow);

    void connectTo(std::string adress, std::string port, std::string userName);

    void sendMessage(std::wstring message);

    void disconnect();

    void stop();

    ~Controller();

private:

    NetworkService* pNetworkService;
};

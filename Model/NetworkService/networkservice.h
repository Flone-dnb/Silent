#pragma once

// C++
#include <string>
#include <mutex>

// ============== Network ==============
// Sockets and stuff
#include <winsock2.h>

// Adress translation
#include <ws2tcpip.h>

// Winsock 2 Library
#pragma comment(lib,"Ws2_32.lib")


class MainWindow;


class NetworkService
{
public:

    NetworkService(MainWindow* pMainWindow);

    void start(std::string adress, std::string port, std::string userName);

    void connectTo(std::string adress, std::string port, std::string userName, std::mutex& mtx);

    void listenForServer(std::mutex& mtx);

    void sendMessage(std::wstring message);

    void disconnect();

    void stop();

private:

    MainWindow* pMainWindow;

    SOCKET userSocket;

    std::string userName;

    std::mutex mtx;

    bool bWinSockLaunched;
    bool bListen;
};

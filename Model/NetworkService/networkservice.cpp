#include "networkservice.h"

// Custom
#include "View/MainWindow/mainwindow.h"

// C++
#include <thread>
#include <ctime>

NetworkService::NetworkService(MainWindow* pMainWindow)
{
    this->pMainWindow = pMainWindow;

    bWinSockLaunched = false;
    bListen          = false;
}

void NetworkService::start(std::string adress, std::string port, std::string userName)
{
    if (bListen)
    {
        disconnect();
    }

    pMainWindow->enableInteractiveElements(false,false);

    // Start WinSock2 (ver. 2.2)
    WSADATA WSAData;

    // This will hold return values from functions and check if there was an error.
    int returnCode = 0;

    returnCode = WSAStartup(MAKEWORD(2, 2), &WSAData);
    if (returnCode != 0)
    {
        pMainWindow->printOutput(std::string("NetworkService::start()::WSAStartup() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\nTry again.\n"));
    }
    else
    {
        bWinSockLaunched = true;

        // Create the IPv4 TCP socket
        userSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (userSocket == INVALID_SOCKET)
        {
            pMainWindow->printOutput(std::string("NetworkService::start()::socket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\nTry again.\n"));

            WSACleanup();
            bWinSockLaunched = false;
        }
        else
        {
            std::thread connectThread(&NetworkService::connectTo,this,adress,port,userName,ref(mtx));
            connectThread.detach();

            std::thread listenThread (&NetworkService::listenForServer,this,ref(mtx));
            listenThread.detach();
        }
    }
}

void NetworkService::connectTo(std::string adress, std::string port, std::string userName, std::mutex& mtx)
{
    mtx.lock();

    // This will hold return values from functions and check if there was an error.
    int returnCode = 0;

    // Fill sockaddr_in
    sockaddr_in sockaddrToConnect;
    memset(sockaddrToConnect.sin_zero, 0, sizeof(sockaddrToConnect.sin_zero));
    sockaddrToConnect.sin_family = AF_INET;
    // IP
    inet_pton(AF_INET, adress.c_str(), &sockaddrToConnect.sin_addr);
    // Port
    sockaddrToConnect.sin_port = htons(static_cast<USHORT>(stoi(port)));


    pMainWindow->printOutput(std::string("Connecting... (time out after 20 sec.)"),true);


    returnCode = connect(userSocket, reinterpret_cast<sockaddr*>(&sockaddrToConnect), sizeof(sockaddrToConnect));
    if (returnCode == SOCKET_ERROR)
    {
        if (WSAGetLastError() == 10060)
        {
            pMainWindow->printOutput(std::string("Time out.\nTry again.\n"),true);
            pMainWindow->enableInteractiveElements(true,false);
        }
        else
        {
            pMainWindow->printOutput(std::string("NetworkService::connectTo()::connect() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\nTry again.\n"),true);
            pMainWindow->enableInteractiveElements(true,false);
        }

        WSACleanup();
        bWinSockLaunched = false;
    }
    else
    {
        // Disable Nagle algorithm for connected socket
        BOOL bOptVal = true;
        int bOptLen = sizeof(BOOL);
        returnCode = setsockopt(userSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&bOptVal), bOptLen);
        if (returnCode == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("NetworkService::connectTo()::setsockopt (Nagle algorithm) failed and returned: "+std::to_string(WSAGetLastError())+".\nTry again.\n"),true);
            closesocket(userSocket);
            WSACleanup();
            bWinSockLaunched = false;
            pMainWindow->enableInteractiveElements(true,false);
        }
        else
        {
            // Send user name
            send(userSocket,userName.c_str(),static_cast<int>(userName.size()),0);

            char* pReadBuffer = new char[1400];
            memset(pReadBuffer,0,1400);

            int iReceivedSize = recv(userSocket,pReadBuffer,1,0);
            if (pReadBuffer[0] == 0)
            {
                // This user name is already in use... Disconnect

                if (recv(userSocket,pReadBuffer,1500,0) == 0)
                {
                    shutdown(userSocket,SD_SEND);
                }

                pMainWindow->printOutput(std::string("\nA user with this name is already present on the server. Select another name."),true);
                pMainWindow->enableInteractiveElements(true,false);
                closesocket(userSocket);
                WSACleanup();
                bWinSockLaunched = false;
            }
            else
            {
                // Receive packet size
                iReceivedSize = recv(userSocket,pReadBuffer,2,0);

                unsigned short int iPacketSize = 0;
                std::memcpy(&iPacketSize, pReadBuffer, 2);

                // Receive data
                iReceivedSize = recv(userSocket, pReadBuffer, iPacketSize, 0);
                pMainWindow->printOutput(std::string("Received " + std::to_string(iReceivedSize + 3) + " bytes of data from the server.\n"), true);

                // READ online info
                int iReadBytes = 0;

                int iOnline    = 0;

                std::memcpy(&iOnline,pReadBuffer,4);
                iReadBytes+=4;

                pMainWindow->setOnlineUsersCount(iOnline);

                for (int i = 0; i < (iOnline-1); i++)
                {
                    unsigned char currentItemSize = 0;
                    std::memcpy(&currentItemSize,pReadBuffer+iReadBytes,1);
                    iReadBytes++;

                    char* pRowText = new char[currentItemSize];
                    memset(pRowText,0,currentItemSize);
                    std::memcpy(pRowText,pReadBuffer+iReadBytes,currentItemSize);
                    iReadBytes+=currentItemSize;

                    pMainWindow->addNewUserToList(std::string(pRowText));
                    delete[] pRowText;
                }

                pMainWindow->addNewUserToList(userName);

                // Translate socket to non-blocking mode
                u_long arg = true;
                if (ioctlsocket(userSocket,FIONBIO,&arg) == SOCKET_ERROR)
                {
                    pMainWindow->printOutput(std::string("NetworkService::connectTo()::ioctsocket() (non-blocking mode) failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"),true);

                    closesocket(userSocket);
                    WSACleanup();

                    bWinSockLaunched = false;
                    pMainWindow->enableInteractiveElements(true,false);
                }

                this->userName = userName;
                pMainWindow->printOutput(std::string("Connected.\n==============   Welcome to chat!  ==============\n\nWARNING:\nThe data transmitted over the network is not encrypted.\n"),true);
                pMainWindow->enableInteractiveElements(true,true);

                bListen = true;
            }
            delete[] pReadBuffer;
        }
    }

    mtx.unlock();
}

void NetworkService::listenForServer(std::mutex& mtx)
{
    mtx.lock();
    mtx.unlock();

    char* pReadBuffer = new char[1500];

    while(bListen)
    {
        // We will check if there is a message every 10 ms (look end of this function)
        if (recv(userSocket,pReadBuffer,0,0) == 0)
        {
            // There are some data to read

            // There are some data to receive
            int receivedAmount = recv(userSocket, pReadBuffer, 1, 0);
            if (receivedAmount == 0)
            {
                // Server sent FIN
                bListen = false;

                pMainWindow->printOutput(std::string("Server is closing connection.\n"),true);
                int returnCode = shutdown(userSocket, SD_SEND);
                if (returnCode == SOCKET_ERROR)
                {
                     pMainWindow->printOutput(std::string("NetworkService::listenForServer()::shutdown() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"),true);
                     closesocket(userSocket);
                     WSACleanup();
                     bWinSockLaunched = false;
                }
                else
                {
                    returnCode = closesocket(userSocket);
                    if (returnCode == SOCKET_ERROR)
                    {
                        pMainWindow->printOutput(std::string("NetworkService::listenForServer()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"),true);
                        WSACleanup();
                        bWinSockLaunched = false;
                    }
                    else
                    {
                        if (WSACleanup() == SOCKET_ERROR)
                        {
                            pMainWindow->printOutput(std::string("NetworkService::listenForServer()::WSACleanup() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"),true);
                        }
                        else
                        {
                            pMainWindow->setOnlineUsersCount(0);
                            pMainWindow->enableInteractiveElements(true,false);

                            bWinSockLaunched = false;

                            pMainWindow->printOutput(std::string("Connection closed successfully!\n"),true);
                            pMainWindow->deleteUserFromList("",true);
                        }
                    }
                }
            }
            else
            {
                if (pReadBuffer[0] == 0)
                {
                    // We received info about new user
                    // Read packet size
                    unsigned char iPacketSize = 0;
                    receivedAmount = recv(userSocket, pReadBuffer, 1, 0);
                    std::memcpy(&iPacketSize, pReadBuffer, 1);

                    receivedAmount = recv(userSocket, pReadBuffer, iPacketSize, 0);

                    int iOnline = 0;
                    std::memcpy(&iOnline, pReadBuffer, 4);

                    unsigned char sizeOfNewUserName = 0;
                    std::memcpy(&sizeOfNewUserName, pReadBuffer + 4, 1);

                    char* pNewUserName = new char[sizeOfNewUserName + 1];
                    memset(pNewUserName, 0, sizeOfNewUserName + 1);
                    std::memcpy(pNewUserName, pReadBuffer + 5, sizeOfNewUserName);

                    pMainWindow->setOnlineUsersCount(iOnline);
                    pMainWindow->addNewUserToList(std::string(pNewUserName));

                    delete[] pNewUserName;
                }
                else if (pReadBuffer[0] == 1)
                {
                    // Someone disconnected

                    // Read packet size
                    receivedAmount = recv(userSocket, pReadBuffer, 1, 0);
                    unsigned char iPacketSize = 0;
                    std::memcpy(&iPacketSize, pReadBuffer, 1);

                    receivedAmount = recv(userSocket, pReadBuffer, iPacketSize, 0);

                    int iOnline = 0;

                    std::memcpy(&iOnline, pReadBuffer, 4);
                    pMainWindow->setOnlineUsersCount(iOnline);

                    // Max user name length = 20 (in ConnectWindow textEdit element) + 1 for null-terminated char
                    char* pUserName = new char[21];
                    memset(pUserName, 0, 21);

                    std::memcpy(pUserName, pReadBuffer + 4, iPacketSize - 4);

                    pMainWindow->deleteUserFromList(std::string(pUserName));

                    delete[] pUserName;
                }
                else if (pReadBuffer[0] == 10)
                {
                    // It's a message
                    // Read size
                    unsigned short int iPacketSize = 0;
                    receivedAmount = recv(userSocket, pReadBuffer, 2, 0);
                    std::memcpy(&iPacketSize, pReadBuffer, 2);

                    // Receive message
                    receivedAmount = recv(userSocket, pReadBuffer, iPacketSize, 0);

                    // Message structure: "Hour:Minute. UserName: Message (in wchar_t*)".

                    // Read time data:
                    int iFirstWCharPos  = 0;
                    bool bFirstColonWas = false;
                    for (int j = 0; j < receivedAmount; j++)
                    {
                        if ( (pReadBuffer[j] == ':') && (!bFirstColonWas) )
                        {
                            bFirstColonWas = true;
                        }
                        else if ( (pReadBuffer[j] == ':') && (bFirstColonWas) )
                        {
                            iFirstWCharPos = j + 2;
                            break;
                        }
                    }

                    // Copy time info to pTimeText
                    char* pTimeText = new char[static_cast<unsigned long long>(iFirstWCharPos + 1)];
                    memset(pTimeText, 0, static_cast<unsigned long long>(iFirstWCharPos + 1));
                    std::memcpy(pTimeText, pReadBuffer, static_cast<unsigned long long>(iFirstWCharPos));

                    // Copy message to pWChatText
                    wchar_t* pWCharText = new wchar_t[static_cast<unsigned long long>(receivedAmount - iFirstWCharPos + 2)];
                    memset(pWCharText, 0, static_cast<unsigned long long>(receivedAmount - iFirstWCharPos + 2));
                    std::memcpy(pWCharText, pReadBuffer + iFirstWCharPos, static_cast<unsigned long long>(receivedAmount - iFirstWCharPos));

                    // Show data on screen
                    pMainWindow->printUserMessage(std::string(pTimeText), std::wstring(pWCharText), true);

                    delete[] pTimeText;
                    delete[] pWCharText;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    delete[] pReadBuffer;
}

void NetworkService::sendMessage(std::wstring message)
{
    if (message.size() > 550) pMainWindow->showMessageBox(0, "Your message is too big!");
    else
    {
        // Get local time to pCurrentTime
        time_t seconds = time(NULL);
        tm* timeinfo = localtime(&seconds);

        // Create string to send in format: "Hour:Minute. UserName: Message".
        std::string timeString = "";
        if (std::to_string(timeinfo->tm_hour).size() == 1)
        {
            timeString += "0";
        }
        timeString += std::to_string(timeinfo->tm_hour);
        timeString += ":";
        if (std::to_string(timeinfo->tm_min).size() == 1)
        {
            timeString += "0";
        }
        timeString += std::to_string(timeinfo->tm_min);
        timeString += ". ";
        timeString += userName;
        timeString += ": ";

        char* pSend = new char[1400];
        memset(pSend,0,1400);

        unsigned short int iPacketSize = static_cast<unsigned short>( timeString.size() + (message.size() * 2) );

        // 10 == message
        unsigned char commandType = 10;
        std::memcpy(pSend, &commandType, 1);
        std::memcpy(pSend + 1, &iPacketSize, 2);
        std::memcpy(pSend + 3, timeString.c_str(), timeString.size());

        std::memcpy(pSend + 3 + timeString.size(), message.c_str(), message.size() * 2);

        int sendSize = send(userSocket,pSend,static_cast<int>(1 + 2 + timeString.size() + (message.size() * 2)), 0);
        if ( sendSize != static_cast<int>(1 + 2 + timeString.size() + (message.size() * 2)) )
        {
            if (sendSize == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("\nWARNING:\nYour message has not been sent!\n"
                                                     "NetworkService::sendMessage()::send() failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"));
            }
            else
            {
                pMainWindow->printOutput(std::string("\nWARNING:\nWe could not send the whole message, because not enough space in the outgoing socket buffer.\n"));

                if (sendSize > static_cast<int>(timeString.size()))
                {
                    pMainWindow->printUserMessage(timeString,message.substr(0,static_cast<size_t>(sendSize - static_cast<int>(timeString.size()))));
                }
                else
                {
                    pMainWindow->printUserMessage(timeString.substr(0,static_cast<size_t>(sendSize)),L"");
                }

                pMainWindow->clearTextEdit();
            }
        }
        else
        {
            pMainWindow->printUserMessage(timeString,message);
            pMainWindow->clearTextEdit();
        }

        delete[] pSend;
    }
}

void NetworkService::disconnect()
{
    if (bListen)
    {
        bListen = false;
        char* pReadBuffer = new char[1500];

        // Send FIN
        int returnCode = shutdown(userSocket, SD_SEND);
        if (returnCode == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("NetworkService::disconnect()::shutdown() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"));
            closesocket(userSocket);
            WSACleanup();
            bWinSockLaunched = false;
        }
        else
        {
            // Translate socket to blocking mode
            u_long arg = false;
            if (ioctlsocket(userSocket,FIONBIO,&arg) == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("NetworkService::disconnect()::ioctsocket() (blocking mode) failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"));
                closesocket(userSocket);
                WSACleanup();
                bWinSockLaunched = false;
            }

            if (recv(userSocket, pReadBuffer, 1500, 0) == 0)
            {
                returnCode = closesocket(userSocket);
                if (returnCode == SOCKET_ERROR)
                {
                    pMainWindow->printOutput(std::string("NetworkService::disconnect()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"));
                    WSACleanup();
                    bWinSockLaunched = false;
                }
                else
                {
                    pMainWindow->printOutput(std::string("Connection closed successfully.\n"));
                    if (WSACleanup() == SOCKET_ERROR)
                    {
                        pMainWindow->printOutput(std::string("NetworkService::disconnect()::WSACleanup() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"));
                    }
                    else
                    {
                        pMainWindow->deleteUserFromList("",true);
                        pMainWindow->setOnlineUsersCount(0);
                        pMainWindow->enableInteractiveElements(true,false);

                        bWinSockLaunched = false;
                    }
                }
            }
            else
            {
                pMainWindow->printOutput(std::string("Server has not responded.\n"));

                closesocket(userSocket);
                WSACleanup();
                bWinSockLaunched = false;

                pMainWindow->deleteUserFromList("",true);
                pMainWindow->setOnlineUsersCount(0);
                pMainWindow->enableInteractiveElements(true,false);
            }
        }

        delete[] pReadBuffer;
    }
    else
    {
        pMainWindow->showMessageBox(0,std::string("You are not connected."));
    }
}

void NetworkService::stop()
{
    if (bListen)
    {
        disconnect();
    }
}

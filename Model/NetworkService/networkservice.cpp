#include "networkservice.h"

// Custom
#include "View/MainWindow/mainwindow.h"

// C++
#include <thread>
#include <ctime>

NetworkService::NetworkService(MainWindow* pMainWindow)
{
    clientVersion = "1.15";

    this->pMainWindow = pMainWindow;

    bWinSockLaunched = false;
    bListen          = false;
}





std::string NetworkService::getClientVersion()
{
    return clientVersion;
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
            std::thread connectThread(&NetworkService::connectTo, this, adress, port, userName, ref(mtx));
            connectThread.detach();

            std::thread listenThread (&NetworkService::listenForServer, this, ref(mtx));
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


    pMainWindow->printOutput(std::string("Connecting... (time out after 20 sec.)"), true);


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
            // Send version & user name
            char versionAndNameBuffer[41];
            memset(versionAndNameBuffer, 0, 41);

            char versionStringSize = static_cast<char>(clientVersion.size());
            versionAndNameBuffer[0] = versionStringSize;

            std::memcpy(versionAndNameBuffer + 1, clientVersion.c_str(), static_cast<unsigned long long>(versionStringSize));
            std::memcpy(versionAndNameBuffer + 1 + versionStringSize, userName.c_str(), userName.size());

            send(userSocket, versionAndNameBuffer, static_cast<int>(1 + versionStringSize + userName.size()), 0);



            char readBuffer[1400];
            memset(readBuffer, 0, 1400);

            int iReceivedSize = recv(userSocket, readBuffer, 1, 0);
            if (readBuffer[0] == 0)
            {
                // This user name is already in use... Disconnect

                if (recv(userSocket, readBuffer, 1, 0) == 0)
                {
                    shutdown(userSocket,SD_SEND);
                }

                pMainWindow->printOutput(std::string("\nA user with this name is already present on the server. Select another name."), true);
                pMainWindow->enableInteractiveElements(true, false);
                closesocket(userSocket);
                WSACleanup();
                bWinSockLaunched = false;
            }
            else if (readBuffer[0] == 2)
            {
                // Server is full

                if (recv(userSocket, readBuffer, 1, 0) == 0)
                {
                    shutdown(userSocket,SD_SEND);
                }

                pMainWindow->printOutput(std::string("\nServer is full."), true);
                pMainWindow->enableInteractiveElements(true, false);
                closesocket(userSocket);
                WSACleanup();
                bWinSockLaunched = false;
            }
            else if (readBuffer[0] == 3)
            {
                char serverVersionSize = 0;
                recv(userSocket, &serverVersionSize, 1, 0);

                char versionBuffer[21];
                memset(versionBuffer, 0, 21);

                recv(userSocket, versionBuffer, 20, 0);

                if (recv(userSocket, readBuffer, 1, 0) == 0)
                {
                    shutdown(userSocket,SD_SEND);
                }

                pMainWindow->printOutput(std::string("\nYour FChat version (" + clientVersion + ") does not match with the server version (" + std::string(versionBuffer) + ").\n"
                                                                          "Please update your FChat Client to version " + std::string(versionBuffer) + "."), true);
                pMainWindow->enableInteractiveElements(true, false);
                closesocket(userSocket);
                WSACleanup();
                bWinSockLaunched = false;
            }
            else
            {
                // Receive packet size
                iReceivedSize = recv(userSocket, readBuffer, 2, 0);

                unsigned short int iPacketSize = 0;
                std::memcpy(&iPacketSize, readBuffer, 2);

                // Receive data
                iReceivedSize = recv(userSocket, readBuffer, iPacketSize, 0);
                pMainWindow->printOutput(std::string("Received " + std::to_string(iReceivedSize + 3) + " bytes of data from the server.\n"), true);

                // READ online info
                int iReadBytes = 0;
                int iOnline    = 0;

                std::memcpy(&iOnline, readBuffer, 4);
                iReadBytes += 4;

                pMainWindow->setOnlineUsersCount(iOnline);

                for (int i = 0; i < (iOnline - 1); i++)
                {
                    unsigned char currentItemSize = 0;
                    std::memcpy(&currentItemSize, readBuffer + iReadBytes, 1);
                    iReadBytes++;

                    // 20 - max user name (in ConnectWindow textEdit element) + 1 null terminator char
                    char rowText[21];
                    memset(rowText, 0, 21);

                    std::memcpy(rowText, readBuffer + iReadBytes, currentItemSize);
                    iReadBytes += currentItemSize;

                    pMainWindow->addNewUserToList(std::string(rowText));
                }

                pMainWindow->addNewUserToList(userName);

                // Translate socket to non-blocking mode
                u_long arg = true;
                if (ioctlsocket(userSocket, FIONBIO, &arg) == SOCKET_ERROR)
                {
                    pMainWindow->printOutput(std::string("NetworkService::connectTo()::ioctsocket() (non-blocking mode) failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"),true);

                    closesocket(userSocket);
                    WSACleanup();

                    bWinSockLaunched = false;
                    pMainWindow->enableInteractiveElements(true, false);
                }

                this->userName = userName;
                pMainWindow->printOutput(std::string("WARNING:\nThe data transmitted over the network is not encrypted.\n\nConnected.\n"),true);
                pMainWindow->enableInteractiveElements(true, true);

                bListen = true;
            }
        }
    }

    mtx.unlock();
}

void NetworkService::listenForServer(std::mutex& mtx)
{
    mtx.lock();
    mtx.unlock();

    char readBuffer[1];

    while(bListen)
    {
        while (recv(userSocket, readBuffer, 0, 0) == 0)
        {
            // There are some data to read

            // There are some data to receive
            int receivedAmount = recv(userSocket, readBuffer, 1, 0);
            if (receivedAmount == 0)
            {
                // Server sent FIN

                answerToFIN();
            }
            else
            {
                if (readBuffer[0] == 0)
                {
                    // We received info about the new user

                    receiveInfoAboutNewUser();
                }
                else if (readBuffer[0] == 1)
                {
                    // Someone disconnected

                    deleteDisconnectedUserFromList();
                }
                else if (readBuffer[0] == 9)
                {
                    // This is keep-alive message
                    // We've been idle for 30 seconds
                    // We should answer in 10 seconds or will be disconnected

                    char keepAliveChar = 9;
                    send(userSocket, &keepAliveChar, 1, 0);
                }
                else if (readBuffer[0] == 10)
                {
                    // It's a message

                    receiveMessage();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

}

void NetworkService::receiveInfoAboutNewUser()
{
    char readBuffer[31];
    memset(readBuffer, 0, 31);

    // Read packet size
    unsigned char iPacketSize = 0;
    recv(userSocket, readBuffer, 1, 0);
    std::memcpy(&iPacketSize, readBuffer, 1);

    // Read other stuff
    recv(userSocket, readBuffer, iPacketSize, 0);
    int iOnline = 0;
    std::memcpy(&iOnline, readBuffer, 4);

//    unsigned char sizeOfNewUserName = 0;
//    std::memcpy(&sizeOfNewUserName, readBuffer + 4, 1);

//    // 20 - max user name (in ConnectWindow textEdit element) + 1 null terminator char
//    char newUserName[21];
//    memset(newUserName, 0, 21);
//    std::memcpy(newUserName, readBuffer + 5, sizeOfNewUserName);

    pMainWindow->setOnlineUsersCount(iOnline);
    pMainWindow->addNewUserToList(std::string(readBuffer + 5));
}

void NetworkService::receiveMessage()
{
    // Read packet size
    unsigned short int iPacketSize = 0;
    recv(userSocket, reinterpret_cast<char*>(&iPacketSize), 2, 0);

    char* pReadBuffer = new char[iPacketSize + 2];
    memset(pReadBuffer, 0, iPacketSize + 2);

    // Receive message
    int receivedAmount = recv(userSocket, pReadBuffer, iPacketSize, 0);

    // Message structure: "Hour:Minute. UserName: Message (message in wchar_t)".

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

    // Copy time info to 'timeText'
    // timeText = time info + user name
    // Max user name size = 20 + ~ max 7 chars before user name
    char timeText[31];
    memset(timeText, 0, 31);
    std::memcpy(timeText, pReadBuffer, static_cast<unsigned long long>(iFirstWCharPos));

    // Copy message to pWChatText
    wchar_t* pWCharText = new wchar_t[static_cast<unsigned long long>(receivedAmount - iFirstWCharPos + 2)];
    memset(pWCharText, 0, static_cast<unsigned long long>(receivedAmount - iFirstWCharPos + 2));
    std::memcpy(pWCharText, pReadBuffer + iFirstWCharPos, static_cast<unsigned long long>(receivedAmount - iFirstWCharPos));

    // Show data on screen
    pMainWindow->printUserMessage("\n" + std::string(timeText), std::wstring(pWCharText), true);

    delete[] pWCharText;
    delete[] pReadBuffer;
}

void NetworkService::deleteDisconnectedUserFromList()
{
    char readBuffer[31];
    memset(readBuffer, 0, 31);

    // Read packet size
    recv(userSocket, readBuffer, 1, 0);
    unsigned char iPacketSize = 0;
    std::memcpy(&iPacketSize, readBuffer, 1);

    // Read other stuff
    recv(userSocket, readBuffer, iPacketSize, 0);
    int iOnline = 0;

    std::memcpy(&iOnline, readBuffer, 4);
    pMainWindow->setOnlineUsersCount(iOnline);

//    // Max user name length = 20 (in ConnectWindow textEdit element) + 1 for null terminator char
//    char userName[21];
//    memset(userName, 0, 21);

//    std::memcpy(userName, readBuffer + 4, iPacketSize - 4);

    pMainWindow->deleteUserFromList(std::string(readBuffer + 4));
}

void NetworkService::sendMessage(std::wstring message)
{
    if (message.size() > 550) pMainWindow->showMessageBox(0, "Your message is too big!");
    else
    {
        char* pSendBuffer = new char[3 + (message.size() * 2) + 2];
        memset(pSendBuffer, 0, 3 + (message.size() * 2) + 2);

        unsigned short int iPacketSize = static_cast<unsigned short>( message.size() * 2 );

        // 10 == message
        unsigned char commandType = 10;
        std::memcpy(pSendBuffer, &commandType, 1);
        std::memcpy(pSendBuffer + 1, &iPacketSize, 2);

        std::memcpy(pSendBuffer + 3, message.c_str(), message.size() * 2);

        int sendSize = send(  userSocket, pSendBuffer, static_cast<int>( 3 + (message.size() * 2) ), 0  );
        if ( sendSize != static_cast<int>(3 + (message.size() * 2)) )
        {
            if (sendSize == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("\nWARNING:\nYour message has not been sent!\n"
                                                     "NetworkService::sendMessage()::send() failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"));
            }
            else
            {
                pMainWindow->printOutput(std::string("\nWARNING:\nWe could not send the whole message, because not enough space in the outgoing socket buffer.\n"));

                pMainWindow->clearTextEdit();
            }
        }
        else
        {
            pMainWindow->clearTextEdit();
        }

        delete[] pSendBuffer;
    }
}

void NetworkService::disconnect()
{
    if (bListen)
    {
        bListen = false;
        char readBuffer[5];

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

            if (recv(userSocket, readBuffer, 5, 0) == 0)
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
                        pMainWindow->deleteUserFromList("", true);
                        pMainWindow->setOnlineUsersCount(0);
                        pMainWindow->enableInteractiveElements(true, false);

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
    }
    else
    {
        pMainWindow->showMessageBox(0, std::string("You are not connected.") );
    }
}

void NetworkService::answerToFIN()
{
    bListen = false;

    pMainWindow->printOutput(std::string("Server is closing connection.\n"),true);
    int returnCode = shutdown(userSocket, SD_SEND);
    if (returnCode == SOCKET_ERROR)
    {
         pMainWindow->printOutput(std::string("NetworkService::listenForServer()::shutdown() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"), true);
         closesocket(userSocket);
         WSACleanup();
         bWinSockLaunched = false;
    }
    else
    {
        returnCode = closesocket(userSocket);
        if (returnCode == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("NetworkService::listenForServer()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"), true);
            WSACleanup();
            bWinSockLaunched = false;
        }
        else
        {
            if (WSACleanup() == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("NetworkService::listenForServer()::WSACleanup() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"), true);
            }
            else
            {
                pMainWindow->setOnlineUsersCount(0);
                pMainWindow->enableInteractiveElements(true, false);

                bWinSockLaunched = false;

                pMainWindow->printOutput(std::string("Connection closed successfully!\n"), true);
                pMainWindow->deleteUserFromList("", true);
            }
        }
    }
}





void NetworkService::stop()
{
    if (bListen)
    {
        disconnect();
    }
}

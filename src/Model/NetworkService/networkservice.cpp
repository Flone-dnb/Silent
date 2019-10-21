#include "networkservice.h"

// Custom
#include "../src/View/MainWindow/mainwindow.h"
#include "../src/Model/AudioService/audioservice.h"
#include "../src/globalparams.h"

// C++
#include <thread>

NetworkService::NetworkService(MainWindow* pMainWindow, AudioService* pAudioService)
{
    clientVersion = "2.16.1";

    this->pMainWindow   = pMainWindow;
    this->pAudioService = pAudioService;
    userName = "";

    bWinSockLaunched = false;
    bTextListen      = false;
    bVoiceListen     = false;
}





std::string NetworkService::getClientVersion()
{
    return clientVersion;
}

std::string NetworkService::getUserName()
{
    return userName;
}

void NetworkService::start(std::string adress, std::string port, std::string userName)
{
    if (bTextListen)
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
        userTCPSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (userTCPSocket == INVALID_SOCKET)
        {
            pMainWindow->printOutput(std::string("NetworkService::start()::socket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\nTry again.\n"));

            WSACleanup();
            bWinSockLaunched = false;
        }
        else
        {
            std::thread connectThread(&NetworkService::connectTo, this, adress, port, userName);
            connectThread.detach();
        }
    }
}

void NetworkService::connectTo(std::string adress, std::string port, std::string userName)
{
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


    memset(serverAddr.sin_zero, 0, sizeof(serverAddr.sin_zero));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<USHORT>(stoi(port)));
    inet_pton(AF_INET, adress.c_str(), &serverAddr.sin_addr);


    pMainWindow->printOutput(std::string("Connecting... (time out after 20 sec.)"), true);


    returnCode = connect(userTCPSocket, reinterpret_cast<sockaddr*>(&sockaddrToConnect), sizeof(sockaddrToConnect));
    if (returnCode == SOCKET_ERROR)
    {
        returnCode = WSAGetLastError();
        if (returnCode == 10060)
        {
            pMainWindow->printOutput(std::string("Time out.\nTry again.\n"),true);
            pMainWindow->enableInteractiveElements(true,false);
        }
        else if (returnCode == 10051)
        {
            pMainWindow->printOutput(std::string("NetworkService::connectTo()::connect() function failed and returned: " + std::to_string(returnCode) + ".\nPossible cause: no internet.\nTry again.\n"),true);
            pMainWindow->enableInteractiveElements(true,false);
        }
        else
        {
            pMainWindow->printOutput(std::string("NetworkService::connectTo()::connect() function failed and returned: " + std::to_string(returnCode) + ".\nTry again.\n"),true);
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
        returnCode = setsockopt(userTCPSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&bOptVal), bOptLen);
        if (returnCode == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("NetworkService::connectTo()::setsockopt (Nagle algorithm) failed and returned: "+std::to_string(WSAGetLastError())+".\nTry again.\n"),true);
            closesocket(userTCPSocket);
            WSACleanup();
            bWinSockLaunched = false;
            pMainWindow->enableInteractiveElements(true,false);
        }
        else
        {
            // Send version & user name
            char versionAndNameBuffer[MAX_NAME_LENGTH + MAX_VERSION_STRING_LENGTH + 1];
            memset(versionAndNameBuffer, 0, MAX_NAME_LENGTH + MAX_VERSION_STRING_LENGTH + 1);

            char versionStringSize = static_cast<char>(clientVersion.size());
            versionAndNameBuffer[0] = versionStringSize;

            std::memcpy(versionAndNameBuffer + 1, clientVersion.c_str(), static_cast<unsigned long long>(versionStringSize));
            std::memcpy(versionAndNameBuffer + 1 + versionStringSize, userName.c_str(), userName.size());

            send(userTCPSocket, versionAndNameBuffer, 1 + versionStringSize + static_cast<int>(userName.size()), 0);



            char readBuffer[MAX_BUFFER_SIZE];
            memset(readBuffer, 0, MAX_BUFFER_SIZE);

            int iReceivedSize = recv(userTCPSocket, readBuffer, 1, 0);
            if (readBuffer[0] == 0)
            {
                // This user name is already in use... Disconnect

                if (recv(userTCPSocket, readBuffer, 1, 0) == 0)
                {
                    shutdown(userTCPSocket,SD_SEND);
                }

                pMainWindow->printOutput(std::string("\nA user with this name is already present on the server. Select another name."), true);
                pMainWindow->enableInteractiveElements(true, false);
                closesocket(userTCPSocket);
                WSACleanup();
                bWinSockLaunched = false;
            }
            else if (readBuffer[0] == 2)
            {
                // Server is full

                if (recv(userTCPSocket, readBuffer, 1, 0) == 0)
                {
                    shutdown(userTCPSocket,SD_SEND);
                }

                pMainWindow->printOutput(std::string("\nServer is full."), true);
                pMainWindow->enableInteractiveElements(true, false);
                closesocket(userTCPSocket);
                WSACleanup();
                bWinSockLaunched = false;
            }
            else if (readBuffer[0] == 3)
            {
                // Wrong client version

                char serverVersionSize = 0;
                recv(userTCPSocket, &serverVersionSize, 1, 0);

                char versionBuffer[MAX_VERSION_STRING_LENGTH + 1];
                memset(versionBuffer, 0, MAX_VERSION_STRING_LENGTH + 1);

                recv(userTCPSocket, versionBuffer, MAX_VERSION_STRING_LENGTH, 0);

                if (recv(userTCPSocket, readBuffer, 1, 0) == 0)
                {
                    shutdown(userTCPSocket,SD_SEND);
                }

                pMainWindow->printOutput(std::string("\nYour FChat version (" + clientVersion + ") does not match with the server version (" + std::string(versionBuffer) + ").\n"
                                                                          "Please update your FChat Client to version " + std::string(versionBuffer) + "."), true);
                pMainWindow->enableInteractiveElements(true, false);
                closesocket(userTCPSocket);
                WSACleanup();
                bWinSockLaunched = false;
            }
            else
            {
                // Receive packet size
                iReceivedSize = recv(userTCPSocket, readBuffer, 2, 0);

                unsigned short int iPacketSize = 0;
                std::memcpy(&iPacketSize, readBuffer, 2);

                // Receive data
                iReceivedSize = recv(userTCPSocket, readBuffer, iPacketSize, 0);
                pMainWindow->printOutput(std::string("Received " + std::to_string(iReceivedSize + 3) + " bytes of data from the server.\n"), true);

                pAudioService->prepareForStart();

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

                    char rowText[MAX_NAME_LENGTH + 1];
                    memset(rowText, 0, MAX_NAME_LENGTH + 1);

                    std::memcpy(rowText, readBuffer + iReadBytes, currentItemSize);
                    iReadBytes += currentItemSize;

                    pMainWindow->addNewUserToList(std::string(rowText));
                    pAudioService->addNewUser(std::string(rowText));
                }

                pMainWindow->addNewUserToList(userName);

                // Translate socket to non-blocking mode
                u_long arg = true;
                if (ioctlsocket(userTCPSocket, static_cast<long>(FIONBIO), &arg) == SOCKET_ERROR)
                {
                    pMainWindow->printOutput(std::string("NetworkService::connectTo()::ioctsocket() (non-blocking mode) failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"),true);

                    closesocket(userTCPSocket);
                    WSACleanup();

                    bWinSockLaunched = false;
                    pMainWindow->enableInteractiveElements(true, false);
                }

                this->userName = userName;
                pMainWindow->printOutput(std::string("WARNING:\nThe data transmitted over the network is not encrypted.\n\nConnected to text chat."),true);
                pMainWindow->enableInteractiveElements(true, true);

                bTextListen = true;

                std::thread listenTextThread (&NetworkService::listenTCPFromServer, this);
                listenTextThread.detach();

                // We can start voice connection
                setupVoiceConnection();

                pMainWindow->saveUserName(userName);

                lastTimeServerKeepAliveCame = clock();
                std::thread monitor(&NetworkService::serverMonitor, this);
                monitor.detach();
            }
        }
    }
}

void NetworkService::setupVoiceConnection()
{
    userUDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (userUDPSocket == INVALID_SOCKET)
    {
        pMainWindow->printOutput( std::string("Cannot start voice connection.\n"
                                              "NetworkService::setupVoiceConnection::socket() error: ") + std::to_string(WSAGetLastError()), true );
        return;
    }
    else
    {
        // For a connectionless socket (for example, type SOCK_DGRAM), the operation performed by connect is merely to establish a default destination address
        // that can be used on subsequent send/ WSASend and recv/ WSARecv calls. Any datagrams received from an address other than the destination address specified will be discarded.
        if (connect(userUDPSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
        {
            pMainWindow->printOutput( std::string("Cannot start voice connection.\n"
                                                  "NetworkService::setupVoiceConnection::connect() error: ") + std::to_string(WSAGetLastError()), true );
            closesocket(userUDPSocket);
            return;
        }
        else
        {
            char firstMessage[MAX_NAME_LENGTH + 3];
            firstMessage[0] = -1;
            firstMessage[1] = static_cast<char>(userName.size());
            std::memcpy(firstMessage + 2, userName.c_str(), userName.size());

            int iSentSize = sendto(userUDPSocket, firstMessage, 2 + static_cast<int>(userName.size()), 0, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
            if ( iSentSize != (2 + static_cast<int>(userName.size())) )
            {
                if (iSentSize == SOCKET_ERROR)
                {
                    pMainWindow->printOutput( std::string("Cannot start voice connection.\n"
                                                          "NetworkService::setupVoiceConnection::sendto() error: ") + std::to_string(WSAGetLastError()), true );
                    closesocket(userUDPSocket);
                    return;
                }
                else
                {
                    pMainWindow->printOutput( std::string("Cannot start voice connection.\n"
                                                          "NetworkService::setupVoiceConnection::sendto() sent only: " + std::to_string(iSentSize) + " out of " + std::to_string(2 + userName.size())), true );
                    closesocket(userUDPSocket);
                    return;
                }
            }

            // Translate socket to non-blocking mode
            u_long arg = true;
            if (ioctlsocket(userUDPSocket, static_cast<long>(FIONBIO),&arg) == SOCKET_ERROR)
            {
                pMainWindow->printOutput( std::string("Cannot start voice connection.\n"
                                                      "NetworkService::setupVoiceConnection::ioctlsocket() error: ") + std::to_string(WSAGetLastError()), true );
                closesocket(userUDPSocket);
                return;
            }
        }
    }
}

void NetworkService::serverMonitor()
{
    do
    {
        clock_t timePassed = clock() - lastTimeServerKeepAliveCame;
        float timePassedInSeconds = static_cast<float>(timePassed)/CLOCKS_PER_SEC;

        if (timePassedInSeconds > (INTERVAL_KEEPALIVE_SEC * 2))
        {
            lostConnection();
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    } while (bTextListen);
}

void NetworkService::listenTCPFromServer()
{
    char readBuffer[1];

    while(bTextListen)
    {
        while (recv(userTCPSocket, readBuffer, 0, 0) == 0)
        {
            // There are some data to read

            // There are some data to receive
            int receivedAmount = recv(userTCPSocket, readBuffer, 1, 0);
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
                else if (readBuffer[0] == 2)
                {
                    std::thread listenVoiceThread (&NetworkService::listenUDPFromServer, this);
                    listenVoiceThread.detach();
                }
                else if (readBuffer[0] == 8)
                {
                    // It's ping
                    receivePing();
                }
                else if (readBuffer[0] == 9)
                {
                    // This is keep-alive message
                    // We've been idle for INTERVAL_KEEPALIVE_SEC seconds
                    // We should answer in 10 seconds or will be disconnected

                    char keepAliveChar = 9;
                    send(userTCPSocket, &keepAliveChar, 1, 0);

                    lastTimeServerKeepAliveCame = clock();
                }
                else if (readBuffer[0] == 10)
                {
                    // It's a message

                    receiveMessage();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_TCP_MESSAGE_MS));
    }
}

void NetworkService::listenUDPFromServer()
{
    if ( pAudioService->start() )
    {
        pMainWindow->printOutput(std::string("Connected to voice chat.\n"), true);
        bVoiceListen = true;
    }
    else
    {
        pMainWindow->printOutput( std::string("An error occurred while starting the voice chat.\n"), true );
        return;
    }

    char readBuffer[MAX_BUFFER_SIZE];

    while (bVoiceListen)
    {
        int iSize = recv(userUDPSocket, readBuffer, MAX_BUFFER_SIZE, 0);
        while (iSize > 0)
        {
            if (readBuffer[0] == 0)
            {
                // it's ping check
                iSize = send(userUDPSocket, readBuffer, 5, 0);
                if (iSize != 5)
                {
                    if (iSize == SOCKET_ERROR)
                    {
                        pMainWindow->printOutput(std::string("\nWARNING:\nNetworkService::listenUDPFromServer::sendto() failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"), true);
                    }
                    else
                    {
                        pMainWindow->printOutput(std::string("\nWARNING:\nSomething went wrong and your ping check wasn't sent fully.\n"), true);
                    }
                }
            }
            else
            {
                char userNameBuffer[MAX_NAME_LENGTH + 1];
                memset(userNameBuffer, 0, MAX_NAME_LENGTH + 1);
                std::memcpy(userNameBuffer, readBuffer + 1, static_cast<size_t>(readBuffer[0]));

                if ( readBuffer[1 + readBuffer[0]] == 1 )
                {
                    // Last audio packet
                    std::thread lastVoiceThread(&AudioService::playAudioData, pAudioService, nullptr, std::string(userNameBuffer), true);
                    lastVoiceThread.detach();
                }
                else
                {
                    short int* pAudio = new short int[ static_cast<size_t>( (iSize - 2 - readBuffer[0]) / 2 ) ];
                    std::memcpy( pAudio, readBuffer + 2 + readBuffer[0], static_cast<size_t>(iSize - 2 - readBuffer[0]) );

                    std::thread voiceThread(&AudioService::playAudioData, pAudioService, pAudio, std::string(userNameBuffer), false);
                    voiceThread.detach();
                }
            }


            iSize = recv(userUDPSocket, readBuffer, MAX_BUFFER_SIZE, 0);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_UDP_MESSAGE_MS));
    }
}

void NetworkService::receiveInfoAboutNewUser()
{
    char readBuffer[MAX_NAME_LENGTH + 11];
    memset(readBuffer, 0, 31);

    // Read packet size
    unsigned char iPacketSize = 0;
    recv(userTCPSocket, readBuffer, 1, 0);
    std::memcpy(&iPacketSize, readBuffer, 1);

    // Read other stuff
    recv(userTCPSocket, readBuffer, iPacketSize, 0);
    int iOnline = 0;
    std::memcpy(&iOnline, readBuffer, 4);

    pMainWindow->setOnlineUsersCount(iOnline);
    pMainWindow->addNewUserToList(std::string(readBuffer + 5));

    pAudioService->addNewUser(std::string(readBuffer + 5));

    pAudioService->playSound(true);
}

void NetworkService::receiveMessage()
{
    // Read packet size
    unsigned short int iPacketSize = 0;
    recv(userTCPSocket, reinterpret_cast<char*>(&iPacketSize), 2, 0);

    char* pReadBuffer = new char[iPacketSize + 2];
    memset(pReadBuffer, 0, iPacketSize + 2);

    // Receive message
    int receivedAmount = recv(userTCPSocket, pReadBuffer, iPacketSize, 0);

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
    char timeText[MAX_NAME_LENGTH + 11];
    memset(timeText, 0, MAX_NAME_LENGTH + 11);
    std::memcpy(timeText, pReadBuffer, static_cast<unsigned long long>(iFirstWCharPos));

    // Copy message to pWChatText
    wchar_t* pWCharText = new wchar_t[static_cast<unsigned long long>(receivedAmount - iFirstWCharPos + 2)];
    memset(pWCharText, 0, static_cast<unsigned long long>(receivedAmount - iFirstWCharPos + 2));
    std::memcpy(pWCharText, pReadBuffer + iFirstWCharPos, static_cast<unsigned long long>(receivedAmount - iFirstWCharPos));

    // Show data on screen
    pMainWindow->printUserMessage(std::string(timeText), std::wstring(pWCharText), true);
    pAudioService->playNewMessageSound();

    delete[] pWCharText;
    delete[] pReadBuffer;
}

void NetworkService::deleteDisconnectedUserFromList()
{
    char readBuffer[MAX_NAME_LENGTH + 11];
    memset(readBuffer, 0, MAX_NAME_LENGTH + 11);

    // Read packet size
    recv(userTCPSocket, readBuffer, 1, 0);
    unsigned char iPacketSize = 0;
    std::memcpy(&iPacketSize, readBuffer, 1);

    // Read other stuff
    recv(userTCPSocket, readBuffer, iPacketSize, 0);
    int iOnline = 0;

    std::memcpy(&iOnline, readBuffer, 4);
    pMainWindow->setOnlineUsersCount(iOnline);

//    // Max user name length = 20 (in ConnectWindow textEdit element) + 1 for null terminator char
//    char userName[21];
//    memset(userName, 0, 21);

//    std::memcpy(userName, readBuffer + 4, iPacketSize - 4);

    pMainWindow->deleteUserFromList(std::string(readBuffer + 4));

    pAudioService->deleteUser(std::string(readBuffer + 4));

    pAudioService->playSound(false);
}

void NetworkService::receivePing()
{
    unsigned short iPacketSize = 0;
    unsigned short iCurrentPos = 0;
    recv(userTCPSocket, reinterpret_cast<char*>(&iPacketSize), sizeof(unsigned short), 0);

    do
    {
        // Read username size
        char nameSize = 0;
        recv(userTCPSocket, &nameSize, 1, 0);
        iCurrentPos++;

        // Read name
        char nameBuffer[MAX_NAME_LENGTH + 1];
        memset(nameBuffer, 0, MAX_NAME_LENGTH + 1);

        recv(userTCPSocket, nameBuffer, nameSize, 0);
        iCurrentPos += nameSize;

        // Read ping
        unsigned short ping = 0;
        recv(userTCPSocket, reinterpret_cast<char*>(&ping), sizeof(unsigned short), 0);
        iCurrentPos += sizeof(unsigned short);

        pMainWindow->setPingToUser(std::string(nameBuffer), ping);

    }while(iCurrentPos < iPacketSize);
}

void NetworkService::sendMessage(std::wstring message)
{
    if (message.size() > MAX_MESSAGE_LENGTH) pMainWindow->showMessageBox(0, "Your message is too big!");
    else
    {
        char* pSendBuffer = new char[3 + (message.size() * 2) + 2];
        memset(pSendBuffer, 0, 3 + (message.size() * 2) + 2);

        unsigned short int iPacketSize = static_cast<unsigned short>( message.size() * 2 );

        // 10 means it's a message
        unsigned char commandType = 10;
        std::memcpy(pSendBuffer, &commandType, 1);
        std::memcpy(pSendBuffer + 1, &iPacketSize, 2);

        std::memcpy(pSendBuffer + 3, message.c_str(), message.size() * 2);

        int sendSize = send(  userTCPSocket, pSendBuffer, static_cast<int>( 3 + (message.size() * 2) ), 0  );
        if ( sendSize != static_cast<int>(3 + (message.size() * 2)) )
        {
            if (sendSize == SOCKET_ERROR)
            {
                int error = WSAGetLastError();

                if (error == 10054)
                {
                    pMainWindow->printOutput(std::string("\nWARNING:\nYour message has not been sent!\n"
                                                     "NetworkService::sendMessage()::send() failed and returned: " + std::to_string(error) + "."));
                    lostConnection();
                    pMainWindow->clearTextEdit();
                }
                else
                {
                    pMainWindow->printOutput(std::string("\nWARNING:\nYour message has not been sent!\n"
                                                     "NetworkService::sendMessage()::send() failed and returned: " + std::to_string(error) + ".\n"));
                }
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

void NetworkService::sendVoiceMessage(char *pVoiceMessage, int iMessageSize, bool bLast)
{
    if (bVoiceListen)
    {
        int iSize = 0;

        if (bLast)
        {
            char lastVoice = 1;
            iSize = sendto(userUDPSocket, &lastVoice, iMessageSize, 0, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
        }
        else
        {
            char* pSend = new char [static_cast<size_t>(iMessageSize + 1)];
            // '2' - not last
            pSend[0] = 2;

            std::memcpy(pSend + 1, pVoiceMessage, static_cast<size_t>(iMessageSize));

            iSize = sendto(userUDPSocket, pSend, iMessageSize + 1, 0, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));

            iMessageSize += 1;

            delete[] pSend;
        }

        if (iSize != iMessageSize)
        {
            if (iSize == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("\nWARNING:\nYour voice message has not been sent!\n"
                                                     "NetworkService::sendVoiceMessage()::sendto() failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"), true);
            }
            else
            {
                pMainWindow->printOutput(std::string("\nWARNING:\nSomething went wrong and your voice message wasn't sent fully.\n"), true);
            }
        }
    }

    if (pVoiceMessage != nullptr) delete[] pVoiceMessage;
}

void NetworkService::disconnect()
{
    if (bTextListen)
    {
        bTextListen  = false;
        if (bVoiceListen)
        {
            bVoiceListen = false;
            closesocket(userUDPSocket);
            pAudioService->stop();
        }

        char readBuffer[5];

        // Send FIN
        int returnCode = shutdown(userTCPSocket, SD_SEND);
        if (returnCode == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("NetworkService::disconnect()::shutdown() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"));
            closesocket(userTCPSocket);
            WSACleanup();
            bWinSockLaunched = false;
        }
        else
        {
            // Translate socket to blocking mode
            u_long arg = false;
            if (ioctlsocket(userTCPSocket, static_cast<long>(FIONBIO), &arg) == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("NetworkService::disconnect()::ioctsocket() (blocking mode) failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"));
                closesocket(userTCPSocket);
                WSACleanup();
                bWinSockLaunched = false;
            }

            if (recv(userTCPSocket, readBuffer, 5, 0) == 0)
            {
                returnCode = closesocket(userTCPSocket);
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

                closesocket(userTCPSocket);
                WSACleanup();
                bWinSockLaunched = false;

                pMainWindow->deleteUserFromList("",true);
                pMainWindow->setOnlineUsersCount(0);
                pMainWindow->enableInteractiveElements(true,false);
            }
        }

        // Wait for serverMonitor() to end
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    else
    {
        pMainWindow->showMessageBox(0, std::string("You are not connected.") );
    }
}

void NetworkService::lostConnection()
{
    pMainWindow->printOutput( std::string("\nThe server is not responding. Disconnecting...\n"), true );

    bTextListen  = false;
    if (bVoiceListen)
    {
        bVoiceListen = false;
        closesocket(userUDPSocket);
        pAudioService->playLostConnectionSound();
        pAudioService->stop();
    }

    closesocket(userTCPSocket);
    WSACleanup();
    bWinSockLaunched = false;

    pMainWindow->deleteUserFromList("",true);
    pMainWindow->setOnlineUsersCount(0);
    pMainWindow->enableInteractiveElements(true,false);
}

void NetworkService::answerToFIN()
{
    bTextListen = false;
    if (bVoiceListen)
    {
        bVoiceListen = false;
        closesocket(userUDPSocket);
        pAudioService->stop();
    }

    pMainWindow->printOutput(std::string("Server is closing connection.\n"),true);
    int returnCode = shutdown(userTCPSocket, SD_SEND);
    if (returnCode == SOCKET_ERROR)
    {
         pMainWindow->printOutput(std::string("NetworkService::listenForServer()::shutdown() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"), true);
         closesocket(userTCPSocket);
         WSACleanup();
         bWinSockLaunched = false;
    }
    else
    {
        returnCode = closesocket(userTCPSocket);
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

    // Wait for serverMonitor() to end
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}





void NetworkService::stop()
{
    if (bTextListen)
    {
        disconnect();
    }
}

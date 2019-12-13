#pragma once


// STL
#include <string>
#include <vector>
#include <mutex>

// ============== Network ==============
// Sockets and stuff
#include <winsock2.h>

// Adress translation
#include <ws2tcpip.h>

// Winsock 2 Library
#pragma comment(lib,"Ws2_32.lib")


// for mmsystem
#pragma comment(lib,"Winmm.lib")
// for GetAsyncKeyState
#pragma comment(lib, "user32.lib")


// Other
#include <Windows.h>
#include "Mmsystem.h"



class QListWidgetItem;


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class User
{
public:

    User(const std::string& sUserName, int iPing, QListWidgetItem* pListWidgetItem)
    {
        this ->sUserName       = sUserName;
        this ->iPing           = iPing;
        this ->pListWidgetItem = pListWidgetItem;
        bTalking               = false;


        vWaveOutHdrs .push_back(&WaveOutHdr1);
        vWaveOutHdrs .push_back(&WaveOutHdr2);
    }


    std::mutex          mtxUser;


    /////////////////////////////////////////////
    //////////      NETWORK    //////////////////
    /////////////////////////////////////////////


    SOCKET              sockUserTCP;
    SOCKET              sockUserUDP;
    sockaddr_in         addrServer;


    std::string         sUserName;



    /////////////////////////////////////////////
    //////////      AUDIO      //////////////////
    /////////////////////////////////////////////
    // (not used if this User object represents this user (i.e. client))
    /////////////////////////////////////////////


    // Audio packets
    std::vector<short*> vAudioPackets;
    bool                bPacketsArePlaying;
    bool                bDeletePacketsAtLast;
    bool                bLastPacketCame;


    // Waveform-audio output device
    HWAVEOUT            hWaveOut;


    // Audio buffers
    WAVEHDR             WaveOutHdr1;
    WAVEHDR             WaveOutHdr2;
    // don't forget to add new line in the constructor
    // in vWaveOutHdrs .push_back(&WaveOutHdr2);


    std::vector<WAVEHDR*> vWaveOutHdrs;


    float               fUserDefinedVolume;



    /////////////////////////////////////////////
    //////////       UI        //////////////////
    /////////////////////////////////////////////



    QListWidgetItem*    pListWidgetItem;
    int                 iPing;
    bool                bTalking;
};

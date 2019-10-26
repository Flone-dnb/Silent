#pragma once

// STL
#include <string>
#include <vector>
#include <mutex>

// Other
#define _WINSOCKAPI_    // stops windows.h from including winsock.h
#include <Windows.h>
#include "Mmsystem.h"

// for mmsystem
#pragma comment(lib,"Winmm.lib")
// for GetAsyncKeyState
#pragma comment(lib, "user32.lib")


class MainWindow;
class NetworkService;

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


struct UserAudioStruct
{
    std::string         sUserName;


    // Audio packets
    std::vector<short*> vAudioPackets;
    bool                bPacketsArePlaying;
    bool                bDeletePacketsAtLast;
    bool                bLastPacketCame;


    float               fUserDefinedVolume;


    // Waveform-audio output device
    HWAVEOUT            hWaveOut;


    // Audio buffers
    WAVEHDR             WaveOutHdr1;
    WAVEHDR             WaveOutHdr2;
    WAVEHDR             WaveOutHdr3;
};



// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class AudioService
{

public:

    AudioService(MainWindow* pMainWindow);




    // Start

        void   prepareForStart               ();
        bool   start                         ();


    // Play sound

        void   playSound                     (bool bConnectSound);
        void   playNewMessageSound           ();
        void   playLostConnectionSound       ();


    // User add/delete

        void   addNewUser                    (std::string sUserName);
        void   deleteUser                    (std::string sUserName);
        void   deleteAll                     ();


    // Audio data record/play

        void   playAudioData                 (short int* pAudio,    std::string sUserName, bool bLast);
        void   play                          (UserAudioStruct* pUser);
        void   sendAudioData                 (short* pAudio);
        void   recordOnPress                 ();


    // Stop

        void   waitForAllBuffers             (UserAudioStruct* pUser);
        void   stop                          ();


    // SET functions

        void   setNewUserVolume              (std::string sUserName,  float fVolume);
        void   setPushToTalkButtonAndVolume  (int iKey,               unsigned short int iVolume);
        void   setNetworkService             (NetworkService* pNetworkService);


    // GET functions

        float  getUserCurrentVolume          (std::string sUserName);





    ~AudioService();

private:

    // Used in recordOnPress()

        bool  addInBuffer       (LPWAVEHDR buffer);
        bool  addOutBuffer      (HWAVEOUT  hWaveOut, LPWAVEHDR buffer);
        void  waitAndSendBuffer (WAVEHDR* WaveInHdr, short int* pWaveIn);


    // Waveform-audio input device
    HWAVEIN         hWaveIn;

    // Audio format
    WAVEFORMATEX    Format;


    // Audio buffers
    WAVEHDR         WaveInHdr1;
    WAVEHDR         WaveInHdr2;
    WAVEHDR         WaveInHdr3;
    WAVEHDR         WaveInHdr4;


    // "In" buffers
    short int*      pWaveIn1;
    short int*      pWaveIn2;
    short int*      pWaveIn3;
    short int*      pWaveIn4;


    MainWindow*     pMainWindow;
    NetworkService* pNetworkService;


    // Record quality
    // Do not set 'sampleCout' to more than ~700 (700 * 2 = 1400) (~MTU)
    // we '*2" because audio data in PCM16, 1 sample = 16 bits.
    // Of course, we can send 2 packets, but it's just more headache.
    const int       sampleCount = 690;   // 46 ms (= 'sampleRate' (15000) * 0.046)
    unsigned long   sampleRate  = 15000; // 15000 hz (samples per second)


    // Push-to-talk
    int             iPushToTalkButton;
    bool            bInputReady;


    // Volume
    unsigned short int iVolume;
    float              fMasterVolumeMult;


    // Users
    std::vector<UserAudioStruct*> vUsersAudio;
    std::mutex                    mtxUsersAudio;
};

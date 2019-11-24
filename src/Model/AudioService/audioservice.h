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
class SettingsManager;

class User;



#define  BUFFER_UPDATE_CHECK_MS      2

#define  AUDIO_CONNECT_PATH          L"sounds/connect.wav"
#define  AUDIO_DISCONNECT_PATH       L"sounds/disconnect.wav"
#define  AUDIO_LOST_CONNECTION_PATH  L"sounds/lostconnection.wav"
#define  AUDIO_NEW_MESSAGE_PATH      L"sounds/newmessage.wav"
#define  AUDIO_PRESS_PATH            L"sounds/press.wav"
#define  AUDIO_UNPRESS_PATH          L"sounds/unpress.wav"



// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class AudioService
{

public:

    AudioService(MainWindow* pMainWindow, SettingsManager* pSettingsManager);




    // Start

        void   prepareForStart               ();
        bool   start                         ();


    // Play sound

        void   playConnectDisconnectSound    (bool bConnectSound);
        void   playNewMessageSound           ();
        void   playLostConnectionSound       ();


    // User add/delete

        void   setupUserAudio                (User* pUser);
        void   deleteUserAudio               (User* pUser);


    // Audio data record/play

        void   playAudioData                 (short int* pAudio,    std::string sUserName, bool bLast);
        void   play                          (User* pUser);
        void   sendAudioData                 (short* pAudio);
        void   recordOnPress                 ();


    // Stop

        void   stop                          ();


    // SET functions

        void   setNewUserVolume              (std::string sUserName,  float fVolume);
        void   setNewMasterVolume            (unsigned short int iVolume);
        void   setNetworkService             (NetworkService* pNetworkService);


    // GET functions

        float  getUserCurrentVolume          (const std::string& sUserName);





    ~AudioService();

private:

    // Used in recordOnPress()

        bool  addInBuffer       (LPWAVEHDR buffer);
        bool  addOutBuffer      (HWAVEOUT  hWaveOut, LPWAVEHDR buffer);
        void  waitAndSendBuffer (WAVEHDR* WaveInHdr, short int* pWaveIn);
        void  waitForAllInBuffers();

    // Used in play()

        void  waitForPlayToEnd  (User* pUser, WAVEHDR* pWaveOutHdr, size_t& iLastPlayingPacketIndex);
        void  waitForAllBuffers (User* pUser, bool bClearPackets, size_t* iLastPlayingPacketIndex);


    // -------------------------------------------------------------


    MainWindow*      pMainWindow;
    NetworkService*  pNetworkService;
    SettingsManager* pSettingsManager;


    // Waveform-audio input device
    HWAVEIN          hWaveIn;


    // Audio format
    WAVEFORMATEX     Format;


    // Audio buffers
    WAVEHDR          WaveInHdr1;
    WAVEHDR          WaveInHdr2;
    WAVEHDR          WaveInHdr3;
    WAVEHDR          WaveInHdr4;


    // "In" buffers
    short int*       pWaveIn1;
    short int*       pWaveIn2;
    short int*       pWaveIn3;
    short int*       pWaveIn4;


    // Record quality
    // Do not set 'sampleCout' to more than ~700 (700 * 2 = 1400) (~MTU)
    // we '*2" because audio data in PCM16, 1 sample = 16 bits.
    // Of course, we can send 2 packets, but it's just more headache.
    const int        sampleCount = 690;   // 46 ms (= 'sampleRate' (15000) * 0.046)
    unsigned long    sampleRate  = 15000; // 15000 hz (samples per second)


    // Push-to-talk
    float            fMasterVolumeMult;
    bool             bInputReady;
};

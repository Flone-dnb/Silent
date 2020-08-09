// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

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
#define  AUDIO_SERVER_MESSAGE_PATH   L"sounds/servermessage.wav"



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
        void   startTestWaveOut              ();


    // Play sound

        void   playConnectDisconnectSound    (bool bConnectSound);
        void   playServerMessageSound        ();
        void   playNewMessageSound           ();
        void   playLostConnectionSound       ();


    // User add/delete

        void   setupUserAudio                (User* pUser);
        void   deleteUserAudio               (User* pUser);


    // Audio data record/play

        void   setTestRecordingPause         (bool bPause);
        void   playAudioData                 (short int* pAudio,    std::string sUserName, bool bLast);
        void   play                          (User* pUser);


    // Stop

        void   stop                          ();


    // SET functions

        void   setInputAudioVolume           (int iVolume);
        void   setVoiceStartValue            (int iValue);
        void   setShouldHearTestVoice        (bool bHear);
        void   setNewUserVolume              (std::string sUserName,  float fVolume);
        void   setNewMasterVolume            (unsigned short int iVolume);
        void   setNetworkService             (NetworkService* pNetworkService);


    // GET functions

        float  getUserCurrentVolume          (const std::string& sUserName);
        std::vector<std::wstring> getInputDevices();





    ~AudioService();

private:

    // Recording

        void   recordOnPush                  ();
        void   recordOnTalk                  ();
        void   testRecord                    ();


    // Used in recordOnPress()/testRecord()

        bool  addInBuffer       (LPWAVEHDR buffer, bool bTestDevice = false);
        bool  addOutBuffer      (HWAVEOUT  hWaveOut, LPWAVEHDR buffer);
        void  waitAndSendBuffer (WAVEHDR* WaveInHdr, short int* pWaveIn, bool bOnTalk = false);
        void  waitAndShowBufferVolume(WAVEHDR* WaveInHdr, short int* pWaveIn);
        void  waitForAllInBuffers();
        void  waitForAllTestInBuffers();
        void  waitForAllTestOutBuffers();
        void  sendAudioData     (short* pAudio);
        void  sendAudioDataOnTalk(short* pAudio);
        void  sendAudioDataVolume(short* pAudio);
        void  testOutputAudio   ();

    // Used in play()

        void  waitForPlayToEnd  (User* pUser, WAVEHDR* pWaveOutHdr, size_t& iLastPlayingPacketIndex);
        void  waitForPlayOnTestToEnd(WAVEHDR* pWaveOutHdr);
        void  waitForAllBuffers (User* pUser, bool bClearPackets, size_t* iLastPlayingPacketIndex);

    // Used in start()

        int  getInputDeviceID  (std::wstring sDeviceName);

    // -------------------------------------------------------------


    MainWindow*      pMainWindow;
    NetworkService*  pNetworkService;
    SettingsManager* pSettingsManager;


    // Waveform-audio input device
    HWAVEIN          hWaveIn;
    HWAVEIN          hTestWaveIn; // Used to show the voice meter in the Settings window.


    // Audio format
    WAVEFORMATEX     Format;


    // Audio buffers
    WAVEHDR          WaveInHdr1;
    WAVEHDR          WaveInHdr2;
    WAVEHDR          WaveInHdr3;
    WAVEHDR          WaveInHdr4;
    WAVEHDR          TestWaveInHdr1;
    WAVEHDR          TestWaveInHdr2;
    WAVEHDR          TestWaveInHdr3;
    WAVEHDR          TestWaveInHdr4;


    // "In" buffers
    short int*       pWaveIn1;
    short int*       pWaveIn2;
    short int*       pWaveIn3;
    short int*       pWaveIn4;
    short int*       pTestWaveIn1;
    short int*       pTestWaveIn2;
    short int*       pTestWaveIn3;
    short int*       pTestWaveIn4;


    // Audio packets
    std::vector<short*> vAudioPacketsForTest;
    std::mutex          mtxAudioPacketsForTest;


    // Waveform-audio output device
    HWAVEOUT            hTestWaveOut;


    // Audio buffers
    WAVEHDR             TestWaveOutHdr1;
    WAVEHDR             TestWaveOutHdr2;


    // Record quality
    // Do not set 'sampleCout' to more than ~700 (700 * 2 = 1400) (~MTU)
    // we '*2" because audio data in PCM16, 1 sample = 16 bits.
    // Of course, we can send 2 packets, but it's just more headache.
    const int        sampleCount = 679;   // 35 ms (= 'sampleRate' (19400) * 0.035)
    unsigned long    sampleRate  = 19400; // 19400 hz (samples per second)


    // Voice.
    int              iAudioInputVolume;
    int              iLowerVoiceStartRecValueInDBFS;
    int              iTestPacketsNeedToRecordLeft;
    int              iPacketsNeedToRecordLeft;
    float            fMasterVolumeMult;
    bool             bInputReady;
    bool             bTestInputReady;
    bool             bPauseTestInput;
    bool             bOutputTestVoice;
    bool             bRecordTalk;
    bool             bRecordedSome;
};

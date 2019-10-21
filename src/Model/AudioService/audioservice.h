#pragma once

#define _WINSOCKAPI_    // stops windows.h from including winsock.h
#include <Windows.h>
#include "Mmsystem.h"

#include <string>
#include <vector>
#include <mutex>

// for mmsystem
#pragma comment(lib,"Winmm.lib")
// for GetAsyncKeyState
#pragma comment(lib, "user32.lib")


struct UserAudioStruct
{
    std::string userName;

    std::vector<short*> audioPackets;
    bool bPacketsArePlaying;
    bool bDeletePacketsAtLast;
    bool bLastPacketCame;

    float fUserDefinedVolume;

    // Waveform-audio output device
    HWAVEOUT hWaveOut;

    // Audio buffers
    WAVEHDR WaveOutHdr1;
    WAVEHDR WaveOutHdr2;
    WAVEHDR WaveOutHdr3;
};



class MainWindow;
class NetworkService;

class AudioService
{

public:

    AudioService(MainWindow* pMainWindow);

    void prepareForStart();
    bool start();

    void playSound(bool bConnectSound);
    void playNewMessageSound();
    void playLostConnectionSound();

    void addNewUser(std::string userName);
    void deleteUser(std::string userName);
    void deleteAll();

    void recordOnPress();
    void sendAudioData(short* pAudio);
    void playAudioData(short int* pAudio, std::string userName, bool bLast);
    void play(UserAudioStruct* user);

    void waitForAllBuffers(UserAudioStruct* user);
    void stop();


    // set
    void setNetworkService(NetworkService* pNetworkService);
    void setPushToTalkButtonAndVolume(int key, unsigned short int volume);
    void setNewUserVolume(std::string userName, float fVolume);

    // get
    float getUserCurrentVolume(std::string userName);


    ~AudioService();

private:

    bool addInBuffer(LPWAVEHDR buffer);
    bool addOutBuffer(HWAVEOUT hWaveOut, LPWAVEHDR buffer);


    // Waveform-audio input device
    HWAVEIN  hWaveIn;

    // Audio format
    WAVEFORMATEX Format;


    // Audio buffers
    WAVEHDR WaveInHdr1;
    WAVEHDR WaveInHdr2;
    WAVEHDR WaveInHdr3;
    WAVEHDR WaveInHdr4;


    // Do not set 'sampleCout' to more than ~700 (700 * 2 = 1400) (~MTU)
    // we '*2" because audio data in PCM16, 1 sample = 16 bits.
    // Of course, we can send 2 packets, but it's just more headache.
    const int sampleCount = 690; // 46 ms (= 'sampleRate' (15000) * 0.046)
    unsigned long sampleRate = 15000; // 15000 hz (samples per second)


    short int* pWaveIn1;
    short int* pWaveIn2;
    short int* pWaveIn3;
    short int* pWaveIn4;



    bool bInputReady;
    int iPushToTalkButton;

    unsigned short int iVolume;
    float fMasterVolumeMult;


    std::vector<UserAudioStruct*> usersAudio;

    std::mutex mtxUsersAudio;


    MainWindow* pMainWindow;
    NetworkService* pNetworkService;
};

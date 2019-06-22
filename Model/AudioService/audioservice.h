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
    // From 0 to 100
    char userVolume;

    std::vector<short*> audioPackets;
    bool bPacketsArePlaying;
    bool bDeletePacketsAtLast;
    bool bLastPacketCame;

    // Waveform-audio output device
    HWAVEOUT hWaveOut;

    // Audio buffers
    WAVEHDR WaveOutHdr1;
    WAVEHDR WaveOutHdr2;
    WAVEHDR WaveOutHdr3;
    WAVEHDR WaveOutHdr4;
};



class MainWindow;
class NetworkService;

class AudioService
{

public:

    AudioService(MainWindow* pMainWindow);


    void setNetworkService(NetworkService* pNetworkService);
    void setPushToTalkButtonAndVolume(int key, unsigned short int volume);

    void start();

    void playSound(bool bConnectSound);
    void playNewMessageSound();

    void addNewUser(std::string userName);
    void deleteUser(std::string userName);
    void deleteAll();

    void recordOnPress();
    void compressAndSend(short* pAudio);
    void uncompressAndPlay(char* pAudio, std::string userName, bool bLast);
    void play(UserAudioStruct* user);

    void stop();


    ~AudioService();

private:

    bool addInBuffer(LPWAVEHDR buffer);
    bool addOutBuffer(HWAVEOUT hWaveOut, LPWAVEHDR buffer);

    char MuLaw_Encode(short number);

    short MuLaw_Decode(char number);


    // Waveform-audio input device
    HWAVEIN  hWaveIn;

    // Audio format
    WAVEFORMATEX Format;


    // Audio buffers
    WAVEHDR WaveInHdr1;
    WAVEHDR WaveInHdr2;
    WAVEHDR WaveInHdr3;
    WAVEHDR WaveInHdr4;


    // Do not set 'sampleCout' to more than ~1400-1460 (MTU) (more than ~700 without compression: MuLaw_Encode & MuLaw_Decode).
    // Of course, we can send 2 packets, but why do we need that?
    const int sampleCount = 690; // 46 ms (= 'sampleRate' (15000) * 0.046)
    unsigned long sampleRate = 15000; // 15000 hz (samples per second)


    short int* pWaveIn1;
    short int* pWaveIn2;
    short int* pWaveIn3;
    short int* pWaveIn4;



    bool bInputReady;
    int iPushToTalkButton;

    unsigned short int iVolume;


    std::vector<UserAudioStruct*> usersAudio;

    std::mutex mtxUsersAudio;


    MainWindow* pMainWindow;
    NetworkService* pNetworkService;
};

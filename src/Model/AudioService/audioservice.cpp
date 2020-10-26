// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "audioservice.h"


// STL
#include <thread>

// Custom
#include "View/MainWindow/mainwindow.h"
#include "Model/NetworkService/networkservice.h"
#include "Model/SettingsManager/settingsmanager.h"
#include "Model/SettingsManager/SettingsFile.h"
#include "Model/User.h"
#include "Model/net_params.h"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


AudioService::AudioService(MainWindow* pMainWindow, SettingsManager* pSettingsManager)
{
    this->pMainWindow      = pMainWindow;
    this->pSettingsManager = pSettingsManager;


    // Do not record audio now
    bInputReady             = false;
    bTestInputReady         = false;
    bPauseTestInput         = true;
    bRecordTalk             = false;
    bMuteMic                = false;


    // "In" (record) buffers
    pWaveIn1                = nullptr;
    pWaveIn2                = nullptr;
    pWaveIn3                = nullptr;
    pWaveIn4                = nullptr;

    pTestWaveIn1                = nullptr;
    pTestWaveIn2                = nullptr;
    pTestWaveIn3                = nullptr;
    pTestWaveIn4                = nullptr;


    // All audio will be x1.45 volume
    // Because waveOutVolume() does not make it loud enough
    fMasterVolumeMult       = 1.45f;

    iAudioInputVolume = pSettingsManager->getCurrentSettings()->iInputVolumeMultiplier;
    iLowerVoiceStartRecValueInDBFS = pSettingsManager->getCurrentSettings()->iVoiceStartRecValueInDBFS;
    bOutputTestVoice = !pSettingsManager->getCurrentSettings()->bPushToTalkVoiceMode;

    startTestWaveOut();
}




void AudioService::setNetworkService(NetworkService *pNetworkService)
{
    this->pNetworkService = pNetworkService;
}

void AudioService::setMuteMic(bool bMute)
{
    bMuteMic = bMute;
}

bool AudioService::getMuteMic()
{
    return bMuteMic;
}

float AudioService::getUserCurrentVolume(const std::string &sUserName)
{
    pNetworkService->getOtherUsersMutex()->lock();


    float fUserVolume = 0.0f;

    for (size_t i = 0;   i < pNetworkService->getOtherUsersVectorSize();   i++)
    {
        if ( pNetworkService->getOtherUser(i)->sUserName == sUserName )
        {
            fUserVolume = pNetworkService->getOtherUser(i)->fUserDefinedVolume;

            break;
        }
    }



    pNetworkService->getOtherUsersMutex()->unlock();


    return fUserVolume;
}

std::vector<std::wstring> AudioService::getInputDevices()
{
    UINT iInDeviceCount = waveInGetNumDevs();

    std::vector<std::wstring> vSupportedDevices;

    for (UINT i = 0; i < iInDeviceCount; i++)
    {
        WAVEINCAPS deviceInfo;

        MMRESULT result = waveInGetDevCaps(i, &deviceInfo, sizeof(deviceInfo));
        if (result)
        {
            char fault [256];
            memset (fault, 0, 256);

            waveInGetErrorTextA (result, fault, 256);
            pMainWindow->printOutput (std::string("AudioService::getInputDevices::waveInGetDevCaps() error: " + std::string(fault) + "."),
                                      SilentMessage(false),
                                      true);
           continue;
        }
        else
        {
            vSupportedDevices.push_back(deviceInfo.szPname);
        }
    }

    return vSupportedDevices;
}

int AudioService::getAudioPacketSizeInSamples() const
{
    return sampleCount;
}

void AudioService::setNewMasterVolume(unsigned short int iVolume)
{
    pNetworkService->getOtherUsersMutex()->lock();


    for (size_t i = 0;  i < pNetworkService->getOtherUsersVectorSize();  i++)
    {
        waveOutSetVolume( pNetworkService->getOtherUser(i)->hWaveOut, MAKELONG(iVolume, iVolume) );
    }

    waveOutSetVolume( hTestWaveOut, MAKELONG(iVolume, iVolume) );


    pNetworkService->getOtherUsersMutex()->unlock();
}

void AudioService::setNewUserVolume(std::string sUserName, float fVolume)
{
    User* pUser = nullptr;

    pNetworkService->getOtherUsersMutex()->lock();



    for (size_t i = 0;   i < pNetworkService->getOtherUsersVectorSize();   i++)
    {
        if (pNetworkService->getOtherUser(i)->sUserName == sUserName)
        {
            pUser = pNetworkService->getOtherUser(i);
            break;
        }
    }

    if (pUser)
    {
       pUser->fUserDefinedVolume = fVolume;
    }



    pNetworkService->getOtherUsersMutex()->unlock();
}

void AudioService::prepareForStart()
{
    pWaveIn1  = new short int [ static_cast<size_t>(sampleCount) ];
    pWaveIn2  = new short int [ static_cast<size_t>(sampleCount) ];
    pWaveIn3  = new short int [ static_cast<size_t>(sampleCount) ];
    pWaveIn4  = new short int [ static_cast<size_t>(sampleCount) ];


    // Format
    Format.wFormatTag      = WAVE_FORMAT_PCM;
    Format.nChannels       = 1;    //  '1' - mono, '2' - stereo
    Format.cbSize          = 0;
    Format.wBitsPerSample  = 16;
    Format.nSamplesPerSec  = sampleRate;
    Format.nBlockAlign     = Format.nChannels      * Format.wBitsPerSample / 8;
    Format.nAvgBytesPerSec = Format.nSamplesPerSec * Format.nChannels           * Format.wBitsPerSample / 8;


    // "In" buffers

    // Audio buffer 1
    WaveInHdr1.lpData          = reinterpret_cast <LPSTR>         (pWaveIn1);
    WaveInHdr1.dwBufferLength  = static_cast      <unsigned long> (sampleCount * 2);
    WaveInHdr1.dwBytesRecorded = 0;
    WaveInHdr1.dwUser          = 0L;
    WaveInHdr1.dwFlags         = 0L;
    WaveInHdr1.dwLoops         = 0L;


    // Audio buffer 2
    WaveInHdr2 = WaveInHdr1;
    WaveInHdr2.lpData = reinterpret_cast <LPSTR> (pWaveIn2);


    // Audio buffer 3
    WaveInHdr3 = WaveInHdr1;
    WaveInHdr3.lpData = reinterpret_cast <LPSTR> (pWaveIn3);


    // Audio buffer 4
    WaveInHdr4 = WaveInHdr1;
    WaveInHdr4.lpData = reinterpret_cast <LPSTR> (pWaveIn4);
}

bool AudioService::start()
{
    MMRESULT result;

    UINT iDeviceID = WAVE_MAPPER;

    int iPreferredDeviceID = getInputDeviceID( pSettingsManager->getCurrentSettings()->sInputDeviceName );
    if (iPreferredDeviceID != -1)
    {
        iDeviceID = static_cast<UINT>(iPreferredDeviceID);
    }

    // Start input device
    result = waveInOpen (&hWaveIn,  iDeviceID,  &Format,  0L,  0L,  WAVE_FORMAT_DIRECT);

    if (result)
    {
        char fault [256];
        memset (fault, 0, 256);


        waveInGetErrorTextA (result, fault, 256);
        pMainWindow->printOutput (std::string("AudioService::start::waveInOpen() error: " + std::string(fault) + "."),
                                   SilentMessage(false),
                                   true);


        delete[] pWaveIn1;
        pWaveIn1 = nullptr;

        delete[] pWaveIn2;
        pWaveIn2 = nullptr;

        delete[] pWaveIn3;
        pWaveIn3 = nullptr;

        delete[] pWaveIn4;
        pWaveIn4 = nullptr;


        return false;
    }
    else
    {
        bInputReady = true;
    }


    if (pSettingsManager->getCurrentSettings()->bPushToTalkVoiceMode)
    {
        std::thread recordThread (&AudioService::recordOnPush, this);
        recordThread.detach ();
    }
    else
    {
        std::thread recordThread (&AudioService::recordOnTalk, this);
        recordThread.detach ();
    }

    return true;
}

void AudioService::startTestWaveOut()
{
    pTestWaveIn1  = new short int [ static_cast<size_t>(sampleCount) ];
    pTestWaveIn2  = new short int [ static_cast<size_t>(sampleCount) ];
    pTestWaveIn3  = new short int [ static_cast<size_t>(sampleCount) ];
    pTestWaveIn4  = new short int [ static_cast<size_t>(sampleCount) ];


    // Format
    Format.wFormatTag      = WAVE_FORMAT_PCM;
    Format.nChannels       = 1;    //  '1' - mono, '2' - stereo
    Format.cbSize          = 0;
    Format.wBitsPerSample  = 16;
    Format.nSamplesPerSec  = sampleRate;
    Format.nBlockAlign     = Format.nChannels      * Format.wBitsPerSample / 8;
    Format.nAvgBytesPerSec = Format.nSamplesPerSec * Format.nChannels           * Format.wBitsPerSample / 8;


    // "In" buffers

    // Audio buffer 1
    TestWaveInHdr1.lpData          = reinterpret_cast <LPSTR>         (pTestWaveIn1);
    TestWaveInHdr1.dwBufferLength  = static_cast      <unsigned long> (sampleCount * 2);
    TestWaveInHdr1.dwBytesRecorded = 0;
    TestWaveInHdr1.dwUser          = 0L;
    TestWaveInHdr1.dwFlags         = 0L;
    TestWaveInHdr1.dwLoops         = 0L;


    // Audio buffer 2
    TestWaveInHdr2 = TestWaveInHdr1;
    TestWaveInHdr2.lpData = reinterpret_cast <LPSTR> (pTestWaveIn2);


    // Audio buffer 3
    TestWaveInHdr3 = TestWaveInHdr1;
    TestWaveInHdr3.lpData = reinterpret_cast <LPSTR> (pTestWaveIn3);


    // Audio buffer 4
    TestWaveInHdr4 = TestWaveInHdr1;
    TestWaveInHdr4.lpData = reinterpret_cast <LPSTR> (pTestWaveIn4);

    MMRESULT result;

    UINT iDeviceID = WAVE_MAPPER;

    int iPreferredDeviceID = getInputDeviceID( pSettingsManager->getCurrentSettings()->sInputDeviceName );
    if (iPreferredDeviceID != -1)
    {
        iDeviceID = static_cast<UINT>(iPreferredDeviceID);
    }

    // Start input device
    result = waveInOpen (&hTestWaveIn,  iDeviceID,  &Format,  0L,  0L,  WAVE_FORMAT_DIRECT);

    if (result)
    {
        char fault [256];
        memset (fault, 0, 256);


        waveInGetErrorTextA (result, fault, 256);
        pMainWindow->printOutput (std::string("AudioService::startTestWaveOut::waveInOpen() error: " + std::string(fault) + "."),
                                   SilentMessage(false),
                                   true);


        delete[] pTestWaveIn1;
        pTestWaveIn1 = nullptr;

        delete[] pTestWaveIn2;
        pTestWaveIn2 = nullptr;

        delete[] pTestWaveIn3;
        pTestWaveIn3 = nullptr;

        delete[] pTestWaveIn4;
        pTestWaveIn4 = nullptr;

        return;
    }
    else
    {
        bTestInputReady = true;
    }


    std::thread recordThread (&AudioService::testRecord, this);
    recordThread.detach ();
}

void AudioService::playConnectDisconnectSound(bool bConnectSound)
{
    if (pSettingsManager->getCurrentSettings()->bPlayConnectDisconnectSound)
    {
        if (bConnectSound)
        {
            PlaySoundW( AUDIO_CONNECT_PATH,    nullptr, SND_FILENAME | SND_ASYNC );
        }
        else
        {
            PlaySoundW( AUDIO_DISCONNECT_PATH, nullptr, SND_FILENAME | SND_ASYNC );
        }
    }
}

void AudioService::playMuteMicSound(bool bMuteSound)
{
    if (bMuteSound)
    {
        PlaySoundW( AUDIO_MUTE_MIC_PATH,   nullptr, SND_FILENAME | SND_ASYNC );
    }
    else
    {
        PlaySoundW( AUDIO_UNMUTE_MIC_PATH, nullptr, SND_FILENAME | SND_ASYNC );
    }
}

void AudioService::playServerMessageSound()
{
    PlaySoundW( AUDIO_SERVER_MESSAGE_PATH, nullptr, SND_FILENAME | SND_ASYNC );
}

void AudioService::playNewMessageSound()
{
    if (pSettingsManager->getCurrentSettings()->bPlayTextMessageSound)
    {
       PlaySoundW( AUDIO_NEW_MESSAGE_PATH, nullptr, SND_FILENAME | SND_ASYNC );
    }
}

void AudioService::playLostConnectionSound()
{
    PlaySoundW( AUDIO_LOST_CONNECTION_PATH, nullptr, SND_FILENAME );
}

void AudioService::setupUserAudio(User *pUser)
{
    pUser->bPacketsArePlaying   = false;
    pUser->bDeletePacketsAtLast = false;
    pUser->bLastPacketCame      = false;
    pUser->fUserDefinedVolume   = 1.0f;


    // Audio buffer1
    pUser->WaveOutHdr1.dwBufferLength  = static_cast <unsigned long> (sampleCount * 2);
    pUser->WaveOutHdr1.dwBytesRecorded = 0;
    pUser->WaveOutHdr1.dwUser          = 0L;
    pUser->WaveOutHdr1.dwFlags         = 0L;
    pUser->WaveOutHdr1.dwLoops         = 0L;

    // Audio buffer2
    pUser->WaveOutHdr2 = pUser->WaveOutHdr1;



    // Start output device
    MMRESULT result = waveOutOpen( &pUser->hWaveOut,  WAVE_MAPPER,  &Format,  0L,  0L,  WAVE_FORMAT_DIRECT );

    if (result)
    {
        char fault [256];
        memset (fault, 0, 256);

        waveInGetErrorTextA (result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::addNewUser::waveOutOpen() error: " + std::string(fault) + "."),
                                  SilentMessage(false),
                                  true);
    }

    waveOutSetVolume( pUser->hWaveOut, MAKELONG(pSettingsManager->getCurrentSettings()->iMasterVolume,
                                                 pSettingsManager->getCurrentSettings()->iMasterVolume) );
}

void AudioService::deleteUserAudio(User *pUser)
{
    pUser->mtxUser. lock();


    for (size_t j = 0;  j < pUser->vAudioPackets.size();  j++)
    {
        if (pUser->vAudioPackets[j])
        {
            delete[] pUser->vAudioPackets[j];
        }
    }

    waveOutClose (pUser->hWaveOut);


    pUser->mtxUser. unlock();
}

void AudioService::setTestRecordingPause(bool bPause)
{
    bPauseTestInput = bPause;
}

void AudioService::recordOnPush()
{
    MMRESULT result;


    bool bError               = false;
    bool bButtonPressed       = false;
    bool bWaitForFourthBuffer = false;

    while(bInputReady)
    {
        while ( (GetAsyncKeyState(pSettingsManager->getCurrentSettings()->iPushToTalkButton) & 0x8000)
                && bInputReady
                && bMuteMic == false)
        {
            // Button pressed
            if ( (bButtonPressed == false) && (pSettingsManager->getCurrentSettings()->bPlayPushToTalkSound) )
            {
                PlaySoundW( AUDIO_PRESS_PATH, nullptr, SND_FILENAME | SND_ASYNC );
            }

            bButtonPressed = true;

            if (bWaitForFourthBuffer == false)
            {
                // Add 1st buffer
                if ( addInBuffer(&WaveInHdr1) )
                {
                    bError = true;
                    break;
                }

                // Add 2nd buffer
                if ( addInBuffer(&WaveInHdr2) )
                {
                    bError = true;
                    break;
                }

                // Add 3rd buffer
                if ( addInBuffer(&WaveInHdr3) )
                {
                    bError = true;
                    break;
                }

                // Add 4th buffer
                if ( addInBuffer(&WaveInHdr4) )
                {
                    bError = true;
                    break;
                }

                // Start recording for ('sampleCount/sampleRate' * 4) seconds
                // Current buffers queue: 1 (recording) - 2 - 3 - 4.
                result = waveInStart(hWaveIn);

                if (result)
                {
                    char fault [256];
                    memset (fault, 0, 256);

                    waveInGetErrorTextA (result, fault, 256);
                    pMainWindow->printOutput(std::string("AudioService::recordOnPress::waveInStart() error: " + std::string(fault) + "."),
                                              SilentMessage(false),
                                              true);

                    bError = true;
                    break;
                }
            }
            else
            {
                // Wait until 4th buffer finished recording.
                // 4-1-2-3.
                waitAndSendBuffer(&WaveInHdr4, pWaveIn4);

                ///////////////////////////////
                // Add 4th buffer
                // 1-2-3-(4).
                ///////////////////////////////
                if ( addInBuffer(&WaveInHdr4) )
                {
                    bError = true;
                    break;
                }
            }


            // Wait until 1st buffer finished recording.
            // 1-2-3-4.
            waitAndSendBuffer (&WaveInHdr1, pWaveIn1);


            ///////////////////////////////
            // Add 1st buffer
            // 2-3-4-(1).
            ///////////////////////////////
            if ( addInBuffer(&WaveInHdr1) )
            {
                bError = true;
                break;
            }


            // Wait until 2nd buffer finished recording.
            // 2-3-4-1.
            waitAndSendBuffer (&WaveInHdr2, pWaveIn2);


            ///////////////////
            // Add 2nd buffer.
            // 3-4-1-(2).
            ///////////////////

            if ( addInBuffer(&WaveInHdr2) )
            {
                bError = true;
                break;
            }


            // Wait until 3rd buffer finished recording.
            // 3-4-1-2.
            waitAndSendBuffer (&WaveInHdr3, pWaveIn3);


            ///////////////////
            // Add 3rd buffer.
            // 4-1-2-(3).
            ///////////////////

            if ( (GetAsyncKeyState(pSettingsManager->getCurrentSettings()->iPushToTalkButton) & 0x8000)
                 && bMuteMic == false)
            {
                if ( addInBuffer(&WaveInHdr3) )
                {
                    bError = true;
                    break;
                }

                bWaitForFourthBuffer = true;
            }
            else break;
        }



        if (bButtonPressed && bInputReady)
        {
            // Button unpressed
            bButtonPressed = false;
            bWaitForFourthBuffer = false;

            if (bError)
            {
                waitForAllInBuffers();

                bError = false;
            }

            // Wait until 4th buffer finished recording.
            // 4-1-2.
            waitAndSendBuffer (&WaveInHdr4, pWaveIn4);


            // Wait until 1st buffer finished recording.
            waitAndSendBuffer (&WaveInHdr1, pWaveIn1);


            // Wait until 2nd buffer finished recording.
            waitAndSendBuffer (&WaveInHdr2, pWaveIn2);


            waveInStop(hWaveIn);


            std::this_thread::sleep_for(std::chrono::milliseconds(10));


            pNetworkService->sendVoiceMessage(nullptr, 1, true);

            if ( pSettingsManager->getCurrentSettings()->bPushToTalkVoiceMode
                 && pSettingsManager->getCurrentSettings()->bPlayPushToTalkSound )
            {
                PlaySoundW( AUDIO_UNPRESS_PATH, nullptr, SND_FILENAME | SND_ASYNC );
            }
        }

        if (bInputReady) std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_AUDIO_RECORD_MS) );
    }
}

void AudioService::recordOnTalk()
{
    MMRESULT result;

    bool bError = false;
    bool bWaitForFourthBuffer = false;

    iPacketsNeedToRecordLeft = 4;

    bRecordedSome = false;

    while(bInputReady)
    {
        if (bWaitForFourthBuffer == false)
        {
            // Add 1st buffer
            if ( addInBuffer(&WaveInHdr1) )
            {
                break;
            }

            // Add 2nd buffer
            if ( addInBuffer(&WaveInHdr2) )
            {
                bError = true;
                break;
            }

            // Add 3rd buffer
            if ( addInBuffer(&WaveInHdr3) )
            {
                bError = true;
                break;
            }

            // Add 4th buffer
            if ( addInBuffer(&WaveInHdr4) )
            {
                bError = true;
                break;
            }

            // Start recording for ('sampleCount/sampleRate' * 4) seconds
            // Current buffers queue: 1 (recording) - 2 - 3 - 4.
            result = waveInStart(hWaveIn);

            if (result)
            {
                char fault [256];
                memset (fault, 0, 256);

                waveInGetErrorTextA (result, fault, 256);
                pMainWindow->printOutput(std::string("AudioService::recordOnTalk::waveInStart() error: " + std::string(fault) + "."),
                                          SilentMessage(false),
                                          true);

                bError = true;
                break;
            }
        }
        else
        {
            // Wait until 4th buffer finished recording.
            // 4-1-2-3.
            waitAndSendBuffer(&WaveInHdr4, pWaveIn4, true);

            ///////////////////////////////
            // Add 4th buffer
            // 1-2-3-(4).
            ///////////////////////////////
            if ( addInBuffer(&WaveInHdr4) )
            {
                bError = true;
                break;
            }
        }


        // Wait until 1st buffer finished recording.
        // 1-2-3-4.
        waitAndSendBuffer (&WaveInHdr1, pWaveIn1, true);


        ///////////////////////////////
        // Add 1st buffer
        // 2-3-4-(1).
        ///////////////////////////////
        if ( addInBuffer(&WaveInHdr1) )
        {
            bError = true;
            break;
        }


        // Wait until 2nd buffer finished recording.
        // 2-3-4-1.
        waitAndSendBuffer (&WaveInHdr2, pWaveIn2, true);


        ///////////////////
        // Add 2nd buffer.
        // 3-4-1-(2).
        ///////////////////

        if ( addInBuffer(&WaveInHdr2) )
        {
            bError = true;
            break;
        }


        // Wait until 3rd buffer finished recording.
        // 3-4-1-2.
        waitAndSendBuffer (&WaveInHdr3, pWaveIn3, true);


        ///////////////////
        // Add 3rd buffer.
        // 4-1-2-(3).
        ///////////////////

        if ( addInBuffer(&WaveInHdr3) )
        {
            bError = true;
            break;
        }

        bWaitForFourthBuffer = true;
    }

    if (bError)
    {
        pMainWindow->showMessageBox(true, "The voice recording won't work.");
    }
}

void AudioService::testRecord()
{
    MMRESULT result;

    bool bError = false;
    bool bWasStarted = false;
    bool bWaitForFourthBuffer = false;

    iTestPacketsNeedToRecordLeft = 4;

    std::thread tOutputThread(&AudioService::testOutputAudio, this);
    tOutputThread.detach();


    while(bTestInputReady)
    {
        while (bPauseTestInput)
        {
            if (bWasStarted)
            {
                waitForAllTestInBuffers();

                waveInStop(hTestWaveIn);

                bWaitForFourthBuffer = false;

                bWasStarted = false;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        bWasStarted = true;

        if (bWaitForFourthBuffer == false)
        {
            // Add 1st buffer
            if ( addInBuffer(&TestWaveInHdr1, true) )
            {
                break;
            }

            // Add 2nd buffer
            if ( addInBuffer(&TestWaveInHdr2, true) )
            {
                bError = true;
                break;
            }

            // Add 3rd buffer
            if ( addInBuffer(&TestWaveInHdr3, true) )
            {
                bError = true;
                break;
            }

            // Add 4th buffer
            if ( addInBuffer(&TestWaveInHdr4, true) )
            {
                bError = true;
                break;
            }

            // Start recording for ('sampleCount/sampleRate' * 4) seconds
            // Current buffers queue: 1 (recording) - 2 - 3 - 4.
            result = waveInStart(hTestWaveIn);

            if (result)
            {
                char fault [256];
                memset (fault, 0, 256);

                waveInGetErrorTextA (result, fault, 256);
                pMainWindow->printOutput(std::string("AudioService::testRecord::waveInStart() error: " + std::string(fault) + "."),
                                          SilentMessage(false),
                                          true);

                bError = true;
                break;
            }
        }
        else
        {
            // Wait until 4th buffer finished recording.
            // 4-1-2-3.
            waitAndShowBufferVolume(&TestWaveInHdr4, pTestWaveIn4);

            ///////////////////////////////
            // Add 4th buffer
            // 1-2-3-(4).
            ///////////////////////////////
            if ( addInBuffer(&TestWaveInHdr4, true) )
            {
                bError = true;
                break;
            }
        }


        // Wait until 1st buffer finished recording.
        // 1-2-3-4.
        waitAndShowBufferVolume (&TestWaveInHdr1, pTestWaveIn1);


        ///////////////////////////////
        // Add 1st buffer
        // 2-3-4-(1).
        ///////////////////////////////
        if ( addInBuffer(&TestWaveInHdr1, true) )
        {
            bError = true;
            break;
        }


        // Wait until 2nd buffer finished recording.
        // 2-3-4-1.
        waitAndShowBufferVolume (&TestWaveInHdr2, pTestWaveIn2);


        ///////////////////
        // Add 2nd buffer.
        // 3-4-1-(2).
        ///////////////////

        if ( addInBuffer(&TestWaveInHdr2, true) )
        {
            bError = true;
            break;
        }


        // Wait until 3rd buffer finished recording.
        // 3-4-1-2.
        waitAndShowBufferVolume (&TestWaveInHdr3, pTestWaveIn3);


        ///////////////////
        // Add 3rd buffer.
        // 4-1-2-(3).
        ///////////////////

        if ( addInBuffer(&TestWaveInHdr3, true) )
        {
            bError = true;
            break;
        }

        bWaitForFourthBuffer = true;
    }

    if (bError)
    {
        pMainWindow->showMessageBox(true, "The voice volume meter in the settings window will not work.");
    }
}

bool AudioService::addInBuffer(LPWAVEHDR buffer, bool bTestDevice)
{
    MMRESULT result;


    if (bTestDevice)
    {
        result = waveInPrepareHeader(hTestWaveIn, buffer, sizeof(WAVEHDR));
    }
    else
    {
        result = waveInPrepareHeader(hWaveIn, buffer, sizeof(WAVEHDR));
    }

    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::addInBuffer::waveInPrepareHeader() error: " + std::string(fault) + "."),
                                 SilentMessage(false), true);

        return true;
    }


    // Insert a wave input buffer to waveform-audio input device
    if (bTestDevice)
    {
        result = waveInAddBuffer (hTestWaveIn, buffer, sizeof(WAVEHDR));
    }
    else
    {
        result = waveInAddBuffer (hWaveIn, buffer, sizeof(WAVEHDR));
    }

    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::addInBuffer::waveInAddBuffer() error: " + std::string(fault) + "."),
                                 SilentMessage(false), true);

        if (bTestDevice)
        {
            waveInUnprepareHeader(hTestWaveIn, buffer, sizeof(WAVEHDR));
        }
        else
        {
            waveInUnprepareHeader(hWaveIn, buffer, sizeof(WAVEHDR));
        }

        return true;
    }


    return false;
}

bool AudioService::addOutBuffer(HWAVEOUT hWaveOut, LPWAVEHDR buffer)
{
    MMRESULT result;

    result = waveOutPrepareHeader (hWaveOut, buffer, sizeof(WAVEHDR));

    if (result)
    {
        char fault [256];
        memset (fault, 0, 256);

        waveInGetErrorTextA (result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::addOutBuffer::waveOutPrepareHeader() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                  SilentMessage(false),
                                  true);

        return true;
    }


    return false;
}

void AudioService::waitAndSendBuffer(WAVEHDR *WaveInHdr, short *pWaveIn, bool bOnTalk)
{
    // Wait until buffer finished recording.

    while (waveInUnprepareHeader(hWaveIn, WaveInHdr, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }


    // Make a copy
    short* pAudioCopy = new short [ static_cast <unsigned long long> (sampleCount) ];
    std::memcpy ( pAudioCopy, pWaveIn, static_cast <unsigned long long> (sampleCount * 2) );


    // Compress and send in other thread
    if (bOnTalk)
    {
        std::thread compressThread (&AudioService::sendAudioDataOnTalk, this, pAudioCopy);
        compressThread.detach();
    }
    else
    {
        std::thread compressThread (&AudioService::sendAudioData, this, pAudioCopy);
        compressThread.detach();
    }
}

void AudioService::waitAndShowBufferVolume(WAVEHDR *WaveInHdr, short *pWaveIn)
{
    // Wait until buffer finished recording.

    while (waveInUnprepareHeader(hWaveIn, WaveInHdr, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }


    // Make a copy
    short* pAudioCopy = new short [ static_cast <unsigned long long> (sampleCount) ];
    std::memcpy ( pAudioCopy, pWaveIn, static_cast <unsigned long long> (sampleCount * 2) );


    // Compress and send in other thread
    std::thread compressThread (&AudioService::sendAudioDataVolume, this, pAudioCopy);
    compressThread.detach();
}

void AudioService::waitForAllInBuffers()
{
    while (waveInUnprepareHeader(hWaveIn, &WaveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }
    while (waveInUnprepareHeader(hWaveIn, &WaveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }
    while (waveInUnprepareHeader(hWaveIn, &WaveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }
    while (waveInUnprepareHeader(hWaveIn, &WaveInHdr4, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }
}

void AudioService::waitForAllTestInBuffers()
{
    while (waveInUnprepareHeader(hTestWaveIn, &TestWaveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }
    while (waveInUnprepareHeader(hTestWaveIn, &TestWaveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }
    while (waveInUnprepareHeader(hTestWaveIn, &TestWaveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }
    while (waveInUnprepareHeader(hTestWaveIn, &TestWaveInHdr4, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }
}

void AudioService::waitForAllTestOutBuffers()
{
    // Wait until finished playing 1
    while ( waveOutUnprepareHeader(hTestWaveOut, &TestWaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }

    // Wait until finished playing 2
    while ( waveOutUnprepareHeader(hTestWaveOut, &TestWaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }
}

void AudioService::waitForPlayToEnd(User *pUser, WAVEHDR* pWaveOutHdr, size_t& iLastPlayingPacketIndex)
{
    // Wait until finished playing buffer
    while (waveOutUnprepareHeader(pUser->hWaveOut, pWaveOutHdr, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }

    delete[] pUser->vAudioPackets[iLastPlayingPacketIndex];
    pUser->vAudioPackets[iLastPlayingPacketIndex] = nullptr;

    iLastPlayingPacketIndex++;
}

void AudioService::waitForPlayOnTestToEnd(WAVEHDR *pWaveOutHdr)
{
    // Wait until finished playing buffer
    while (waveOutUnprepareHeader(hTestWaveOut, pWaveOutHdr, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }
}


void AudioService::sendAudioData(short *pAudio)
{
    if (iAudioInputVolume != 100)
    {
        float fInputMult = iAudioInputVolume / 100.0f;

        for (int t = 0;  t < sampleCount;  t++)
        {
            int iNewValue = static_cast <int> (pAudio[t] * fInputMult);

            if      (iNewValue > SHRT_MAX)
            {
                pAudio[t] = SHRT_MAX;
            }
            else if (iNewValue < SHRT_MIN)
            {
                pAudio[t] = SHRT_MIN;
            }
            else
            {
                pAudio[t] = static_cast <short> (iNewValue);
            }
        }
    }


    char* pAudioSamples = new char     [ static_cast <size_t> (sampleCount * 2) ];
    std::memcpy ( pAudioSamples, pAudio, static_cast <size_t> (sampleCount * 2) );


    delete[] pAudio;


    pNetworkService->sendVoiceMessage( pAudioSamples, sampleCount * 2, false );
}

void AudioService::sendAudioDataOnTalk(short *pAudio)
{
    if (iAudioInputVolume != 100)
    {
        float fInputMult = iAudioInputVolume / 100.0f;

        for (int t = 0;  t < sampleCount;  t++)
        {
            int iNewValue = static_cast <int> (pAudio[t] * fInputMult);

            if      (iNewValue > SHRT_MAX)
            {
                pAudio[t] = SHRT_MAX;
            }
            else if (iNewValue < SHRT_MIN)
            {
                pAudio[t] = SHRT_MIN;
            }
            else
            {
                pAudio[t] = static_cast <short> (iNewValue);
            }
        }
    }

    double maxDBFS = -1000.0;

    for (int t = 0;  t < sampleCount;  t++)
    {
        int iNewValue = static_cast <int> (pAudio[t] * fMasterVolumeMult);

        if      (iNewValue > SHRT_MAX)
        {
            pAudio[t] = SHRT_MAX;
        }
        else if (iNewValue < SHRT_MIN)
        {
            pAudio[t] = SHRT_MIN;
        }
        else
        {
            double sampleInRange = static_cast<double>(iNewValue) / SHRT_MAX;

            double sampleInDBFS = 20 * log10(abs(sampleInRange));

            if (sampleInDBFS > maxDBFS)
            {
                maxDBFS = sampleInDBFS;
            }
        }
    }


    if (iPacketsNeedToRecordLeft == 0)
    {
        iPacketsNeedToRecordLeft = 4;
    }

    if ( iPacketsNeedToRecordLeft != 4 )
    {
        bRecordTalk = true;
        bRecordedSome = true;
        iPacketsNeedToRecordLeft--;
    }
    else if ( (static_cast<int>(maxDBFS) >= iLowerVoiceStartRecValueInDBFS) && bMuteMic == false )
    {
        bRecordTalk = true;
        bRecordedSome = true;
        iPacketsNeedToRecordLeft--;
    }
    else
    {
        bRecordTalk = false;

        if (bRecordedSome)
        {
            bRecordedSome = false;

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            pNetworkService->sendVoiceMessage(nullptr, 1, true);
        }
    }


    if (bRecordTalk)
    {
        char* pAudioSamples = new char     [ static_cast <size_t> (sampleCount * 2) ];
        std::memcpy ( pAudioSamples, pAudio, static_cast <size_t> (sampleCount * 2) );


        delete[] pAudio;


        pNetworkService->sendVoiceMessage( pAudioSamples, sampleCount * 2, false );
    }
    else
    {
        delete[] pAudio;
    }
}

void AudioService::sendAudioDataVolume(short *pAudio)
{
    bool bInDBFS = true; //do not change this thing please

    // Set volume.
    float fInputMult = fMasterVolumeMult;

    if (bInDBFS == false)
    {
        fInputMult += 3.0f;
    }

    for (int t = 0;  t < sampleCount;  t++)
    {
        int iNewValue = static_cast <int> (pAudio[t] * fInputMult);

        if      (iNewValue > SHRT_MAX)
        {
            pAudio[t] = SHRT_MAX;
        }
        else if (iNewValue < SHRT_MIN)
        {
            pAudio[t] = SHRT_MIN;
        }
        else
        {
            pAudio[t] = static_cast <short> (iNewValue);
        }
    }

    if (iAudioInputVolume != 100)
    {
        float fInputMult = iAudioInputVolume / 100.0f;

        for (int t = 0;  t < sampleCount;  t++)
        {
            int iNewValue = static_cast <int> (pAudio[t] * fInputMult);

            if      (iNewValue > SHRT_MAX)
            {
                pAudio[t] = SHRT_MAX;
            }
            else if (iNewValue < SHRT_MIN)
            {
                pAudio[t] = SHRT_MIN;
            }
            else
            {
                pAudio[t] = static_cast <short> (iNewValue);
            }
        }
    }

    // Find max volume.

    short maxVolume = 0;
    double maxDBFS = -1000.0;

    for (int i = 0; i < sampleCount; i++)
    {
        if (bInDBFS)
        {
            double sampleInRange = static_cast<double>(pAudio[i]) / SHRT_MAX;

            double sampleInDBFS = 20 * log10(abs(sampleInRange));

            if (sampleInDBFS > maxDBFS)
            {
                maxDBFS = sampleInDBFS;
            }
        }
        else
        {
            if (abs(pAudio[i]) > abs(maxVolume))
            {
                maxVolume = static_cast<short>(abs(pAudio[i]));
            }
        }
    }

    if (bInDBFS)
    {
        pMainWindow->showVoiceVolumeValueInSettings(static_cast<int>(maxDBFS));
    }
    else
    {
        pMainWindow->showVoiceVolumeValueInSettings(static_cast<int>(static_cast<float>(maxVolume) / SHRT_MAX * 100));
    }


    if (pSettingsManager->getCurrentSettings()->bHearVoiceInSettings && bOutputTestVoice)
    {
        if (iTestPacketsNeedToRecordLeft == 0)
        {
            iTestPacketsNeedToRecordLeft = 4;
        }

        if ( (static_cast<int>(maxDBFS) >= iLowerVoiceStartRecValueInDBFS)
             ||
             (iTestPacketsNeedToRecordLeft != 4) )
        {
            mtxAudioPacketsForTest.lock();

            vAudioPacketsForTest.push_back(pAudio);

            iTestPacketsNeedToRecordLeft--;

            mtxAudioPacketsForTest.unlock();
        }
    }
}

void AudioService::testOutputAudio()
{
    // Audio buffer1
    TestWaveOutHdr1.dwBufferLength  = static_cast <unsigned long> (sampleCount * 2);
    TestWaveOutHdr1.dwBytesRecorded = 0;
    TestWaveOutHdr1.dwUser          = 0L;
    TestWaveOutHdr1.dwFlags         = 0L;
    TestWaveOutHdr1.dwLoops         = 0L;

    // Audio buffer2
    TestWaveOutHdr2 = TestWaveOutHdr1;

    // Start output device
    MMRESULT result = waveOutOpen( &hTestWaveOut,  WAVE_MAPPER,  &Format,  0L,  0L,  WAVE_FORMAT_DIRECT );

    if (result)
    {
        char fault [256];
        memset (fault, 0, 256);

        waveInGetErrorTextA (result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::testOutputAudio::waveOutOpen() error: " + std::string(fault) + "."),
                                  SilentMessage(false),
                                  true);
    }

    waveOutSetVolume( hTestWaveOut, MAKELONG(pSettingsManager->getCurrentSettings()->iMasterVolume,
                                             pSettingsManager->getCurrentSettings()->iMasterVolume) );



    bool bWasStarted = false;
    size_t iCurrentAudioPacketIndex = 0;

    while (bTestInputReady)
    {
        while (bPauseTestInput || bOutputTestVoice == false)
        {
            if (bWasStarted)
            {
                waitForAllTestOutBuffers();

                waveOutReset(hTestWaveOut);

                mtxAudioPacketsForTest.lock();

                for (size_t i = 0; i < vAudioPacketsForTest.size(); i++)
                {
                    delete[] vAudioPacketsForTest[i];
                }

                vAudioPacketsForTest.clear();

                iCurrentAudioPacketIndex = 0;

                mtxAudioPacketsForTest.unlock();

                bWasStarted = false;

                waveOutSetVolume( hTestWaveOut, MAKELONG(pSettingsManager->getCurrentSettings()->iMasterVolume,
                                                         pSettingsManager->getCurrentSettings()->iMasterVolume) );
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        while (iCurrentAudioPacketIndex + 3 >= vAudioPacketsForTest.size())
        {
            if (vAudioPacketsForTest.size() >= 4)
            {
                waitForAllTestOutBuffers();

                waveOutReset(hTestWaveOut);

                mtxAudioPacketsForTest.lock();

                for (size_t i = 0; i < vAudioPacketsForTest.size(); i++)
                {
                    delete[] vAudioPacketsForTest[i];
                }

                vAudioPacketsForTest.clear();

                iCurrentAudioPacketIndex = 0;

                mtxAudioPacketsForTest.unlock();

                waveOutSetVolume( hTestWaveOut, MAKELONG(pSettingsManager->getCurrentSettings()->iMasterVolume,
                                                         pSettingsManager->getCurrentSettings()->iMasterVolume) );
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        bWasStarted = true;


        // Add buffer1
        TestWaveOutHdr1.lpData = reinterpret_cast<LPSTR>( vAudioPacketsForTest[iCurrentAudioPacketIndex] );
        if ( addOutBuffer(hTestWaveOut, &TestWaveOutHdr1) ) {break;}
        iCurrentAudioPacketIndex++;



        // Add buffer2
        TestWaveOutHdr2.lpData = reinterpret_cast<LPSTR>( vAudioPacketsForTest[iCurrentAudioPacketIndex] );
        if ( addOutBuffer(hTestWaveOut, &TestWaveOutHdr2) ) {break;}
        iCurrentAudioPacketIndex++;


        // Play buffer1
        result = waveOutWrite(hTestWaveOut, &TestWaveOutHdr1, sizeof(WAVEHDR));
        if (result)
        {
            char fault[256];
            memset(fault, 0, 256);

            waveInGetErrorTextA(result, fault, 256);
            pMainWindow->printOutput(std::string("AudioService::testOutputAudio::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                     SilentMessage(false),
                                     true);

            waveOutUnprepareHeader(hTestWaveOut, &TestWaveOutHdr1, sizeof(WAVEHDR));

            break;
        }


        // Play buffer2
        result = waveOutWrite(hTestWaveOut, &TestWaveOutHdr2, sizeof(WAVEHDR));
        if (result)
        {
            char fault[256];
            memset(fault, 0, 256);

            waveInGetErrorTextA(result, fault, 256);
            pMainWindow->printOutput(std::string("AudioService::testOutputAudio::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                     SilentMessage(false),
                                     true);

            waveOutUnprepareHeader(hTestWaveOut, &TestWaveOutHdr2, sizeof(WAVEHDR));

            break;
        }



        // Wait until finished playing 1st buffer
        waitForPlayOnTestToEnd(&TestWaveOutHdr1);


        bool bWaitForSecondBuffer = true;
        bool bError = false;

        do
        {
            if (bWaitForSecondBuffer)
            {
                // Wait until finished playing 2st buffer
                while ( (waveOutUnprepareHeader(hTestWaveOut, &TestWaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                        &&
                        !(iCurrentAudioPacketIndex < vAudioPacketsForTest.size()) )
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS / 2));
                }
            }
            else
            {
                // Wait until finished playing 1st buffer
                while ( (waveOutUnprepareHeader(hTestWaveOut, &TestWaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                        &&
                        !(iCurrentAudioPacketIndex < vAudioPacketsForTest.size()) )
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS / 2));
                }
            }


            if ( iCurrentAudioPacketIndex < vAudioPacketsForTest.size() )
            {
                if (bWaitForSecondBuffer)
                {
                    // Add 1st buffer
                    TestWaveOutHdr1.lpData = reinterpret_cast<LPSTR>( vAudioPacketsForTest[iCurrentAudioPacketIndex] );
                    if ( addOutBuffer(hTestWaveOut, &TestWaveOutHdr1) ) {bError = true; break;}
                    iCurrentAudioPacketIndex++;


                    // Play 1.
                    result = waveOutWrite(hTestWaveOut, &TestWaveOutHdr1, sizeof(WAVEHDR));
                    if (result)
                    {
                        char fault[256];
                        memset(fault, 0, 256);

                        waveInGetErrorTextA(result, fault, 256);
                        pMainWindow->printOutput(std::string("AudioService::testOutputAudio::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                                 SilentMessage(false),
                                                 true);

                        waveOutUnprepareHeader(hTestWaveOut, &TestWaveOutHdr1, sizeof(WAVEHDR));

                        bError = true;

                        break;
                    }


                    // Wait until finished playing 2st buffer
                    waitForPlayOnTestToEnd(&TestWaveOutHdr2);
                }
                else
                {
                    // Add 2st buffer
                    TestWaveOutHdr2.lpData = reinterpret_cast<LPSTR>( vAudioPacketsForTest[iCurrentAudioPacketIndex] );
                    if ( addOutBuffer(hTestWaveOut, &TestWaveOutHdr2) ) {bError = true; break;}
                    iCurrentAudioPacketIndex++;


                    // Play 2.
                    result = waveOutWrite(hTestWaveOut, &TestWaveOutHdr2, sizeof(WAVEHDR));
                    if (result)
                    {
                        char fault[256];
                        memset(fault, 0, 256);

                        waveInGetErrorTextA(result, fault, 256);
                        pMainWindow->printOutput(std::string("AudioService::testOutputAudio::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                                 SilentMessage(false),
                                                 true);

                        waveOutUnprepareHeader(hTestWaveOut, &TestWaveOutHdr2, sizeof(WAVEHDR));

                        bError = true;

                        break;
                    }


                    // Wait until finished playing 1st buffer
                    waitForPlayOnTestToEnd(&TestWaveOutHdr1);
                }

                bWaitForSecondBuffer = !bWaitForSecondBuffer;
            }

        }while ( iCurrentAudioPacketIndex < vAudioPacketsForTest.size() );

        if (bError)
        {
            break;
        }
    }

    waitForAllTestOutBuffers();

    mtxAudioPacketsForTest.lock();

    for (size_t i = 0; i < vAudioPacketsForTest.size(); i++)
    {
        delete[] vAudioPacketsForTest[i];
    }

    vAudioPacketsForTest.clear();

    mtxAudioPacketsForTest.unlock();
}

void AudioService::playAudioData(short int *pAudio, std::string sUserName, bool bLast)
{
    if (bInputReady == false)
    {
        delete[] pAudio;
        return;
    }

    pNetworkService->getOtherUsersMutex()->lock();

    User* pUser = nullptr;

    for (size_t i = 0;   i < pNetworkService->getOtherUsersVectorSize();   i++)
    {
        if (pNetworkService->getOtherUser(i)->sUserName == sUserName)
        {
            pUser = pNetworkService->getOtherUser(i);
        }
    }

    pNetworkService->getOtherUsersMutex()->unlock();

    if (pUser == nullptr)
    {
        delete[] pAudio;

        return;
    }



    if (bLast)
    {
        if (pUser->bDeletePacketsAtLast)
        {
            // Something went wrong, delete all packets

            for (size_t j = 0;  j < pUser->vAudioPackets.size();  j++)
            {
                if (pUser->vAudioPackets[j])
                {
                    delete[] pUser->vAudioPackets[j];
                }
            }

            pUser->vAudioPackets.clear();

            pUser->bDeletePacketsAtLast = false;
            pUser->bPacketsArePlaying   = false;

            // Wait until finished playing
            waitForAllBuffers (pUser, false, nullptr);
        }

        pUser->bLastPacketCame = true;
    }
    else
    {
        // Set volume multiplier
        float fVolumeMult  = fMasterVolumeMult;

        if (pUser->fUserDefinedVolume != 1.0f)
        {
            fVolumeMult += ( pUser->fUserDefinedVolume - 1.0f );
        }


        // Set volume
        for (int t = 0;  t < sampleCount;  t++)
        {
            int iNewValue = static_cast <int> (pAudio[t] * fVolumeMult);

            if      (iNewValue > SHRT_MAX)
            {
                pAudio[t] = SHRT_MAX;
            }
            else if (iNewValue < SHRT_MIN)
            {
                pAudio[t] = SHRT_MIN;
            }
            else
            {
                pAudio[t] = static_cast <short> (iNewValue);
            }
        }




        pUser->vAudioPackets.push_back(pAudio);



        if ( (pUser->vAudioPackets.size() > 1)
             &&
             (pUser->bPacketsArePlaying == false) )
        {
            pUser->bPacketsArePlaying = true;

            std::thread playThread (&AudioService::play, this, pUser);
            playThread.detach();
        }
        else if (pUser->vAudioPackets.size() == 1)
        {
            pUser->bLastPacketCame = false;
        }
    }
}

void AudioService::play(User* pUser)
{
    pUser->mtxUser.lock();


    MMRESULT result;

    size_t i = 0;
    size_t iLastPlayingPacketIndex = 0;


    // Wait a little so when we finished playing the first buffer
    // we will almost 100% have a new audio packet came.
    std::this_thread::sleep_for(std::chrono::milliseconds(30));


    pUser      ->bTalking = true;
    pMainWindow->setPingAndTalkingToUser(pUser->pListWidgetItem, pUser->iPing, pUser->bTalking);



    // Add buffer1
    pUser->WaveOutHdr1.lpData = reinterpret_cast<LPSTR>( pUser->vAudioPackets[i] );
    if ( addOutBuffer(pUser->hWaveOut, &pUser->WaveOutHdr1) ) {pUser->bDeletePacketsAtLast = true;}
    i++;



    // Add buffer2
    pUser->WaveOutHdr2.lpData = reinterpret_cast<LPSTR>( pUser->vAudioPackets[i] );
    if ( addOutBuffer(pUser->hWaveOut, &pUser->WaveOutHdr2) ) {pUser->bDeletePacketsAtLast = true;}
    i++;


    // Play buffer1
    result = waveOutWrite(pUser->hWaveOut, &pUser->WaveOutHdr1, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::play::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                 SilentMessage(false),
                                 true);

        waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr1, sizeof(WAVEHDR));

        pUser->bDeletePacketsAtLast = true;
    }


    // Play buffer2
    result = waveOutWrite(pUser->hWaveOut, &pUser->WaveOutHdr2, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::play::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                 SilentMessage(false),
                                 true);

        waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr2, sizeof(WAVEHDR));

        pUser->bDeletePacketsAtLast = true;
    }



    // Wait until finished playing 1st buffer
    waitForPlayToEnd(pUser, &pUser->WaveOutHdr1, iLastPlayingPacketIndex);


    bool bWaitForSecondBuffer = true;


    do
    {
        if (bWaitForSecondBuffer)
        {
            // Wait until finished playing 2st buffer
            while ( (waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                    &&
                    !(i < pUser->vAudioPackets.size()) )
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS / 2));
            }
        }
        else
        {
            // Wait until finished playing 1st buffer
            while ( (waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                    &&
                    !(i < pUser->vAudioPackets.size()) )
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS / 2));
            }
        }


        if ( i < pUser->vAudioPackets.size() )
        {
            if (bWaitForSecondBuffer)
            {
                // Add 1st buffer
                pUser->WaveOutHdr1.lpData = reinterpret_cast<LPSTR>( pUser->vAudioPackets[i] );
                if ( addOutBuffer(pUser->hWaveOut, &pUser->WaveOutHdr1) ) {pUser->bDeletePacketsAtLast = true;}
                i++;


                // Play 1.
                result = waveOutWrite(pUser->hWaveOut, &pUser->WaveOutHdr1, sizeof(WAVEHDR));
                if (result)
                {
                    char fault[256];
                    memset(fault, 0, 256);

                    waveInGetErrorTextA(result, fault, 256);
                    pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                             SilentMessage(false),
                                             true);

                    waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr1, sizeof(WAVEHDR));

                    pUser->bDeletePacketsAtLast = true;

                    break;
                }


                // Wait until finished playing 2st buffer
                waitForPlayToEnd(pUser, &pUser->WaveOutHdr2, iLastPlayingPacketIndex);
            }
            else
            {
                // Add 2st buffer
                pUser->WaveOutHdr2.lpData = reinterpret_cast<LPSTR>( pUser->vAudioPackets[i] );
                if ( addOutBuffer(pUser->hWaveOut, &pUser->WaveOutHdr2) ) {pUser->bDeletePacketsAtLast = true;}
                i++;


                // Play 2.
                result = waveOutWrite(pUser->hWaveOut, &pUser->WaveOutHdr2, sizeof(WAVEHDR));
                if (result)
                {
                    char fault[256];
                    memset(fault, 0, 256);

                    waveInGetErrorTextA(result, fault, 256);
                    pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                             SilentMessage(false),
                                             true);

                    waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr2, sizeof(WAVEHDR));

                    pUser->bDeletePacketsAtLast = true;

                    break;
                }


                // Wait until finished playing 1st buffer
                waitForPlayToEnd(pUser, &pUser->WaveOutHdr1, iLastPlayingPacketIndex);
            }


            bWaitForSecondBuffer = !bWaitForSecondBuffer;
        }
        else
        {
            delete[] pUser->vAudioPackets[iLastPlayingPacketIndex];
            pUser->vAudioPackets[iLastPlayingPacketIndex] = nullptr;

            iLastPlayingPacketIndex++;

            break;
        }

    }while ( bInputReady );


    if (bInputReady)
    {
        // Wait until finished playing
        waitForAllBuffers (pUser, true, &iLastPlayingPacketIndex);


        pUser      ->bTalking = false;
        pMainWindow->setPingAndTalkingToUser(pUser->pListWidgetItem, pUser->iPing, pUser->bTalking);


        if (pUser->bDeletePacketsAtLast == false)
        {
            // No errors, delete here.

            for (size_t i = 0;  i < pUser->vAudioPackets.size();  i++)
            {
                if (pUser->vAudioPackets[i])
                {
                    delete[] pUser->vAudioPackets[i];
                }
            }

            pUser->vAudioPackets.clear ();
            pUser->bPacketsArePlaying = false;
        }
        else if (pUser->bLastPacketCame)
        {
            // An error occured but last packet already came-> delete here and not in playAudioData().

            for (size_t i = 0;  i < pUser->vAudioPackets.size();  i++)
            {
                if (pUser->vAudioPackets[i])
                {
                    delete[] pUser->vAudioPackets[i];
                }
            }

            pUser->vAudioPackets.clear ();
            pUser->bPacketsArePlaying   = false;
            pUser->bDeletePacketsAtLast = false;
        }
    }
    else
    {
        for (size_t i = 0;  i < pUser->vAudioPackets.size();  i++)
        {
            if (pUser->vAudioPackets[i])
            {
                delete[] pUser->vAudioPackets[i];
            }
        }

        pUser->vAudioPackets.clear ();
        pUser->bPacketsArePlaying   = false;
        pUser->bDeletePacketsAtLast = false;
    }



    pUser->mtxUser.unlock();
}

void AudioService::waitForAllBuffers(User* pUser, bool bClearPackets, size_t* iLastPlayingPacketIndex)
{
    // Wait until finished playing 1
    while ( waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }

    // Wait until finished playing 2
    while ( waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }

    if (bClearPackets)
    {
        for (size_t i = *iLastPlayingPacketIndex;   i < pUser->vAudioPackets.size();   i++)
        {
            delete[] pUser->vAudioPackets[i];
            pUser->vAudioPackets[i] = nullptr;
        }
    }
}

int AudioService::getInputDeviceID(std::wstring sDeviceName)
{
    UINT iInDeviceCount = waveInGetNumDevs();

    for (UINT i = 0; i < iInDeviceCount; i++)
    {
        WAVEINCAPS deviceInfo;

        MMRESULT result = waveInGetDevCaps(i, &deviceInfo, sizeof(deviceInfo));
        if (result)
        {
            char fault [256];
            memset (fault, 0, 256);

            waveInGetErrorTextA (result, fault, 256);
            pMainWindow->printOutput (std::string("AudioService::getInputDevices::waveInGetDevCaps() error: " + std::string(fault) + "."),
                                       SilentMessage(false),
                                       true);
           continue;
        }
        else
        {
            if (sDeviceName == std::wstring(deviceInfo.szPname))
            {
                return static_cast<int>(i);
            }
        }
    }

    return -1;
}

void AudioService::stop()
{
    if (bInputReady)
    {
        bInputReady = false;

        // Wait for record to stop.
        std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_AUDIO_RECORD_MS * 2) );

        waitForAllInBuffers();

        waveInClose(hWaveIn);
    }


    if (pWaveIn1 != nullptr)
    {
        delete[] pWaveIn1;
        pWaveIn1 = nullptr;
    }

    if (pWaveIn2 != nullptr)
    {
        delete[] pWaveIn2;
        pWaveIn2 = nullptr;
    }

    if (pWaveIn3 != nullptr)
    {
        delete[] pWaveIn3;
        pWaveIn3 = nullptr;
    }

    if (pWaveIn4 != nullptr)
    {
        delete[] pWaveIn4;
        pWaveIn4 = nullptr;
    }


    for (size_t i = 0;   i < pNetworkService->getOtherUsersVectorSize();   i++)
    {
        deleteUserAudio( pNetworkService->getOtherUser(i) );
    }
}

void AudioService::setInputAudioVolume(int iVolume)
{
    iAudioInputVolume = iVolume;
}

void AudioService::setVoiceStartValue(int iValue)
{
    iLowerVoiceStartRecValueInDBFS = iValue;
}

void AudioService::setShouldHearTestVoice(bool bHear)
{
    bOutputTestVoice = bHear;
}





AudioService::~AudioService()
{
    stop ();

    bTestInputReady = false;
    bPauseTestInput = false;

    if (pTestWaveIn1 != nullptr)
    {
        delete[] pTestWaveIn1;
        pTestWaveIn1 = nullptr;
    }

    if (pTestWaveIn2 != nullptr)
    {
        delete[] pTestWaveIn2;
        pTestWaveIn2 = nullptr;
    }

    if (pTestWaveIn3 != nullptr)
    {
        delete[] pTestWaveIn3;
        pTestWaveIn3 = nullptr;
    }

    if (pTestWaveIn4 != nullptr)
    {
        delete[] pTestWaveIn4;
        pTestWaveIn4 = nullptr;
    }

    waitForAllTestInBuffers();

    waveInClose(hTestWaveIn);

    waveOutClose(hTestWaveOut);
}

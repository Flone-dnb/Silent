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
    this ->pMainWindow      = pMainWindow;
    this ->pSettingsManager = pSettingsManager;


    // Do not record audio now
    bInputReady             = false;


    // "In" (record) buffers
    pWaveIn1                = nullptr;
    pWaveIn2                = nullptr;
    pWaveIn3                = nullptr;
    pWaveIn4                = nullptr;


    // All audio will be x1.45 volume
    // Because waveOutVolume() does not make it loud enough
    fMasterVolumeMult       = 1.45f;
}




void AudioService::setNetworkService(NetworkService *pNetworkService)
{
    this ->pNetworkService = pNetworkService;
}

float AudioService::getUserCurrentVolume(const std::string &sUserName)
{
    pNetworkService ->getOtherUsersMutex() ->lock();


    float fUserVolume = 0.0f;

    for (size_t i = 0;   i < pNetworkService ->getOtherUsersVectorSize();   i++)
    {
        if ( pNetworkService ->getOtherUser(i) ->sUserName == sUserName )
        {
            fUserVolume = pNetworkService ->getOtherUser(i) ->fUserDefinedVolume;

            break;
        }
    }



    pNetworkService ->getOtherUsersMutex() ->unlock();


    return fUserVolume;
}

void AudioService::setNewMasterVolume(unsigned short int iVolume)
{
    pNetworkService ->getOtherUsersMutex() ->lock();


    for (size_t i = 0;  i < pNetworkService ->getOtherUsersVectorSize();  i++)
    {
        waveOutSetVolume( pNetworkService ->getOtherUser(i) ->hWaveOut, MAKELONG(iVolume, iVolume) );
    }


    pNetworkService ->getOtherUsersMutex() ->unlock();
}

void AudioService::setNewUserVolume(std::string sUserName, float fVolume)
{
    User* pUser = nullptr;

    pNetworkService ->getOtherUsersMutex() ->lock();



    for (size_t i = 0;   i < pNetworkService ->getOtherUsersVectorSize();   i++)
    {
        if (pNetworkService ->getOtherUser(i) ->sUserName == sUserName)
        {
            pUser = pNetworkService ->getOtherUser(i);
            break;
        }
    }

    if (pUser)
    {
       pUser ->fUserDefinedVolume = fVolume;
    }



    pNetworkService ->getOtherUsersMutex() ->unlock();
}

void AudioService::prepareForStart()
{
    pWaveIn1  = new short int [ static_cast<size_t>(sampleCount) ];
    pWaveIn2  = new short int [ static_cast<size_t>(sampleCount) ];
    pWaveIn3  = new short int [ static_cast<size_t>(sampleCount) ];
    pWaveIn4  = new short int [ static_cast<size_t>(sampleCount) ];


    // Format
    Format .wFormatTag      = WAVE_FORMAT_PCM;
    Format .nChannels       = 1;    //  '1' - mono, '2' - stereo
    Format .cbSize          = 0;
    Format .wBitsPerSample  = 16;
    Format .nSamplesPerSec  = sampleRate;
    Format .nBlockAlign     = Format .nChannels      * Format .wBitsPerSample / 8;
    Format .nAvgBytesPerSec = Format .nSamplesPerSec * Format .nChannels           * Format .wBitsPerSample / 8;


    // "In" buffers

    // Audio buffer 1
    WaveInHdr1 .lpData          = reinterpret_cast <LPSTR>         (pWaveIn1);
    WaveInHdr1 .dwBufferLength  = static_cast      <unsigned long> (sampleCount * 2);
    WaveInHdr1 .dwBytesRecorded = 0;
    WaveInHdr1 .dwUser          = 0L;
    WaveInHdr1 .dwFlags         = 0L;
    WaveInHdr1 .dwLoops         = 0L;


    // Audio buffer 2
    WaveInHdr2 = WaveInHdr1;
    WaveInHdr2 .lpData = reinterpret_cast <LPSTR> (pWaveIn2);


    // Audio buffer 3
    WaveInHdr3 = WaveInHdr1;
    WaveInHdr3 .lpData = reinterpret_cast <LPSTR> (pWaveIn3);


    // Audio buffer 4
    WaveInHdr4 = WaveInHdr1;
    WaveInHdr4 .lpData = reinterpret_cast <LPSTR> (pWaveIn4);
}

bool AudioService::start()
{
    MMRESULT result;


    // Start input device
    result = waveInOpen (&hWaveIn,  WAVE_MAPPER,  &Format,  0L,  0L,  WAVE_FORMAT_DIRECT);

    if (result)
    {
        char fault [256];
        memset (fault, 0, 256);


        waveInGetErrorTextA (result, fault, 256);
        pMainWindow ->printOutput (std::string("AudioService::start::waveInOpen() error: " + std::string(fault) + "."),
                                   SilentMessageColor(false),
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


    std::thread recordThread (&AudioService::recordOnPress, this);
    recordThread .detach ();


    return true;
}

void AudioService::playConnectDisconnectSound(bool bConnectSound)
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

void AudioService::playNewMessageSound()
{
    PlaySoundW( AUDIO_NEW_MESSAGE_PATH, nullptr, SND_FILENAME | SND_ASYNC );
}

void AudioService::playLostConnectionSound()
{
    PlaySoundW( AUDIO_LOST_CONNECTION_PATH, nullptr, SND_FILENAME );
}

void AudioService::setupUserAudio(User *pUser)
{
    pUser ->bPacketsArePlaying   = false;
    pUser ->bDeletePacketsAtLast = false;
    pUser ->bLastPacketCame      = false;
    pUser ->fUserDefinedVolume   = 1.0f;


    // Audio buffer1
    pUser ->WaveOutHdr1 .dwBufferLength  = static_cast <unsigned long> (sampleCount * 2);
    pUser ->WaveOutHdr1 .dwBytesRecorded = 0;
    pUser ->WaveOutHdr1 .dwUser          = 0L;
    pUser ->WaveOutHdr1 .dwFlags         = 0L;
    pUser ->WaveOutHdr1 .dwLoops         = 0L;

    // Audio buffer2
    pUser ->WaveOutHdr2 = pUser ->WaveOutHdr1;



    // Start output device
    MMRESULT result = waveOutOpen( &pUser ->hWaveOut,  WAVE_MAPPER,  &Format,  0L,  0L,  WAVE_FORMAT_DIRECT );

    if (result)
    {
        char fault [256];
        memset (fault, 0, 256);

        waveInGetErrorTextA (result, fault, 256);
        pMainWindow ->printOutput(std::string("AudioService::addNewUser::waveOutOpen() error: " + std::string(fault) + "."),
                                  SilentMessageColor(false),
                                  true);
    }

    waveOutSetVolume( pUser ->hWaveOut, MAKELONG(pSettingsManager ->getCurrentSettings() ->iMasterVolume,
                                                 pSettingsManager ->getCurrentSettings() ->iMasterVolume) );
}

void AudioService::deleteUserAudio(User *pUser)
{
    pUser ->mtxUser. lock();


    for (size_t j = 0;  j < pUser ->vAudioPackets .size();  j++)
    {
        if (pUser ->vAudioPackets[j])
        {
            delete[] pUser ->vAudioPackets[j];
        }
    }

    waveOutClose (pUser ->hWaveOut);


    pUser ->mtxUser. unlock();
}

void AudioService::recordOnPress()
{
    MMRESULT result;


    bool bError               = false;
    bool bButtonPressed       = false;
    bool bWaitForFourthBuffer = false;


    while(bInputReady)
    {
        while ( (GetAsyncKeyState(pSettingsManager ->getCurrentSettings() ->iPushToTalkButton) & 0x8000)
                &&
                (bInputReady) )
        {
            // Button pressed
            if ( (bButtonPressed == false) && (pSettingsManager ->getCurrentSettings() ->bPlayPushToTalkSound) )
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
                    pMainWindow ->printOutput(std::string("AudioService::recordOnPress::waveInStart() error: " + std::string(fault) + "."),
                                              SilentMessageColor(false),
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

            if ( GetAsyncKeyState(pSettingsManager ->getCurrentSettings() ->iPushToTalkButton) & 0x8000 )
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


            std::this_thread::sleep_for(std::chrono::milliseconds(10));


            pNetworkService ->sendVoiceMessage(nullptr, 1, true);

            if (pSettingsManager ->getCurrentSettings() ->bPlayPushToTalkSound)
            {
                PlaySoundW( AUDIO_UNPRESS_PATH, nullptr, SND_FILENAME | SND_ASYNC );
            }
        }

        if (bInputReady) std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_AUDIO_RECORD_MS) );
    }
}

bool AudioService::addInBuffer(LPWAVEHDR buffer)
{
    MMRESULT result;


    result = waveInPrepareHeader(hWaveIn, buffer, sizeof(WAVEHDR));

    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::addInBuffer::waveInPrepareHeader() error: " + std::string(fault) + "."),
                                 SilentMessageColor(false), true);

        return true;
    }


    // Insert a wave input buffer to waveform-audio input device
    result = waveInAddBuffer (hWaveIn, buffer, sizeof(WAVEHDR));

    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::addInBuffer::waveInAddBuffer() error: " + std::string(fault) + "."),
                                 SilentMessageColor(false), true);

        waveInUnprepareHeader(hWaveIn, buffer, sizeof(WAVEHDR));

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
        pMainWindow ->printOutput(std::string("AudioService::addOutBuffer::waveOutPrepareHeader() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                  SilentMessageColor(false),
                                  true);

        return true;
    }


    return false;
}

void AudioService::waitAndSendBuffer(WAVEHDR *WaveInHdr, short *pWaveIn)
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
    std::thread compressThread (&AudioService::sendAudioData, this, pAudioCopy);
    compressThread .detach();
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

void AudioService::waitForPlayToEnd(User *pUser, WAVEHDR* pWaveOutHdr, size_t& iLastPlayingPacketIndex)
{
    // Wait until finished playing buffer
    while (waveOutUnprepareHeader(pUser ->hWaveOut, pWaveOutHdr, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }

    delete[] pUser ->vAudioPackets[iLastPlayingPacketIndex];
    pUser ->vAudioPackets[iLastPlayingPacketIndex] = nullptr;

    iLastPlayingPacketIndex++;
}


void AudioService::sendAudioData(short *pAudio)
{
    char* pAudioSamples = new char     [ static_cast <size_t> (sampleCount * 2) ];
    std::memcpy ( pAudioSamples, pAudio, static_cast <size_t> (sampleCount * 2) );


    delete[] pAudio;


    pNetworkService ->sendVoiceMessage( pAudioSamples, sampleCount * 2, false );
}

void AudioService::playAudioData(short int *pAudio, std::string sUserName, bool bLast)
{
    if (bInputReady == false)
    {
        delete[] pAudio;
        return;
    }

    pNetworkService ->getOtherUsersMutex() ->lock();

    User* pUser = nullptr;

    for (size_t i = 0;   i < pNetworkService ->getOtherUsersVectorSize();   i++)
    {
        if (pNetworkService ->getOtherUser(i) ->sUserName == sUserName)
        {
            pUser = pNetworkService ->getOtherUser(i);
        }
    }

    pNetworkService ->getOtherUsersMutex() ->unlock();

    if (pUser == nullptr)
    {
        delete[] pAudio;

        return;
    }



    if (bLast)
    {
        if (pUser ->bDeletePacketsAtLast)
        {
            // Something went wrong, delete all packets

            for (size_t j = 0;  j < pUser ->vAudioPackets .size();  j++)
            {
                if (pUser ->vAudioPackets[j])
                {
                    delete[] pUser ->vAudioPackets[j];
                }
            }

            pUser ->vAudioPackets.clear();

            pUser ->bDeletePacketsAtLast = false;
            pUser ->bPacketsArePlaying   = false;

            // Wait until finished playing
            waitForAllBuffers (pUser, false, nullptr);
        }

        pUser ->bLastPacketCame = true;
    }
    else
    {
        // Set volume multiplier
        float fVolumeMult  = fMasterVolumeMult;

        if (pUser ->fUserDefinedVolume != 1.0f)
        {
            fVolumeMult += ( pUser ->fUserDefinedVolume - 1.0f );
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




        pUser ->vAudioPackets.push_back(pAudio);



        if ( (pUser ->vAudioPackets .size() > 1)
             &&
             (pUser ->bPacketsArePlaying == false) )
        {
            pUser ->bPacketsArePlaying = true;

            std::thread playThread (&AudioService::play, this, pUser);
            playThread .detach();
        }
        else if (pUser ->vAudioPackets .size() == 1)
        {
            pUser ->bLastPacketCame = false;
        }
    }
}

void AudioService::play(User* pUser)
{
    pUser ->mtxUser .lock();


    MMRESULT result;

    size_t i = 0;
    size_t iLastPlayingPacketIndex = 0;


    // Wait a little so when we finished playing the first buffer
    // we will almost 100% have a new audio packet came.
    std::this_thread::sleep_for(std::chrono::milliseconds(30));


    pUser       ->bTalking = true;
    pMainWindow ->setPingAndTalkingToUser(pUser ->sUserName, pUser ->pListWidgetItem, pUser ->iPing, pUser ->bTalking);



    // Add buffer1
    pUser ->WaveOutHdr1 .lpData = reinterpret_cast<LPSTR>( pUser->vAudioPackets[i] );
    if ( addOutBuffer(pUser->hWaveOut, &pUser ->WaveOutHdr1) ) {pUser->bDeletePacketsAtLast = true;}
    i++;



    // Add buffer2
    pUser ->WaveOutHdr2 .lpData = reinterpret_cast<LPSTR>( pUser->vAudioPackets[i] );
    if ( addOutBuffer(pUser->hWaveOut, &pUser ->WaveOutHdr2) ) {pUser->bDeletePacketsAtLast = true;}
    i++;


    // Play buffer1
    result = waveOutWrite(pUser->hWaveOut, &pUser ->WaveOutHdr1, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::play::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                 SilentMessageColor(false),
                                 true);

        waveOutUnprepareHeader(pUser->hWaveOut, &pUser ->WaveOutHdr1, sizeof(WAVEHDR));

        pUser->bDeletePacketsAtLast = true;
    }


    // Play buffer2
    result = waveOutWrite(pUser->hWaveOut, &pUser ->WaveOutHdr2, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::play::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                 SilentMessageColor(false),
                                 true);

        waveOutUnprepareHeader(pUser->hWaveOut, &pUser ->WaveOutHdr2, sizeof(WAVEHDR));

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
            while ( (waveOutUnprepareHeader(pUser ->hWaveOut, &pUser->WaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                    &&
                    !(i < pUser ->vAudioPackets .size()) )
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS / 2));
            }
        }
        else
        {
            // Wait until finished playing 1st buffer
            while ( (waveOutUnprepareHeader(pUser ->hWaveOut, &pUser->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                    &&
                    !(i < pUser ->vAudioPackets .size()) )
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS / 2));
            }
        }


        if ( i < pUser ->vAudioPackets .size() )
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
                                             SilentMessageColor(false),
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
                                             SilentMessageColor(false),
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
            delete[] pUser ->vAudioPackets[iLastPlayingPacketIndex];
            pUser ->vAudioPackets[iLastPlayingPacketIndex] = nullptr;

            iLastPlayingPacketIndex++;

            break;
        }

    }while ( bInputReady );


    if (bInputReady)
    {
        // Wait until finished playing
        waitForAllBuffers (pUser, true, &iLastPlayingPacketIndex);


        pUser       ->bTalking = false;
        pMainWindow ->setPingAndTalkingToUser(pUser ->sUserName, pUser ->pListWidgetItem, pUser ->iPing, pUser ->bTalking);


        if (pUser ->bDeletePacketsAtLast == false)
        {
            // No errors, delete here.

            for (size_t i = 0;  i < pUser ->vAudioPackets .size();  i++)
            {
                if (pUser ->vAudioPackets[i])
                {
                    delete[] pUser ->vAudioPackets[i];
                }
            }

            pUser ->vAudioPackets .clear ();
            pUser ->bPacketsArePlaying = false;
        }
        else if (pUser->bLastPacketCame)
        {
            // An error occured but last packet already came -> delete here and not in playAudioData().

            for (size_t i = 0;  i < pUser ->vAudioPackets .size();  i++)
            {
                if (pUser ->vAudioPackets[i])
                {
                    delete[] pUser ->vAudioPackets[i];
                }
            }

            pUser ->vAudioPackets .clear ();
            pUser ->bPacketsArePlaying   = false;
            pUser ->bDeletePacketsAtLast = false;
        }
    }
    else
    {
        for (size_t i = 0;  i < pUser ->vAudioPackets .size();  i++)
        {
            if (pUser ->vAudioPackets[i])
            {
                delete[] pUser ->vAudioPackets[i];
            }
        }

        pUser ->vAudioPackets .clear ();
        pUser ->bPacketsArePlaying   = false;
        pUser ->bDeletePacketsAtLast = false;
    }



    pUser ->mtxUser .unlock();
}

void AudioService::waitForAllBuffers(User* pUser, bool bClearPackets, size_t* iLastPlayingPacketIndex)
{
    // Wait until finished playing 1
    while ( waveOutUnprepareHeader(pUser ->hWaveOut, &pUser ->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }

    // Wait until finished playing 2
    while ( waveOutUnprepareHeader(pUser ->hWaveOut, &pUser ->WaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }

    if (bClearPackets)
    {
        for (size_t i = *iLastPlayingPacketIndex;   i < pUser ->vAudioPackets .size();   i++)
        {
            delete[] pUser ->vAudioPackets[i];
            pUser ->vAudioPackets[i] = nullptr;
        }
    }
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


    for (size_t i = 0;   i < pNetworkService ->getOtherUsersVectorSize();   i++)
    {
        deleteUserAudio( pNetworkService ->getOtherUser(i) );
    }
}





AudioService::~AudioService()
{
    stop ();
}

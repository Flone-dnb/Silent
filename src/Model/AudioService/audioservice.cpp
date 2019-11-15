#include "audioservice.h"

// STL
#include <thread>

// Custom
#include "../src/View/MainWindow/mainwindow.h"
#include "../src/Model/NetworkService/networkservice.h"
#include "../src/Model/SettingsManager/settingsmanager.h"
#include "../src/Model/SettingsManager/SettingsFile.h"


AudioService::AudioService(MainWindow* pMainWindow, SettingsManager* pSettingsManager)
{
    this ->pMainWindow      = pMainWindow;
    this ->pSettingsManager = pSettingsManager;


    // Do not record audio now
    bInputReady        = false;


    // "In" (record) buffers
    pWaveIn1           = nullptr;
    pWaveIn2           = nullptr;
    pWaveIn3           = nullptr;
    pWaveIn4           = nullptr;


    // All audio will be x1.45 volume
    // Because waveOutVolume() does not make it loud enough
    fMasterVolumeMult  = 1.45f;
}




void AudioService::setNetworkService(NetworkService *pNetworkService)
{
    this ->pNetworkService = pNetworkService;
}

void AudioService::setNewMasterVolume(unsigned short int iVolume)
{
    mtxUsersAudio .lock();

    for (size_t i = 0;  i < vUsersAudio .size();  i++)
    {
        waveOutSetVolume( vUsersAudio[i] ->hWaveOut, MAKELONG(iVolume, iVolume) );
    }

    mtxUsersAudio .unlock();
}

void AudioService::setNewUserVolume(std::string sUserName, float fVolume)
{
    mtxUsersAudio .lock();


    for (size_t i = 0;  i < vUsersAudio .size();  i++)
    {
        if (vUsersAudio[i] ->sUserName == sUserName)
        {
            vUsersAudio[i] ->fUserDefinedVolume = fVolume;
            break;
        }
    }


    mtxUsersAudio .unlock();
}

float AudioService::getUserCurrentVolume(std::string sUserName)
{
    mtxUsersAudio .lock();

    float fCurrentVolume = 1.0f;

    for (size_t i = 0;  i < vUsersAudio .size();  i++)
    {
        if (vUsersAudio[i] ->sUserName == sUserName)
        {
            fCurrentVolume = vUsersAudio[i] ->fUserDefinedVolume;
            break;
        }
    }

    mtxUsersAudio .unlock();

    return fCurrentVolume;
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

void AudioService::playSound(bool bConnectSound)
{
    if (bConnectSound)
    {
        PlaySoundW( L"sounds/connect.wav",    nullptr, SND_FILENAME | SND_ASYNC );
    }
    else
    {
        PlaySoundW( L"sounds/disconnect.wav", nullptr, SND_FILENAME | SND_ASYNC );
    }
}

void AudioService::playNewMessageSound()
{
    PlaySoundW( L"sounds/newmessage.wav", nullptr, SND_FILENAME | SND_ASYNC );
}

void AudioService::playLostConnectionSound()
{
    PlaySoundW( L"sounds/lostconnection.wav", nullptr, SND_FILENAME );
}

void AudioService::addNewUser(std::string sUserName)
{
    mtxUsersAudio .lock();


    vUsersAudio .push_back ( new UserAudioStruct() );
    vUsersAudio .back() ->sUserName            = sUserName;
    vUsersAudio .back() ->bPacketsArePlaying   = false;
    vUsersAudio .back() ->bDeletePacketsAtLast = false;
    vUsersAudio .back() ->bLastPacketCame      = false;
    vUsersAudio .back() ->fUserDefinedVolume   = 1.0f;


    // Audio buffer1
    vUsersAudio .back() ->WaveOutHdr1 .dwBufferLength  = static_cast <unsigned long> (sampleCount * 2);
    vUsersAudio .back() ->WaveOutHdr1 .dwBytesRecorded = 0;
    vUsersAudio .back() ->WaveOutHdr1 .dwUser          = 0L;
    vUsersAudio .back() ->WaveOutHdr1 .dwFlags         = 0L;
    vUsersAudio .back() ->WaveOutHdr1 .dwLoops         = 0L;

    // Audio buffer2
    vUsersAudio .back() ->WaveOutHdr2 = vUsersAudio .back() ->WaveOutHdr1;

    // Audio buffer3
    vUsersAudio .back() ->WaveOutHdr3 = vUsersAudio .back() ->WaveOutHdr1;



    // Start output device
    MMRESULT result = waveOutOpen( &vUsersAudio .back() ->hWaveOut,  WAVE_MAPPER,  &Format,  0L,  0L,  WAVE_FORMAT_DIRECT );

    if (result)
    {
        char fault [256];
        memset (fault, 0, 256);

        waveInGetErrorTextA (result, fault, 256);
        pMainWindow ->printOutput(std::string("AudioService::addNewUser::waveOutOpen() error: " + std::string(fault) + "."),
                                  SilentMessageColor(false),
                                  true);
    }

    waveOutSetVolume( vUsersAudio .back() ->hWaveOut, MAKELONG(pSettingsManager ->getCurrentSettings() ->iMasterVolume,
                                                               pSettingsManager ->getCurrentSettings() ->iMasterVolume) );


    mtxUsersAudio .unlock();
}

void AudioService::deleteUser(std::string sUserName)
{
    mtxUsersAudio .lock();


    for (size_t i = 0;  i < vUsersAudio .size();  i++)
    {
        if (vUsersAudio[i] ->sUserName == sUserName)
        {
            for (size_t j = 0;  j < vUsersAudio[i] ->vAudioPackets .size();  j++)
            {
                delete[] vUsersAudio[i] ->vAudioPackets[j];
            }

            waveOutClose (vUsersAudio[i]->hWaveOut);
            delete vUsersAudio[i];
            vUsersAudio .erase (vUsersAudio.begin() + i);
        }
    }


    mtxUsersAudio .unlock();
}

void AudioService::deleteAll()
{
    mtxUsersAudio .lock();


    for (size_t i = 0;  i < vUsersAudio .size();  i++)
    {
        for (size_t j = 0;  j < vUsersAudio[i] ->vAudioPackets .size();  j++)
        {
            delete[] vUsersAudio[i] ->vAudioPackets[j];
        }

        waveOutClose (vUsersAudio[i] ->hWaveOut);
        delete vUsersAudio[i];
    }

    vUsersAudio .clear();


    mtxUsersAudio .unlock();
}

void AudioService::recordOnPress()
{
    MMRESULT result;


    bool bError               = false;
    bool bButtonPressed       = false;
    bool bWaitForFourthBuffer = false;


    while(bInputReady)
    {
        while ( GetAsyncKeyState(pSettingsManager ->getCurrentSettings() ->iPushToTalkButton) & 0x8000 )
        {
            // Button pressed
            if (bButtonPressed == false)
            {
                PlaySoundW( L"sounds/press.wav", nullptr, SND_FILENAME | SND_ASYNC );
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




        if (bButtonPressed)
        {
            // Button unpressed
            bButtonPressed = false;
            bWaitForFourthBuffer = false;

            if (bError)
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


            pNetworkService->sendVoiceMessage(nullptr, 1, true);

            PlaySoundW( L"sounds/unpress.wav", nullptr, SND_FILENAME | SND_ASYNC );
        }

        std::this_thread::sleep_for( std::chrono::milliseconds(15) );
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


void AudioService::sendAudioData(short *pAudio)
{
    char* pAudioSamples = new char     [ static_cast <size_t> (sampleCount * 2) ];
    std::memcpy ( pAudioSamples, pAudio, static_cast <size_t> (sampleCount * 2) );


    delete[] pAudio;


    pNetworkService ->sendVoiceMessage( pAudioSamples, sampleCount * 2, false );
}

void AudioService::playAudioData(short int *pAudio, std::string sUserName, bool bLast)
{
    if (bLast)
    {
        mtxUsersAudio .lock();


        for (size_t i = 0;  i < vUsersAudio .size();  i++)
        {
            if (vUsersAudio[i] ->sUserName == sUserName)
            {
                if (vUsersAudio[i] ->bDeletePacketsAtLast)
                {
                    // Something went wrong, delete all packets

                    for (size_t j = 0;  j < vUsersAudio[i] ->vAudioPackets .size();  j++)
                    {
                        delete[] vUsersAudio[i] ->vAudioPackets[j];
                    }

                    vUsersAudio[i] ->vAudioPackets.clear();

                    vUsersAudio[i] ->bDeletePacketsAtLast = false;
                    vUsersAudio[i] ->bPacketsArePlaying   = false;

                    // Wait until finished playing
                    waitForAllBuffers (vUsersAudio[i]);
                }

                vUsersAudio[i] ->bLastPacketCame = true;

                break;
            }
        }


        mtxUsersAudio .unlock();
    }
    else
    {
        mtxUsersAudio .lock();


        for (size_t i = 0;  i < vUsersAudio .size();  i++)
        {
            if (vUsersAudio[i] ->sUserName == sUserName)
            {
                // Set volume multiplier
                float fVolumeMult  = fMasterVolumeMult;

                if (vUsersAudio[i] ->fUserDefinedVolume != 1.0f)
                {
                    fVolumeMult += ( vUsersAudio[i]->fUserDefinedVolume - 1.0f );
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

                vUsersAudio[i] ->vAudioPackets.push_back(pAudio);



                if (
                        (vUsersAudio[i] ->vAudioPackets .size() > 5)
                        &&
                        (vUsersAudio[i] ->bPacketsArePlaying == false)
                   )
                {
                    vUsersAudio[i] ->bPacketsArePlaying = true;

                    std::thread playThread (&AudioService::play, this, vUsersAudio[i]);
                    playThread .detach();
                }
                else if (vUsersAudio[i] ->vAudioPackets .size() == 1)
                {
                    vUsersAudio[i] ->bLastPacketCame = false;
                }


                break;
            }
        }

        mtxUsersAudio .unlock();
    }
}

void AudioService::play(UserAudioStruct* pUser)
{
    MMRESULT result;

    unsigned int i = 0;

    // Let's wait a little more for the remaining packages to come (if there are any). We will wait because if the user has a high ping,
    // then when we will play sound somewhere on the first cycle, it will break and the function will end because at some point due to the high ping, the package had not yet come.
    // Therefore, we will wait for another 6 packages to start the function again. Because of this, there will be a "hole" in the sound.
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    pMainWindow->setUserIsTalking(pUser->sUserName, true);

    // Add 1st buffer
    pUser->WaveOutHdr1.lpData = reinterpret_cast<LPSTR>( pUser->vAudioPackets[i] );
    if ( addOutBuffer(pUser->hWaveOut, &pUser->WaveOutHdr1) ) {pUser->bDeletePacketsAtLast = true;}
    i++;

    // Add 2nd buffer
    pUser->WaveOutHdr2.lpData = reinterpret_cast<LPSTR>( pUser->vAudioPackets[i] );
    if ( addOutBuffer(pUser->hWaveOut, &pUser->WaveOutHdr2) ) {pUser->bDeletePacketsAtLast = true;}
    i++;

    // Add 3rd buffer
    pUser->WaveOutHdr3.lpData = reinterpret_cast<LPSTR>( pUser->vAudioPackets[i] );
    if ( addOutBuffer(pUser->hWaveOut, &pUser->WaveOutHdr3) ) {pUser->bDeletePacketsAtLast = true;}
    i++;

    // So, I want the sound to be louder because with waveOutSetVolume on max it's pretty quiet (there are some mics that record sound very quietly)
    // and to make sound louder I run 2 identical sounds together so if the phases of these sounds are about the same, we will get an increase in volume.
    // It’s not really a good idea to launch 2 identical sounds one after another with a slight delay, but let's
    // just hope that they will not play in antiphase (if their phases are opposite, the sound will disappear).


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
    }


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
    }


    // Play 3.
    result = waveOutWrite(pUser->hWaveOut, &pUser->WaveOutHdr3, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                 SilentMessageColor(false),
                                 true);

        waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr3, sizeof(WAVEHDR));

        pUser->bDeletePacketsAtLast = true;
    }


    // Wait until finished playing 1st buffer
    while (waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }


    // Buffer queue: 2-3.


    while( i < pUser ->vAudioPackets .size() )
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
        }


        // Buffer queue: 2-3-1.

        // Wait until finished playing 2nd
        while (waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
        }


        if (i == pUser->vAudioPackets.size()) break;


        // Buffer queue: 3-1.

        // Add 2nd buffer
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
        }


        // Buffer queue: 3-1-2.

        // Wait until finished playing 3rd
        while (waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
        }


        if (i == pUser->vAudioPackets.size()) break;

        // Buffer queue: 1-2.

        // Add 3rd buffer
        pUser->WaveOutHdr3.lpData = reinterpret_cast<LPSTR>( pUser->vAudioPackets[i] );
        if ( addOutBuffer(pUser->hWaveOut, &pUser->WaveOutHdr3) ) {pUser->bDeletePacketsAtLast = true;}
        i++;

        // Play 3.
        result = waveOutWrite(pUser->hWaveOut, &pUser->WaveOutHdr3, sizeof(WAVEHDR));
        if (result)
        {
            char fault[256];
            memset(fault, 0, 256);

            waveInGetErrorTextA(result, fault, 256);
            pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                     SilentMessageColor(false),
                                     true);

            waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr3, sizeof(WAVEHDR));

            pUser->bDeletePacketsAtLast = true;
        }


        // Buffer queue: 1-2-3.

        // Wait until finished playing 1st buffer
        while (waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
        }
    }



    // Wait until finished playing
    waitForAllBuffers (pUser);


    pMainWindow->setUserIsTalking(pUser->sUserName, false);

    if (pUser ->bDeletePacketsAtLast == false)
    {
        mtxUsersAudio .lock();


        for (size_t i = 0;  i < pUser ->vAudioPackets .size();  i++)
        {
            delete[] pUser ->vAudioPackets[i];
        }

        pUser ->vAudioPackets .clear ();
        pUser ->bPacketsArePlaying = false;



        mtxUsersAudio .unlock ();
    }
    else if (pUser->bLastPacketCame)
    {
        mtxUsersAudio .lock ();


        for (size_t i = 0;  i < pUser ->vAudioPackets .size();  i++)
        {
            delete[] pUser ->vAudioPackets[i];
        }

        pUser ->vAudioPackets .clear ();
        pUser ->bPacketsArePlaying   = false;
        pUser ->bDeletePacketsAtLast = false;


        mtxUsersAudio .unlock ();
    }
}

void AudioService::waitForAllBuffers(UserAudioStruct* pUser)
{
    // Wait until finished playing
    while ( waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }

    // Wait until finished playing
    while ( waveOutUnprepareHeader(pUser->hWaveOut, &pUser->WaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_UPDATE_CHECK_MS));
    }
}

void AudioService::stop()
{
    if (bInputReady)
    {
        waveInClose(hWaveIn);

        bInputReady = false;
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


    deleteAll ();
}





AudioService::~AudioService()
{
    stop ();
}

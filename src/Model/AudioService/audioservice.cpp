#include "audioservice.h"

// Custom
#include "../src/View/MainWindow/mainwindow.h"
#include "../src/Model/NetworkService/networkservice.h"

#include <thread>

AudioService::AudioService(MainWindow* pMainWindow)
{
    this->pMainWindow = pMainWindow;

    bInputReady = false;

    pWaveIn1 = nullptr;
    pWaveIn2 = nullptr;
    pWaveIn3 = nullptr;
    pWaveIn4 = nullptr;

    // T button
    iPushToTalkButton = 0x54;

    // Master volume: 52428 ~ 80%
    iVolume = 52428;

    // All audio will be x1.5 volume
    // Because waveOutVolume does not make it loud enough
    fMasterVolumeMult = 1.5f;
}




void AudioService::setNetworkService(NetworkService *pNetworkService)
{
    this->pNetworkService = pNetworkService;
}

void AudioService::setPushToTalkButtonAndVolume(int key, unsigned short int volume)
{
    iPushToTalkButton = key;
    iVolume = volume;

    mtxUsersAudio.lock();

    for (unsigned int i = 0; i < usersAudio.size(); i++)
    {
        waveOutSetVolume( usersAudio[i]->hWaveOut, MAKELONG(iVolume, iVolume) );
    }

    mtxUsersAudio.unlock();
}

void AudioService::setNewUserVolume(std::string userName, float fVolume)
{
    mtxUsersAudio.lock();

    for (size_t i = 0; i < usersAudio.size(); i++)
    {
        if (usersAudio[i]->userName == userName)
        {
            usersAudio[i]->fUserDefinedVolume = fVolume;

            break;
        }
    }

    mtxUsersAudio.unlock();
}

float AudioService::getUserCurrentVolume(std::string userName)
{
    mtxUsersAudio.lock();

    float fCurrentVolume = 1.0f;

    for (size_t i = 0; i < usersAudio.size(); i++)
    {
        if (usersAudio[i]->userName == userName)
        {
            fCurrentVolume = usersAudio[i]->fUserDefinedVolume;

            break;
        }
    }

    mtxUsersAudio.unlock();

    return fCurrentVolume;
}

void AudioService::prepareForStart()
{
    pWaveIn1  = new short int[static_cast<size_t>(sampleCount)];
    pWaveIn2  = new short int[static_cast<size_t>(sampleCount)];
    pWaveIn3  = new short int[static_cast<size_t>(sampleCount)];
    pWaveIn4  = new short int[static_cast<size_t>(sampleCount)];

    Format.wFormatTag      = WAVE_FORMAT_PCM;
    //  '1' - mono, '2' - stereo
    Format.nChannels       = 1;
    Format.nSamplesPerSec  = sampleRate;
    // nSamplesPerSec * n.Channels * wBitsPerSample/8
    Format.nAvgBytesPerSec = sampleRate * 2;
    // n.Channels * wBitsPerSample/8
    Format.nBlockAlign     = 2;
    Format.wBitsPerSample  = 16;
    Format.cbSize          = 0;

    // IN BUFFERS

    // Audio buffer
    WaveInHdr1.lpData  = reinterpret_cast<LPSTR>(pWaveIn1);
    WaveInHdr1.dwBufferLength  = static_cast<unsigned long>(sampleCount * 2);
    WaveInHdr1.dwBytesRecorded = 0;
    WaveInHdr1.dwUser  = 0L;
    WaveInHdr1.dwFlags = 0L;
    WaveInHdr1.dwLoops = 0L;


    // Audio buffer2
    WaveInHdr2.lpData = reinterpret_cast<LPSTR>(pWaveIn2);
    WaveInHdr2.dwBufferLength = static_cast<unsigned long>(sampleCount * 2);
    WaveInHdr2.dwBytesRecorded = 0;
    WaveInHdr2.dwUser = 0L;
    WaveInHdr2.dwFlags = 0L;
    WaveInHdr2.dwLoops = 0L;


    // Audio buffer3
    WaveInHdr3.lpData = reinterpret_cast<LPSTR>(pWaveIn3);
    WaveInHdr3.dwBufferLength = static_cast<unsigned long>(sampleCount * 2);
    WaveInHdr3.dwBytesRecorded = 0;
    WaveInHdr3.dwUser = 0L;
    WaveInHdr3.dwFlags = 0L;
    WaveInHdr3.dwLoops = 0L;

    // Audio buffer4
    WaveInHdr4.lpData = reinterpret_cast<LPSTR>(pWaveIn4);
    WaveInHdr4.dwBufferLength = static_cast<unsigned long>(sampleCount * 2);
    WaveInHdr4.dwBytesRecorded = 0;
    WaveInHdr4.dwUser = 0L;
    WaveInHdr4.dwFlags = 0L;
    WaveInHdr4.dwLoops = 0L;
}

bool AudioService::start()
{
    MMRESULT result;

    // Start input device
    result = waveInOpen(&hWaveIn, WAVE_MAPPER, &Format, 0L, 0L, WAVE_FORMAT_DIRECT);
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::start::waveInOpen() error: " + std::string(fault) + "."), SilentMessageColor(false), true);

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

    std::thread recordThread(&AudioService::recordOnPress, this);
    recordThread.detach();

    return true;
}

void AudioService::playSound(bool bConnectSound)
{
    if (bConnectSound)
    {
        PlaySoundW(L"sounds/connect.wav", nullptr, SND_FILENAME | SND_ASYNC);
    }
    else
    {
        PlaySoundW(L"sounds/disconnect.wav", nullptr, SND_FILENAME | SND_ASYNC);
    }
}

void AudioService::playNewMessageSound()
{
    PlaySoundW(L"sounds/newmessage.wav", nullptr, SND_FILENAME | SND_ASYNC);
}

void AudioService::playLostConnectionSound()
{
    PlaySoundW(L"sounds/lostconnection.wav", nullptr, SND_FILENAME);
}

void AudioService::addNewUser(std::string userName)
{
    mtxUsersAudio.lock();

    usersAudio.push_back(new UserAudioStruct());
    usersAudio.back()->userName = userName;
    usersAudio.back()->bPacketsArePlaying   = false;
    usersAudio.back()->bDeletePacketsAtLast = false;
    usersAudio.back()->bLastPacketCame      = false;
    usersAudio.back()->fUserDefinedVolume   = 1.0f;


    // Audio buffer1
    usersAudio.back()->WaveOutHdr1.dwBufferLength  = static_cast<unsigned long>(sampleCount * 2);
    usersAudio.back()->WaveOutHdr1.dwBytesRecorded = 0;
    usersAudio.back()->WaveOutHdr1.dwUser  = 0L;
    usersAudio.back()->WaveOutHdr1.dwFlags = 0L;
    usersAudio.back()->WaveOutHdr1.dwLoops = 0L;

    // Audio buffer2
    usersAudio.back()->WaveOutHdr2.dwBufferLength = static_cast<unsigned long>(sampleCount * 2);
    usersAudio.back()->WaveOutHdr2.dwBytesRecorded = 0;
    usersAudio.back()->WaveOutHdr2.dwUser = 0L;
    usersAudio.back()->WaveOutHdr2.dwFlags = 0L;
    usersAudio.back()->WaveOutHdr2.dwLoops = 0L;

    // Audio buffer3
    usersAudio.back()->WaveOutHdr3.dwBufferLength = static_cast<unsigned long>(sampleCount * 2);
    usersAudio.back()->WaveOutHdr3.dwBytesRecorded = 0;
    usersAudio.back()->WaveOutHdr3.dwUser = 0L;
    usersAudio.back()->WaveOutHdr3.dwFlags = 0L;
    usersAudio.back()->WaveOutHdr3.dwLoops = 0L;



    // Start output device
    MMRESULT result = waveOutOpen(&usersAudio.back()->hWaveOut, WAVE_MAPPER, &Format, 0L, 0L, WAVE_FORMAT_DIRECT);
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::addNewUser::waveOutOpen() error: " + std::string(fault) + "."), SilentMessageColor(false), true);
    }

    waveOutSetVolume(usersAudio.back()->hWaveOut, MAKELONG(iVolume, iVolume));

    mtxUsersAudio.unlock();
}

void AudioService::deleteUser(std::string userName)
{
    mtxUsersAudio.lock();

    for (unsigned int i = 0; i < usersAudio.size(); i++)
    {
        if (usersAudio[i]->userName == userName)
        {
            for (unsigned int j = 0; j < usersAudio[i]->audioPackets.size(); j++)
            {
                delete[] usersAudio[i]->audioPackets[j];
            }

            waveOutClose(usersAudio[i]->hWaveOut);
            delete usersAudio[i];
            usersAudio.erase(usersAudio.begin() + i);
        }
    }

    mtxUsersAudio.unlock();
}

void AudioService::deleteAll()
{
    mtxUsersAudio.lock();

    for (unsigned int i = 0; i < usersAudio.size(); i++)
    {
        for (unsigned int j = 0; j < usersAudio[i]->audioPackets.size(); j++)
        {
            delete[] usersAudio[i]->audioPackets[j];
        }

        waveOutClose(usersAudio[i]->hWaveOut);
        delete usersAudio[i];
    }
    usersAudio.clear();

    mtxUsersAudio.unlock();
}

void AudioService::recordOnPress()
{
    MMRESULT result;

    bool bError = false;

    bool bButtonPressed = false;
    bool bWaitForFourthBuffer = false;

    while(bInputReady)
    {
        while ( GetAsyncKeyState(iPushToTalkButton) & 0x8000 )
        {
            // Button pressed
            if (bButtonPressed == false)
            {
                PlaySoundW(L"sounds/press.wav", nullptr, SND_FILENAME | SND_ASYNC);
            }
            bButtonPressed = true;

            if (bWaitForFourthBuffer == false)
            {
                // Add 1st buffer
                if ( addInBuffer(&WaveInHdr1) ) {bError = true; break;}

                // Add 2nd buffer
                if ( addInBuffer(&WaveInHdr2) ) {bError = true; break;}

                // Add 3rd buffer
                if ( addInBuffer(&WaveInHdr3) ) {bError = true; break;}

                // Add 4th buffer
                if ( addInBuffer(&WaveInHdr4) ) {bError = true; break;}

                // Start recording for ('sampleCount/sampleRate' * 4) seconds
                // Current buffers queue: 1 (recording) - 2 - 3 - 4.
                result = waveInStart(hWaveIn);
                if (result)
                {
                    char fault[256];
                    memset(fault, 0, 256);

                    waveInGetErrorTextA(result, fault, 256);
                    pMainWindow->printOutput(std::string("AudioService::recordOnPress::waveInStart() error: " + std::string(fault) + "."),
                                             SilentMessageColor(false), true);

                    bError = true;
                    break;
                }
            }
            else
            {
                // Wait until 4th buffer finished recording.
                // 4-1-2-3.
                while (waveInUnprepareHeader(hWaveIn, &WaveInHdr4, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }

                // Make a copy
                short* pAudioCopy4 = new short [static_cast<unsigned long long>(sampleCount)];
                std::memcpy(pAudioCopy4, pWaveIn4, static_cast<unsigned long long>(sampleCount * 2));

                // Compress and send in other thread
                std::thread compressThread4(&AudioService::sendAudioData, this, pAudioCopy4);
                compressThread4.detach();

                ///////////////////////////////
                // Add 4th buffer
                // 1-2-3-(4).
                ///////////////////////////////
                if ( addInBuffer(&WaveInHdr4) ) {bError = true; break;}
            }


            // Wait until 1st buffer finished recording.
            // 1-2-3-4.
            while (waveInUnprepareHeader(hWaveIn, &WaveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }
            // Make a copy
            short* pAudioCopy = new short [static_cast<unsigned long long>(sampleCount)];
            std::memcpy(pAudioCopy, pWaveIn1, static_cast<unsigned long long>(sampleCount * 2));

            // Compress and send in other thread
            std::thread compressThread(&AudioService::sendAudioData, this, pAudioCopy);
            compressThread.detach();


            ///////////////////////////////
            // Add 1st buffer
            // 2-3-4-(1).
            ///////////////////////////////
            if ( addInBuffer(&WaveInHdr1) ) {bError = true; break;}


            // Wait until 2nd buffer finished recording.
            // 2-3-4-1.
            while (waveInUnprepareHeader(hWaveIn, &WaveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }

            // Make a copy
            short* pAudioCopy2 = new short [static_cast<unsigned long long>(sampleCount)];
            std::memcpy(pAudioCopy2, pWaveIn2, static_cast<unsigned long long>(sampleCount * 2));

            // Compress and send in other thread
            std::thread compressThread2(&AudioService::sendAudioData, this, pAudioCopy2);
            compressThread2.detach();


            ///////////////////
            // Add 2nd buffer.
            // 3-4-1-(2).
            ///////////////////

            if ( addInBuffer(&WaveInHdr2) ) {bError = true; break;}

            // Wait until 3rd buffer finished recording.
            // 3-4-1-2.
            while (waveInUnprepareHeader(hWaveIn, &WaveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }
            // Make a copy
            short* pAudioCopy3 = new short [static_cast<unsigned long long>(sampleCount)];
            std::memcpy(pAudioCopy3, pWaveIn3, static_cast<unsigned long long>(sampleCount * 2));

            // Compress and send in other thread
            std::thread compressThread3(&AudioService::sendAudioData, this, pAudioCopy3);
            compressThread3.detach();


            ///////////////////
            // Add 3rd buffer.
            // 4-1-2-(3).
            ///////////////////

            if ( GetAsyncKeyState(iPushToTalkButton) & 0x8000 )
            {
                if ( addInBuffer(&WaveInHdr3) ) {bError = true; break;}

                bWaitForFourthBuffer = true;
            }
            else break;
        };




        if (bButtonPressed)
        {
            // Button unpressed
            bButtonPressed = false;
            bWaitForFourthBuffer = false;

            if (bError)
            {
                while (waveInUnprepareHeader(hWaveIn, &WaveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }
                while (waveInUnprepareHeader(hWaveIn, &WaveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }
                while (waveInUnprepareHeader(hWaveIn, &WaveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }
                while (waveInUnprepareHeader(hWaveIn, &WaveInHdr4, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }
                bError = false;
            }


            // Wait until 4th buffer finished recording.
            // 4-1-2-3.
            while (waveInUnprepareHeader(hWaveIn, &WaveInHdr4, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }

            // Make a copy
            short* pAudioCopy4 = new short [static_cast<unsigned long long>(sampleCount)];
            std::memcpy(pAudioCopy4, pWaveIn4, static_cast<unsigned long long>(sampleCount * 2));

            // Compress and send in other thread
            std::thread compressThread4(&AudioService::sendAudioData, this, pAudioCopy4);
            compressThread4.detach();




            // Wait until 1st buffer finished recording.
            while (waveInUnprepareHeader(hWaveIn, &WaveInHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }
            // Make a copy
            short* pAudioCopy = new short [static_cast<unsigned long long>(sampleCount)];
            std::memcpy(pAudioCopy, pWaveIn1, static_cast<unsigned long long>(sampleCount * 2));

            // Compress and send in other thread
            std::thread compressThread(&AudioService::sendAudioData, this, pAudioCopy);
            compressThread.detach();




            // Wait until 2nd buffer finished recording.
            while (waveInUnprepareHeader(hWaveIn, &WaveInHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }
            // Make a copy
            short* pAudioCopy2 = new short [static_cast<unsigned long long>(sampleCount)];
            std::memcpy(pAudioCopy2, pWaveIn2, static_cast<unsigned long long>(sampleCount * 2));

            // Compress and send in other thread
            std::thread compressThread2(&AudioService::sendAudioData, this, pAudioCopy2);
            compressThread2.detach();


            std::this_thread::sleep_for(std::chrono::milliseconds(10));


            pNetworkService->sendVoiceMessage(nullptr, 1, true);

            PlaySoundW(L"sounds/unpress.wav", nullptr, SND_FILENAME | SND_ASYNC);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(15));
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
    result = waveInAddBuffer(hWaveIn, buffer, sizeof(WAVEHDR));
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

    result = waveOutPrepareHeader(hWaveOut, buffer, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::addOutBuffer::waveOutPrepareHeader() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                 SilentMessageColor(false), true);

        return true;
    }

    return false;
}


void AudioService::sendAudioData(short *pAudio)
{
    char* pAudioSamples = new char[static_cast<size_t>(sampleCount * 2)];
    std::memcpy(pAudioSamples, pAudio, static_cast<size_t>(sampleCount * 2));

    delete[] pAudio;

    pNetworkService->sendVoiceMessage(pAudioSamples, sampleCount * 2, false);
}

void AudioService::playAudioData(short int *pAudio, std::string userName, bool bLast)
{
    if (bLast)
    {
        mtxUsersAudio.lock();

        for (unsigned int i = 0; i < usersAudio.size(); i++)
        {
            if (usersAudio[i]->userName == userName)
            {
                if (usersAudio[i]->bDeletePacketsAtLast)
                {
                    // Something went wrong, delete all packets

                    for (unsigned int j = 0; j < usersAudio[i]->audioPackets.size(); j++)
                    {
                        delete[] usersAudio[i]->audioPackets[j];
                    }

                    usersAudio[i]->audioPackets.clear();

                    usersAudio[i]->bDeletePacketsAtLast = false;
                    usersAudio[i]->bPacketsArePlaying   = false;

                    // Wait until finished playing
                    waitForAllBuffers(usersAudio[i]);
                }

                usersAudio[i]->bLastPacketCame = true;

                break;
            }
        }

        mtxUsersAudio.unlock();
    }
    else
    {
        mtxUsersAudio.lock();

        for (unsigned int i = 0; i < usersAudio.size(); i++)
        {
            if (usersAudio[i]->userName == userName)
            {
                // Set volume multiplier
                float fVolumeMult = fMasterVolumeMult;
                if (usersAudio[i]->fUserDefinedVolume != 1.0f)
                {
                    fVolumeMult += ( usersAudio[i]->fUserDefinedVolume - 1.0f );
                }

                // Set volume
                for (int t = 0; t < sampleCount; t++)
                {
                    int iNewValue = static_cast<int>(pAudio[t] * fVolumeMult);
                    if (iNewValue > SHRT_MAX)
                    {
                        pAudio[t] = SHRT_MAX;
                    }
                    else if (iNewValue < SHRT_MIN)
                    {
                        pAudio[t] = SHRT_MIN;
                    }
                    else
                    {
                        pAudio[t] = static_cast<short>(iNewValue);
                    }
                }

                usersAudio[i]->audioPackets.push_back(pAudio);



                if ( (usersAudio[i]->audioPackets.size() > 5) && (usersAudio[i]->bPacketsArePlaying == false) )
                {
                    usersAudio[i]->bPacketsArePlaying = true;

                    std::thread playThread(&AudioService::play, this, usersAudio[i]);
                    playThread.detach();
                }
                else if (usersAudio[i]->audioPackets.size() == 1)
                {
                    usersAudio[i]->bLastPacketCame = false;
                }

                break;
            }
        }

        mtxUsersAudio.unlock();
    }
}

void AudioService::play(UserAudioStruct* user)
{
    MMRESULT result;

    unsigned int i = 0;

    // Let's wait a little more for the remaining packages to come (if there are any). We will wait because if the user has a high ping,
    // then when we will play sound somewhere on the first cycle, it will break and the function will end because at some point due to the high ping, the package had not yet come.
    // Therefore, we will wait for another 6 packages to start the function again. Because of this, there will be a "hole" in the sound.
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    // Add 1st buffer
    user->WaveOutHdr1.lpData = reinterpret_cast<LPSTR>( user->audioPackets[i] );
    if ( addOutBuffer(user->hWaveOut, &user->WaveOutHdr1) ) {user->bDeletePacketsAtLast = true;}
    i++;

    // Add 2nd buffer
    user->WaveOutHdr2.lpData = reinterpret_cast<LPSTR>( user->audioPackets[i] );
    if ( addOutBuffer(user->hWaveOut, &user->WaveOutHdr2) ) {user->bDeletePacketsAtLast = true;}
    i++;

    // Add 3rd buffer
    user->WaveOutHdr3.lpData = reinterpret_cast<LPSTR>( user->audioPackets[i] );
    if ( addOutBuffer(user->hWaveOut, &user->WaveOutHdr3) ) {user->bDeletePacketsAtLast = true;}
    i++;

    // So, I want the sound to be louder because with waveOutSetVolume on max it's pretty quiet (there are some mics that record sound very quietly)
    // and to make sound louder I run 2 identical sounds together so if the phases of these sounds are about the same, we will get an increase in volume.
    // It’s not really a good idea to launch 2 identical sounds one after another with a slight delay, but let's
    // just hope that they will not play in antiphase (if their phases are opposite, the sound will disappear).

    // Play 1.
    result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                 SilentMessageColor(false), true);

        waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR));

        user->bDeletePacketsAtLast = true;
    }

    // Play 2.
    result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr2, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                 SilentMessageColor(false), true);

        waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr2, sizeof(WAVEHDR));

        user->bDeletePacketsAtLast = true;
    }

    // Play 3.
    result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                 SilentMessageColor(false), true);

        waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR));

        user->bDeletePacketsAtLast = true;
    }

    // Wait until finished playing 1st buffer
    while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    };


    // Buffer queue: 2-3.


    while(i < user->audioPackets.size())
    {
        // Add 1st buffer
        user->WaveOutHdr1.lpData = reinterpret_cast<LPSTR>( user->audioPackets[i] );
        if ( addOutBuffer(user->hWaveOut, &user->WaveOutHdr1) ) {user->bDeletePacketsAtLast = true;}
        i++;

        // Play 1.
        result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR));
        if (result)
        {
            char fault[256];
            memset(fault, 0, 256);

            waveInGetErrorTextA(result, fault, 256);
            pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                     SilentMessageColor(false), true);

            waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR));

            user->bDeletePacketsAtLast = true;
        }

        // Buffer queue: 2-3-1.

        // Wait until finished playing 2nd
        while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        };


        if (i == user->audioPackets.size()) break;

        // Buffer queue: 3-1.

        // Add 2nd buffer
        user->WaveOutHdr2.lpData = reinterpret_cast<LPSTR>( user->audioPackets[i] );
        if ( addOutBuffer(user->hWaveOut, &user->WaveOutHdr2) ) {user->bDeletePacketsAtLast = true;}
        i++;

        // Play 2.
        result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr2, sizeof(WAVEHDR));
        if (result)
        {
            char fault[256];
            memset(fault, 0, 256);

            waveInGetErrorTextA(result, fault, 256);
            pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                     SilentMessageColor(false), true);

            waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr2, sizeof(WAVEHDR));

            user->bDeletePacketsAtLast = true;
        }

        // Buffer queue: 3-1-2.

        // Wait until finished playing 3rd
        while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        };


        if (i == user->audioPackets.size()) break;

        // Buffer queue: 1-2.

        // Add 3rd buffer
        user->WaveOutHdr3.lpData = reinterpret_cast<LPSTR>( user->audioPackets[i] );
        if ( addOutBuffer(user->hWaveOut, &user->WaveOutHdr3) ) {user->bDeletePacketsAtLast = true;}
        i++;

        // Play 3.
        result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR));
        if (result)
        {
            char fault[256];
            memset(fault, 0, 256);

            waveInGetErrorTextA(result, fault, 256);
            pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."),
                                     SilentMessageColor(false), true);

            waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR));

            user->bDeletePacketsAtLast = true;
        }

        // Buffer queue: 1-2-3.

        // Wait until finished playing 1st buffer
        while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        };
    };



    // Wait until finished playing
    waitForAllBuffers(user);



    if (user->bDeletePacketsAtLast == false)
    {
        mtxUsersAudio.lock();


        for (unsigned int i = 0; i < user->audioPackets.size(); i++)
        {
            delete[] user->audioPackets[i];
        }

        user->audioPackets.clear();
        user->bPacketsArePlaying = false;


        mtxUsersAudio.unlock();
    }
    else if (user->bLastPacketCame)
    {
        mtxUsersAudio.lock();


        for (unsigned int i = 0; i < user->audioPackets.size(); i++)
        {
            delete[] user->audioPackets[i];
        }

        user->audioPackets.clear();
        user->bPacketsArePlaying = false;
        user->bDeletePacketsAtLast = false;


        mtxUsersAudio.unlock();
    }
}

void AudioService::waitForAllBuffers(UserAudioStruct* user)
{
    // Wait until finished playing
    while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    };

    // Wait until finished playing
    while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    };
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

    deleteAll();
}





AudioService::~AudioService()
{
    stop();
}

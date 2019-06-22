#include "audioservice.h"

// Custom
#include "View/MainWindow/mainwindow.h"
#include "Model/NetworkService/networkservice.h"

#include <thread>

AudioService::AudioService(MainWindow* pMainWindow)
{
    this->pMainWindow = pMainWindow;

    bInputReady  = false;

    pWaveIn1 = nullptr;
    pWaveIn2 = nullptr;
    pWaveIn3 = nullptr;
    pWaveIn4 = nullptr;

    // T button
    iPushToTalkButton = 0x54;

    iVolume = 65535;
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



void AudioService::start()
{
    pWaveIn1  = new short int[static_cast<unsigned long long>(sampleCount)];
    pWaveIn2  = new short int[static_cast<unsigned long long>(sampleCount)];
    pWaveIn3  = new short int[static_cast<unsigned long long>(sampleCount)];
    pWaveIn4  = new short int[static_cast<unsigned long long>(sampleCount)];

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


    MMRESULT result;

    // Start input device
    result = waveInOpen(&hWaveIn, WAVE_MAPPER, &Format, 0L, 0L, WAVE_FORMAT_DIRECT);
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::start::waveInOpen() error: " + std::string(fault) + "."), true);

        delete[] pWaveIn1;
        pWaveIn1 = nullptr;

        delete[] pWaveIn2;
        pWaveIn2 = nullptr;

        delete[] pWaveIn3;
        pWaveIn3 = nullptr;

        delete[] pWaveIn4;
        pWaveIn4 = nullptr;

        return;
    }
    else
    {
        bInputReady = true;
    }

    std::thread recordThread(&AudioService::recordOnPress, this);
    recordThread.detach();
}

void AudioService::playSound(bool bConnectSound)
{
    if (bConnectSound)
    {
        PlaySoundW(L"sounds/connect.wav", NULL, SND_FILENAME | SND_ASYNC);
    }
    else
    {
        PlaySoundW(L"sounds/disconnect.wav", NULL, SND_FILENAME | SND_ASYNC);
    }
}

void AudioService::playNewMessageSound()
{
    PlaySoundW(L"sounds/newmessage.wav", NULL, SND_FILENAME | SND_ASYNC);
}

void AudioService::addNewUser(std::string userName)
{
    mtxUsersAudio.lock();

    usersAudio.push_back(new UserAudioStruct());
    usersAudio.back()->userName = userName;
    usersAudio.back()->bPacketsArePlaying   = false;
    usersAudio.back()->bDeletePacketsAtLast = false;
    usersAudio.back()->bLastPacketCame  = false;

    // Audio buffer
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

    // Audio buffer2
    usersAudio.back()->WaveOutHdr4.dwBufferLength = static_cast<unsigned long>(sampleCount * 2);
    usersAudio.back()->WaveOutHdr4.dwBytesRecorded = 0;
    usersAudio.back()->WaveOutHdr4.dwUser = 0L;
    usersAudio.back()->WaveOutHdr4.dwFlags = 0L;
    usersAudio.back()->WaveOutHdr4.dwLoops = 0L;

    // Start output device
    MMRESULT result = waveOutOpen(&usersAudio.back()->hWaveOut, WAVE_MAPPER, &Format, 0L, 0L, WAVE_FORMAT_DIRECT);
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::addNewUser::waveOutOpen() error: " + std::string(fault) + "."), true);
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
        usersAudio.erase(usersAudio.begin() + i);
    }

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
                PlaySoundW(L"sounds/press.wav", NULL, SND_FILENAME | SND_ASYNC);
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
                    pMainWindow->printOutput(std::string("AudioService::recordOnPress::waveInStart() error: " + std::string(fault) + "."), true);

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
                std::thread compressThread4(&AudioService::compressAndSend, this, pAudioCopy4);
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
            std::thread compressThread(&AudioService::compressAndSend, this, pAudioCopy);
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
            std::thread compressThread2(&AudioService::compressAndSend, this, pAudioCopy2);
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
            std::thread compressThread3(&AudioService::compressAndSend, this, pAudioCopy3);
            compressThread3.detach();


            ///////////////////
            // Add 3rd buffer.
            // 4-1-2-(3).
            ///////////////////

            if ( addInBuffer(&WaveInHdr3) ) {bError = true; break;}


            bWaitForFourthBuffer = true;
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
            std::thread compressThread4(&AudioService::compressAndSend, this, pAudioCopy4);
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
            std::thread compressThread(&AudioService::compressAndSend, this, pAudioCopy);
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
            std::thread compressThread2(&AudioService::compressAndSend, this, pAudioCopy2);
            compressThread2.detach();




            // Wait until 3rd buffer finished recording.
            while (waveInUnprepareHeader(hWaveIn, &WaveInHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }
            // Make a copy
            short* pAudioCopy3 = new short [static_cast<unsigned long long>(sampleCount)];
            std::memcpy(pAudioCopy3, pWaveIn3, static_cast<unsigned long long>(sampleCount * 2));

            // Compress and send in other thread
            std::thread compressThread3(&AudioService::compressAndSend, this, pAudioCopy3);
            compressThread3.detach();

            std::this_thread::sleep_for(std::chrono::milliseconds(20));

            pNetworkService->sendVoiceMessage(nullptr, 1, true);

            PlaySoundW(L"sounds/unpress.wav", NULL, SND_FILENAME | SND_ASYNC);
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
        pMainWindow->printOutput(std::string("AudioService::addInBuffer::waveInPrepareHeader() error: " + std::string(fault) + "."), true);

        return true;
    }

    // Insert a wave input buffer to waveform-audio input device
    result = waveInAddBuffer(hWaveIn, buffer, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::addInBuffer::waveInAddBuffer() error: " + std::string(fault) + "."), true);

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
        pMainWindow->printOutput(std::string("AudioService::addOutBuffer::waveOutPrepareHeader() error (" + std::to_string(result) + "): " + std::string(fault) + "."), true);

        return true;
    }

    return false;
}


void AudioService::compressAndSend(short *pAudio)
{
    // For now, we will refuse to compress audio data, because of the fact that due to compression distortion appears on a loud sound.

//    char* pCompressedAudio = new char [static_cast<unsigned long long>(sampleCount)];

//    // Compress
//    for (int i = 0; i < sampleCount; i++)
//    {
//        pCompressedAudio[i] = MuLaw_Encode(pAudio[i]);
//    }


    pNetworkService->sendVoiceMessage(reinterpret_cast<char*>(pAudio), sampleCount * 2, false);

    //delete[] pAudio;
}

void AudioService::uncompressAndPlay(char *pAudio, std::string userName, bool bLast)
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

                    // Wait until finished playing 1st buffer
                    do
                    {

                    } while (waveOutUnprepareHeader(usersAudio[i]->hWaveOut, &usersAudio[i]->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);
                    // Wait until finished playing 2nd buffer
                    do
                    {

                    } while (waveOutUnprepareHeader(usersAudio[i]->hWaveOut, &usersAudio[i]->WaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);
                    // Wait until finished playing 3rd buffer
                    do
                    {

                    } while (waveOutUnprepareHeader(usersAudio[i]->hWaveOut, &usersAudio[i]->WaveOutHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);
                    // Wait until finished playing 4th buffer
                    do
                    {

                    } while (waveOutUnprepareHeader(usersAudio[i]->hWaveOut, &usersAudio[i]->WaveOutHdr4, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);
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
//                short* pUncompressedAudio = new short[static_cast<unsigned long long>(sampleCount)];
//                for (int j = 0; j < sampleCount; j++)
//                {
//                    pUncompressedAudio[j] = MuLaw_Decode(pAudio[j]);
//                }

                usersAudio[i]->audioPackets.push_back(reinterpret_cast<short*>(pAudio));

                if ( (usersAudio[i]->audioPackets.size() > 6) && (usersAudio[i]->bPacketsArePlaying == false) )
                {
                    // Starting to play from 4th audio packet
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

        //delete[] pAudio;
    }
}

void AudioService::play(UserAudioStruct* user)
{
    MMRESULT result;

    unsigned int i = 0;
    bool bFourthAdded = false;

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

    // Add 4th buffer
    user->WaveOutHdr4.lpData = reinterpret_cast<LPSTR>( user->audioPackets[i] );
    if ( addOutBuffer(user->hWaveOut, &user->WaveOutHdr4) ) {user->bDeletePacketsAtLast = true;}
    i++;

    // Play 1.
    result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."), true);

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
        pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."), true);

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
        pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."), true);

        waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR));

        user->bDeletePacketsAtLast = true;
    }

    // Play 4.
    result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr4, sizeof(WAVEHDR));
    if (result)
    {
        char fault[256];
        memset(fault, 0, 256);

        waveInGetErrorTextA(result, fault, 256);
        pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."), true);

        waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr4, sizeof(WAVEHDR));

        user->bDeletePacketsAtLast = true;
    }

    while(i < user->audioPackets.size())
    {
        // Buffers queue: 1-2-3-4.


        // Wait until finished playing 1st buffer
        while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        };

        // Add 1st buffer
        // 2-3-4-(1).

        user->WaveOutHdr1.lpData = reinterpret_cast<LPSTR>( user->audioPackets[i] );
        if ( addOutBuffer(user->hWaveOut, &user->WaveOutHdr1) ) {user->bDeletePacketsAtLast = true; break;}
        i++;

        // Play 1.
        result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR));
        if (result)
        {
            char fault[256];
            memset(fault, 0, 256);

            waveInGetErrorTextA(result, fault, 256);
            pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."), true);

            waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR));

            user->bDeletePacketsAtLast = true;
            break;
        }


        if (i < user->audioPackets.size())
        {
            // Wait until finished playing
            while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            };

            // Add 2nd buffer
            // 3-4-1-(2).

            user->WaveOutHdr2.lpData = reinterpret_cast<LPSTR>( user->audioPackets[i] );
            if ( addOutBuffer(user->hWaveOut, &user->WaveOutHdr2) ) {user->bDeletePacketsAtLast = true; break;}
            i++;

            // Play 2.
            result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr2, sizeof(WAVEHDR));
            if (result)
            {
                char fault[256];
                memset(fault, 0, 256);

                waveInGetErrorTextA(result, fault, 256);
                pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."), true);

                waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr2, sizeof(WAVEHDR));

                user->bDeletePacketsAtLast = true;
                break;
            }


            if (i < user->audioPackets.size())
            {
                // Wait until finished playing
                while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                };

                // Add 3rd buffer
                // 4-1-2-(3).

                user->WaveOutHdr3.lpData = reinterpret_cast<LPSTR>( user->audioPackets[i] );
                if ( addOutBuffer(user->hWaveOut, &user->WaveOutHdr3) ) {user->bDeletePacketsAtLast = true; break;}
                i++;

                // Play 3.
                result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR));
                if (result)
                {
                    char fault[256];
                    memset(fault, 0, 256);

                    waveInGetErrorTextA(result, fault, 256);
                    pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."), true);

                    waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR));

                    user->bDeletePacketsAtLast = true;
                    break;
                }


                if (i < user->audioPackets.size())
                {
                    // Wait until finished playing
                    while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr4, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(3));
                    };

                    // Add 4th buffer
                    // 1-2-3-(4)

                    user->WaveOutHdr4.lpData = reinterpret_cast<LPSTR>( user->audioPackets[i] );
                    if ( addOutBuffer(user->hWaveOut, &user->WaveOutHdr4) ) {user->bDeletePacketsAtLast = true; break;}
                    i++;

                    // Play 4.
                    result = waveOutWrite(user->hWaveOut, &user->WaveOutHdr4, sizeof(WAVEHDR));
                    if (result)
                    {
                        char fault[256];
                        memset(fault, 0, 256);

                        waveInGetErrorTextA(result, fault, 256);
                        pMainWindow->printOutput(std::string("AudioService::uncompressAndPlay::waveOutWrite() error (" + std::to_string(result) + "): " + std::string(fault) + "."), true);

                        waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr4, sizeof(WAVEHDR));

                        user->bDeletePacketsAtLast = true;
                        break;
                    }

                    bFourthAdded = true;
                }
                else
                {
                    bFourthAdded = false;

                    // Wait until finished playing
                    while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr4, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(3));
                    };

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

                    // Wait until finished playing
                    while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(3));
                    };
                }
            }
            else
            {
                bFourthAdded = false;

                // Wait until finished playing
                while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                };

                // Wait until finished playing
                while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr4, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                };

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
        }
        else
        {
            bFourthAdded = false;

            // Wait until finished playing
            while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr2, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            };

            // Wait until finished playing
            while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            };

            // Wait until finished playing
            while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr4, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            };

            // Wait until finished playing
            while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr1, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            };
        }
    };

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

    // Wait until finished playing
    while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr3, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    };

    // Wait until finished playing
    while (waveOutUnprepareHeader(user->hWaveOut, &user->WaveOutHdr4, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    };

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

char AudioService::MuLaw_Encode(short number)
{
    const short MULAW_MAX = 0x1FFF;
    const short MULAW_BIAS = 33;
    short mask = 0x1000;
    char sign = 0;
    char position = 12;
    char lsb = 0;
    if (number < 0)
    {
        number = -number;
        sign = 0x80;
    }
    number += MULAW_BIAS;
    if (number > MULAW_MAX)
    {
        number = MULAW_MAX;
    }
    for (; ((number & mask) != mask && position >= 5); mask >>= 1, position--)
        ;
    lsb = (number >> (position - 4)) & 0x0f;
    return (~(sign | ((position - 5) << 4) | lsb));
}

short AudioService::MuLaw_Decode(char number)
{
    const short MULAW_BIAS = 33;
    char sign = 0, position = 0;
    short decoded = 0;
    number = ~number;
    if (number & 0x80)
    {
        number &= ~(1 << 7);
        sign = -1;
    }
    position = ((number & 0xF0) >> 4) + 5;
    decoded = ((1 << position) | ((number & 0x0F) << (position - 4))
        | (1 << (position - 5))) - MULAW_BIAS;
    return (sign == 0) ? (decoded) : (-(decoded));
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
}





AudioService::~AudioService()
{
    stop();
}

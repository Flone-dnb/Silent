// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// STL
#include <string>

#define SILENT_MAGIC_NUMBER 51337
#define SILENT_SETTINGS_FILE_VERSION 1

class SettingsFile
{

public:

    SettingsFile(int iPushToTalkButton            = 0x54    /* T button */,
                 unsigned short int iMasterVolume = 52428   /* ~80% */,
                 const std::string& sThemeName    = "Default",
                 const std::string& sUserName     = "",
                 bool bPlayPushToTalkSound        = true,
                 const std::string& sConnectString= "",
                 unsigned short int iPort         = 51337,
                 const std::wstring sPassword     = L"",
                 std::wstring sInputDeviceName    = L"",
                 int iInputVolumeMultiplier       = 100,
                 bool bPushToTalkVoiceMode        = true,
                 int iVoiceStartRecValueInDBFS    = -30,
                 bool bHearVoiceInSettings        = true,
                 bool bPlayTextMessageSound       = true)
    {
        this ->iPushToTalkButton    = iPushToTalkButton;
        this ->iMasterVolume        = iMasterVolume;
        this ->sThemeName           = sThemeName;
        this ->sUsername            = sUserName;
        this ->bPlayPushToTalkSound = bPlayPushToTalkSound;
        this ->sConnectString       = sConnectString;
        this ->iPort                = iPort;
        this ->sPassword            = sPassword;
        this ->sInputDeviceName     = sInputDeviceName;
        this ->iInputVolumeMultiplier = iInputVolumeMultiplier;
        this ->bPushToTalkVoiceMode = bPushToTalkVoiceMode;
        this ->iVoiceStartRecValueInDBFS = iVoiceStartRecValueInDBFS;
        this ->bHearVoiceInSettings = bHearVoiceInSettings;
        this ->bPlayTextMessageSound = bPlayTextMessageSound;
    }


    std::string getPushToTalkButtonName() const
    {
        std::string sButtonText = "";


        if ( (iPushToTalkButton >= 0x41)
             &&
             (iPushToTalkButton <= 0x5a) )
        {
            // From A to Z
            sButtonText += static_cast <char> (iPushToTalkButton);
        }
        else if ( iPushToTalkButton == 0x12 )
        {
            // Alt
            sButtonText += "Alt";
        }
        else if ( iPushToTalkButton == 0x05 )
        {
            // X1 (mouse button)
            sButtonText += "X1";
        }
        else if ( iPushToTalkButton == 0x06 )
        {
            // X2 (mouse button)
            sButtonText += "X2";
        }


        return sButtonText;
    }


    // ------------------------------------------------------------


    std::string        sUsername;
    std::string        sThemeName;
    std::string        sConnectString;
    std::wstring       sPassword;
    std::wstring       sInputDeviceName;


    int                iPushToTalkButton;
    int                iInputVolumeMultiplier;
    int                iVoiceStartRecValueInDBFS;
    unsigned short int iMasterVolume;


    unsigned short int iPort;


    bool               bPlayPushToTalkSound;
    bool               bPushToTalkVoiceMode;
    bool               bHearVoiceInSettings;
    bool               bPlayTextMessageSound;
};

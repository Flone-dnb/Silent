// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "settingsmanager.h"


// STL
#include <fstream>
#include <codecvt>
#include <locale>

// Other
#include <shlobj.h>

// Custom
#include "View/MainWindow/mainwindow.h"
#include "Model/SettingsManager/SettingsFile.h"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


SettingsManager::SettingsManager(MainWindow* pMainWindow)
{
    this ->pMainWindow   = pMainWindow;


    bSettingsFileCreatedFirstTime = false;

    bInit = true;


    pCurrentSettingsFile = nullptr;
    pCurrentSettingsFile = readSettings();

    if (pCurrentSettingsFile && bSettingsFileCreatedFirstTime)
    {
        saveCurrentSettings();
    }

    bInit = false;
}





void SettingsManager::saveCurrentSettings()
{
    // Get the path to the Documents folder.

    TCHAR   my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathW( nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, my_documents );

    if (result != S_OK)
    {
        pMainWindow ->showMessageBox(true, "An error occurred at SettingsManager::saveCurrentSettings(). Error: "
                                           "can't open the Documents folder to read the settings.");

        delete pCurrentSettingsFile;

        return;
    }




    // Open the settings file.

    std::wstring sPathToOldSettings  = std::wstring(my_documents);
    std::wstring sPathToNewSettings  = std::wstring(my_documents);
    sPathToOldSettings  += L"\\" + std::wstring(SETTINGS_NAME);
    sPathToNewSettings  += L"\\" + std::wstring(SETTINGS_NAME) + L"~";

    std::ifstream settingsFile (sPathToOldSettings, std::ios::binary);
    std::ofstream newSettingsFile;

    if ( settingsFile .is_open() )
    {
        newSettingsFile .open(sPathToNewSettings);
    }
    else
    {
        newSettingsFile .open(sPathToOldSettings);
    }



    // Write new setting to the new file.

    // Write magic number.
    int iMagicNumber = SILENT_MAGIC_NUMBER;

    newSettingsFile .write
            ( reinterpret_cast <char*> (&iMagicNumber), sizeof(iMagicNumber) );



    // Write settings version.
    unsigned short int iSettingsVersion = SILENT_SETTINGS_FILE_VERSION;

    newSettingsFile .write
            ( reinterpret_cast <char*> (&iSettingsVersion), sizeof(iSettingsVersion) );



    // Write push-to-Talk button.
    newSettingsFile .write
            ( reinterpret_cast <char*> (&pCurrentSettingsFile ->iPushToTalkButton), sizeof(pCurrentSettingsFile ->iPushToTalkButton) );



    // Write master Volume.
    newSettingsFile .write
            ( reinterpret_cast <char*> (&pCurrentSettingsFile ->iMasterVolume),     sizeof(pCurrentSettingsFile ->iMasterVolume)     );



    // Write user name.
    char cUserNameLength = static_cast <char> ( pCurrentSettingsFile ->sUsername .size() );

    newSettingsFile .write
            ( reinterpret_cast <char*> (&cUserNameLength), sizeof(cUserNameLength) );

    newSettingsFile .write
            ( pCurrentSettingsFile ->sUsername .c_str(), cUserNameLength );



    // Write theme name.
    unsigned char cThemeLength = static_cast <unsigned char> ( pCurrentSettingsFile ->sThemeName .size() );

    newSettingsFile .write
            ( reinterpret_cast <char*> (&cThemeLength),  sizeof(cThemeLength) );

    newSettingsFile .write
            ( pCurrentSettingsFile ->sThemeName .c_str(), cThemeLength );



    // Write push-to-Talk press/unpress sound.
    char cPlayPushToTalkSound = pCurrentSettingsFile ->bPlayPushToTalkSound;

    newSettingsFile .write
            ( &cPlayPushToTalkSound, sizeof(cPlayPushToTalkSound) );



    // Write connect string.
    unsigned char cConnectStringSize = static_cast <unsigned char>( pCurrentSettingsFile ->sConnectString.size() );

    newSettingsFile .write
            ( reinterpret_cast <char*> (&cConnectStringSize), sizeof(cConnectStringSize) );

    newSettingsFile .write
            ( const_cast <char*>       (pCurrentSettingsFile ->sConnectString .c_str()), cConnectStringSize );



    // Write port.
    newSettingsFile .write
            ( reinterpret_cast <char*> (&pCurrentSettingsFile ->iPort), sizeof(pCurrentSettingsFile ->iPort) );



    // Write password.
    unsigned char cPassSize = static_cast <unsigned char>( pCurrentSettingsFile ->sPassword.size() );

    newSettingsFile .write
            ( reinterpret_cast <char*> (&cPassSize), sizeof(cPassSize) );

    newSettingsFile .write
            ( reinterpret_cast <char*>( const_cast<wchar_t*>(pCurrentSettingsFile ->sPassword .c_str()) ), cPassSize * sizeof(wchar_t) );



    // Write input device name.
    unsigned char cInputDeviceNameSize = static_cast <unsigned char>( pCurrentSettingsFile ->sInputDeviceName.size() );

    newSettingsFile .write
            ( reinterpret_cast <char*> (&cInputDeviceNameSize), sizeof(cInputDeviceNameSize) );

    newSettingsFile .write
            ( reinterpret_cast <char*>( const_cast<wchar_t*>(pCurrentSettingsFile ->sInputDeviceName .c_str()) ), cInputDeviceNameSize * sizeof(wchar_t) );



    // Write input volume multiplier.
    newSettingsFile .write( reinterpret_cast<char*>(&pCurrentSettingsFile ->iInputVolumeMultiplier), sizeof(pCurrentSettingsFile ->iInputVolumeMultiplier));



    // NEW SETTINGS GO HERE
    // Don't forget to update "readSettings()".



    if ( settingsFile .is_open() )
    {
        // Close files and rename the new file.

        settingsFile    .close();
        newSettingsFile .close();

        _wremove( sPathToOldSettings .c_str() );

        _wrename( sPathToNewSettings .c_str(), sPathToOldSettings .c_str() );
    }
    else
    {
        newSettingsFile .close();
    }



    // Update theme if it was changed.

    if (bInit == false)
    {
       pMainWindow ->applyTheme();
    }
}


SettingsFile *SettingsManager::readSettings()
{
    // Get the path to the Documents folder.

    TCHAR   my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathW( nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, my_documents );

    if (result != S_OK)
    {
        pMainWindow ->showMessageBox(true, "Can't open the Documents folder to read the settings.");

        return nullptr;
    }




    // Delete FChatSettings (FChat - old version) if they are exists
    std::wstring adressToOldSettings = std::wstring(my_documents);
    adressToOldSettings += L"\\FChatSettings.data";

    std::ifstream oldSettings (adressToOldSettings);
    if (oldSettings .is_open())
    {
        oldSettings .close();
        _wremove ( adressToOldSettings .c_str() );
    }




    // Create settings file object

    SettingsFile* pSettingsFile = new SettingsFile();





    // Open the settings file.

    std::wstring addressToSettings = std::wstring(my_documents);
    addressToSettings             += L"\\" + std::wstring(SETTINGS_NAME);

    std::ifstream settingsFile (addressToSettings, std::ios::binary);

    if ( settingsFile .is_open() )
    {
        // Read the settings.


        // Read magic number.

        int iMagicNumber = 0;
        settingsFile .read( reinterpret_cast<char*>(&iMagicNumber), sizeof(iMagicNumber) );

        if (iMagicNumber != SILENT_MAGIC_NUMBER)
        {
            using convert_type = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_type, wchar_t> converter;

            std::string converted_addr_str = converter.to_bytes( addressToSettings );

            pMainWindow ->showMessageBox(true, "An error occurred at SettingsManager::readSettings(). Error: "
                                               "the settings file, located at \"" + converted_addr_str + "\" is not a Silent settings file "
                                               "(it may have an old format from the older version).\n"
                                               "\n"
                                               "This file will be deleted and replaced with the valid Silent settings file. No further actions required.");
            settingsFile .close();
            _wremove ( addressToSettings .c_str() );

            bSettingsFileCreatedFirstTime = true;

            return pSettingsFile;
        }


        // Read settings version.

        unsigned short int iSettingsVersion = SILENT_SETTINGS_FILE_VERSION;

        settingsFile .read( reinterpret_cast<char*>(&iSettingsVersion), sizeof(iSettingsVersion) );



        // ------------------------------------------------
        // ------------------------------------------------
        //           HANDLE OLD VERSIONS HERE
        // ------------------------------------------------
        // ------------------------------------------------



        // Read push-to-talk button.
        settingsFile .read( reinterpret_cast <char*> (&pSettingsFile ->iPushToTalkButton), sizeof(pSettingsFile ->iPushToTalkButton) );



        // Read master volume.
        settingsFile .read( reinterpret_cast <char*> (&pSettingsFile ->iMasterVolume),     sizeof(pSettingsFile ->iMasterVolume) );



        // Read user name length.
        unsigned char cUserNameLength = 0;
        settingsFile .read( reinterpret_cast <char*> (&cUserNameLength), sizeof(cUserNameLength) );

        // Read user name.
        char vBuffer[UCHAR_MAX];
        memset(vBuffer, 0, UCHAR_MAX);

        settingsFile .read( vBuffer, cUserNameLength );

        pSettingsFile ->sUsername = vBuffer;



        // Read theme name length.
        unsigned char cThemeLength = 0;
        settingsFile .read( reinterpret_cast <char*> (&cThemeLength), sizeof(cThemeLength) );

        // Read theme name.
        memset(vBuffer, 0, UCHAR_MAX);

        settingsFile .read( vBuffer, cThemeLength );

        pSettingsFile ->sThemeName = vBuffer;



        // Read push-to-talk sound enabled.
        char cPushToTalkSoundEnabled = 0;
        settingsFile .read( &cPushToTalkSoundEnabled, sizeof(cPushToTalkSoundEnabled) );

        pSettingsFile ->bPlayPushToTalkSound = cPushToTalkSoundEnabled;



        // Read connect string.
        unsigned char cConnectStringSize = 0;
        settingsFile .read( reinterpret_cast<char*>(&cConnectStringSize), sizeof(cConnectStringSize) );

        memset(vBuffer, 0, UCHAR_MAX);
        settingsFile .read(vBuffer, cConnectStringSize);

        pSettingsFile ->sConnectString = vBuffer;



        // Read port.
        settingsFile .read( reinterpret_cast<char*>(&pSettingsFile ->iPort), sizeof(pSettingsFile ->iPort));



        // Read password.
        unsigned char cPasswordSize = 0;
        settingsFile .read( reinterpret_cast<char*>(&cPasswordSize), sizeof(cPasswordSize) );

        wchar_t vWBuffer[UCHAR_MAX];
        memset(vWBuffer, 0, UCHAR_MAX * sizeof(wchar_t));

        settingsFile .read(reinterpret_cast<char*>(vWBuffer), cPasswordSize * sizeof(wchar_t));

        pSettingsFile ->sPassword = vWBuffer;



        // Read input device name.
        unsigned char cInputDeviceSize = 0;
        settingsFile .read( reinterpret_cast<char*>(&cInputDeviceSize), sizeof(cInputDeviceSize) );

        wchar_t vDeviceBuffer[UCHAR_MAX];
        memset(vDeviceBuffer, 0, UCHAR_MAX * sizeof(wchar_t));

        settingsFile .read(reinterpret_cast<char*>(vDeviceBuffer), cInputDeviceSize * sizeof(wchar_t));

        pSettingsFile ->sInputDeviceName = vDeviceBuffer;



        // Read input volume multiplier.
        settingsFile .read( reinterpret_cast<char*>(&pSettingsFile ->iInputVolumeMultiplier), sizeof(pSettingsFile ->iInputVolumeMultiplier));



        settingsFile .close();
    }
    else
    {
        // The settings file does not exist.
        // Create one and write default settings.

        bSettingsFileCreatedFirstTime = true;
    }



    return pSettingsFile;
}

SettingsFile *SettingsManager::getCurrentSettings()
{
    return pCurrentSettingsFile;
}

bool SettingsManager::isSettingsCreatedFirstTime()
{
    return bSettingsFileCreatedFirstTime;
}

SettingsManager::~SettingsManager()
{
    if (pCurrentSettingsFile)
    {
        delete pCurrentSettingsFile;
    }
}

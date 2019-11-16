#include "settingsmanager.h"


// STL
#include <fstream>
#include <shlobj.h>

// Custom
#include "../src/View/MainWindow/mainwindow.h"
#include "../src/Model/SettingsManager/SettingsFile.h"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


SettingsManager::SettingsManager(MainWindow* pMainWindow)
{
    this ->pMainWindow   = pMainWindow;


    bSettingsFileCreatedFirstTime = false;


    pCurrentSettingsFile = nullptr;
    pCurrentSettingsFile = readSettings();
}





void SettingsManager::saveSettings(SettingsFile* pSettingsFile, bool bSetOnlyNewUserName)
{
    // Get the path to the Documents folder.

    TCHAR   my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathW( nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, my_documents );

    if (result != S_OK)
    {
        pMainWindow ->showMessageBox(true, "Can't open the Documents folder to read the settings.");

        delete pSettingsFile;

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




    if ( bSetOnlyNewUserName )
    {
        pSettingsFile ->iPushToTalkButton = pCurrentSettingsFile ->iPushToTalkButton;
        pSettingsFile ->iMasterVolume     = pCurrentSettingsFile ->iMasterVolume;
        pSettingsFile ->sThemeName        = pCurrentSettingsFile ->sThemeName;
    }




    // Write new setting to the new file.

    // Push-to-Talk button
    newSettingsFile .write
            ( reinterpret_cast <char*> (&pSettingsFile ->iPushToTalkButton), sizeof(pSettingsFile ->iPushToTalkButton) );

    // Master Volume
    newSettingsFile .write
            ( reinterpret_cast <char*> (&pSettingsFile ->iMasterVolume),     sizeof(pSettingsFile ->iMasterVolume)     );




    if ( bSetOnlyNewUserName )
    {
        char cUserNameLength = static_cast <char> ( pSettingsFile ->sUsername .size() );

        newSettingsFile .write
                ( reinterpret_cast <char*> (&cUserNameLength),          sizeof(cUserNameLength) );

        newSettingsFile .write
                ( const_cast <char*> (pSettingsFile ->sUsername .c_str()), cUserNameLength );
    }
    else
    {
        if ( settingsFile .is_open() )
        {
            // Open the old file to copy user name.

            int iOldFileSize = 0;

            settingsFile .seekg(0, std::ios::end);
            iOldFileSize     = static_cast <int> ( settingsFile .tellg() );
            settingsFile .seekg( sizeof(pSettingsFile ->iPushToTalkButton) + sizeof(pSettingsFile ->iMasterVolume) );



            iOldFileSize    -= ( sizeof(pSettingsFile ->iPushToTalkButton)
                                 + sizeof(pSettingsFile ->iMasterVolume) );




            // Copy user name

            char cUserNameLength = 0;
            settingsFile .read ( reinterpret_cast <char*> (&cUserNameLength), sizeof(cUserNameLength) );

            char vBuffer[UCHAR_MAX];
            memset(vBuffer, 0, UCHAR_MAX);

            settingsFile .read( vBuffer, cUserNameLength );

            pSettingsFile ->sUsername = vBuffer;




            // Write this user name

            newSettingsFile .write
                    ( reinterpret_cast <char*> (&cUserNameLength),          sizeof(cUserNameLength) );

            newSettingsFile .write
                    ( const_cast <char*> (pSettingsFile ->sUsername .c_str()), cUserNameLength );
        }
        else
        {
            // User name: no name
            unsigned char cNameSize = 0;
            newSettingsFile .write( reinterpret_cast <char*> (&cNameSize), sizeof(cNameSize) );
        }
    }


    // Theme name
    unsigned char cThemeLength = static_cast <unsigned char> ( pSettingsFile ->sThemeName .size() );

    newSettingsFile .write
            ( reinterpret_cast <char*> (&cThemeLength),          sizeof(cThemeLength) );

    newSettingsFile .write
            ( const_cast <char*>       (pSettingsFile ->sThemeName .c_str()), cThemeLength );

    // NEW SETTINGS GO HERE
    // + don't forget to update "if ( bSetOnlyNewUserName )" above!
    // + don't forget to update "readSettings()"





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




    // Save the old theme

    std::string sOldTheme = "";

    if (pCurrentSettingsFile)
    {
        sOldTheme = pCurrentSettingsFile ->sThemeName;
    }
    else
    {
        sOldTheme = pSettingsFile ->sThemeName;
    }

    // Update our 'pCurrentSettingsFile' to new settings

    // Careful here
    // AudioService takes settings (calls getSettings()) very often.

    mtxRefreshSettings .lock();


    if (pCurrentSettingsFile)
    {
        delete pCurrentSettingsFile;
        pCurrentSettingsFile = nullptr;
    }


    pCurrentSettingsFile = pSettingsFile;

    mtxRefreshSettings .unlock();




    // Update theme if it was changed

    if ( pCurrentSettingsFile ->sThemeName != sOldTheme )
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

    std::wstring adressToSettings = std::wstring(my_documents);
    adressToSettings             += L"\\" + std::wstring(SETTINGS_NAME);

    std::ifstream settingsFile (adressToSettings, std::ios::binary);

    if ( settingsFile .is_open() )
    {
        // Read the settings.



        // Read push-to-talk button
        settingsFile .read( reinterpret_cast <char*> (&pSettingsFile ->iPushToTalkButton), sizeof(pSettingsFile ->iPushToTalkButton) );



        // Read master volume
        settingsFile .read( reinterpret_cast <char*> (&pSettingsFile ->iMasterVolume),     sizeof(pSettingsFile ->iMasterVolume) );



        // Read user name length
        unsigned char cUserNameLength = 0;
        settingsFile .read( reinterpret_cast <char*> (&cUserNameLength), sizeof(cUserNameLength) );

        // Read user name
        char vBuffer[UCHAR_MAX];
        memset(vBuffer, 0, UCHAR_MAX);

        settingsFile .read( vBuffer, cUserNameLength );

        pSettingsFile ->sUsername = vBuffer;



        // Read theme length
        unsigned char cThemeLength = 0;
        settingsFile .read( reinterpret_cast <char*> (&cThemeLength), sizeof(cThemeLength) );

        // Read theme name
        memset(vBuffer, 0, UCHAR_MAX);

        settingsFile .read( vBuffer, cThemeLength );

        pSettingsFile ->sThemeName = vBuffer;



        settingsFile .close();
    }
    else
    {
        // The settings file does not exist.
        // Create one and write default settings.


        bSettingsFileCreatedFirstTime = true;


        saveSettings(pSettingsFile);
    }



    return pSettingsFile;
}

SettingsFile *SettingsManager::getCurrentSettings()
{
    mtxRefreshSettings .lock();
    mtxRefreshSettings .unlock();

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

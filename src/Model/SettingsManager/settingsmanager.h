#pragma once

// STL
#include <mutex>

class MainWindow;
class SettingsFile;


#define SETTINGS_NAME L"SilentSettings.data"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class SettingsManager
{

public:

    SettingsManager(MainWindow* pMainWindow);


    void           saveSettings (SettingsFile* pSettingsFile, bool bSetOnlyNewUserName = false);

    SettingsFile*  getCurrentSettings();
    bool           isSettingsCreatedFirstTime();


    ~SettingsManager();

private:

    SettingsFile*  readSettings  ();


    // ---------------------------------------



    MainWindow*        pMainWindow;
    SettingsFile*      pCurrentSettingsFile;


    std::mutex         mtxRefreshSettings;


    bool               bSettingsFileCreatedFirstTime;
};

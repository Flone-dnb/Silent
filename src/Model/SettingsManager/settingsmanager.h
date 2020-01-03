// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

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

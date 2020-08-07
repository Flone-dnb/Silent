// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// Qt
#include <QMainWindow>


class QKeyEvent;
class QMouseEvent;
class SettingsManager;

namespace Ui
{
    class SettingsWindow;
}


// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


class SettingsWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit SettingsWindow                        (SettingsManager* pSettingsManager, std::vector<QString> vInputDevices, QWidget *parent = nullptr);

    ~SettingsWindow                                ();

signals:

    void  applyNewMasterVolume                     ();

protected:

    void  keyPressEvent                            (QKeyEvent*   event);
    void  mousePressEvent                          (QMouseEvent* event);

private slots:

    void  on_horizontalSlider_volume_valueChanged  (int value);
    void  on_pushButton_pushtotalk_clicked         ();
    void  on_pushButton_2_clicked                  ();

private:

    void  updateUIToSettings                       (std::vector<QString> vInputDevices);
    void  showThemes                               ();
    void  refreshVolumeSliderText                  ();



    // ----------------------------------------------

    SettingsManager*    pSettingsManager;


    // Push-to-Talk button
    int                 iPushToTalkKey;
    unsigned short int  iMasterVolume;
    bool                bWaitingForPushToTalkButtonInput;


    // Changed settings
    bool                bPushToTalkChanged;
    bool                bMasterVolumeChanged;


    // Original settings
    int                 iOriginalPushToTalkButton;
    unsigned short int  iOriginalMasterVolume;


    Ui::SettingsWindow* ui;
};

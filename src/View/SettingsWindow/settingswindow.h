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
    void  closedSettingsWindow                     ();
    void  signalSetAudioInputVolume                (int iVolume);
    void  signalSetVoiceStartValue                 (int iValue);
    void  signalSetShouldHearTestVoice             (bool bHear);

protected:

    void  keyPressEvent                            (QKeyEvent*   event);
    void  mousePressEvent                          (QMouseEvent* event);
    void  closeEvent                               (QCloseEvent* event);

public slots:

    void  slotSetVoiceVolume                       (int iVolume);

private slots:

    void  on_horizontalSlider_volume_valueChanged  (int value);
    void  on_pushButton_pushtotalk_clicked         ();
    void  on_pushButton_2_clicked                  ();

    void on_comboBox_input_currentIndexChanged(int index);
    void on_horizontalSlider_input_volume_mult_valueChanged(int position);
    void on_horizontalSlider_start_voice_rec_valueChanged(int value);

    void on_voiceVolumeMeter_valueChanged(int value);

    void on_comboBox_voice_mode_currentIndexChanged(int index);

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
    int                 iInputVolumeMult;


    // Original settings
    int                 iOriginalPushToTalkButton;
    unsigned short int  iOriginalMasterVolume;


    Ui::SettingsWindow* ui;


    bool                bShowedWarning;
    bool                bInit;
};

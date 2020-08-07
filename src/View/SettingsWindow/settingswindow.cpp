// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "settingswindow.h"
#include "ui_settingswindow.h"


// Qt
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QDir>

// Custom
#include "View/StyleAndInfoPaths.h"
#include "Model/SettingsManager/settingsmanager.h"
#include "Model/SettingsManager/SettingsFile.h"


// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


SettingsWindow::SettingsWindow(SettingsManager* pSettingsManager,  std::vector<QString> vInputDevices, QWidget *parent) : QMainWindow(parent), ui(new Ui::SettingsWindow)
{
    ui ->setupUi(this);
    setFixedSize( width(), height() );

    bInit = true;

    this->pSettingsManager = pSettingsManager;


    bWaitingForPushToTalkButtonInput = false;
    bPushToTalkChanged               = false;
    bMasterVolumeChanged             = false;

    updateUIToSettings(vInputDevices);

    bInit = false;
}





void SettingsWindow::keyPressEvent(QKeyEvent *event)
{
    if ( bWaitingForPushToTalkButtonInput )
    {
        int key = -1;

        if ( (event ->key() >= 0x41)
             &&
             (event ->key() <= 0x5a) )
        {
            // From A to Z

            key = event ->key();

            ui ->pushButton_pushtotalk ->setText( event ->text() .toUpper() );
        }
        else if ( event ->key() == Qt::Key_Alt )
        {
            // 0x12 Virtual Code for GetAsyncKeyState

            key = 0x12;

            ui ->pushButton_pushtotalk ->setText("Alt");
        }

        if (key != -1)
        {
            iPushToTalkKey     = key;
            bWaitingForPushToTalkButtonInput = false;
            bPushToTalkChanged = true;
        }
    }
}

void SettingsWindow::mousePressEvent(QMouseEvent *event)
{
    if (bWaitingForPushToTalkButtonInput)
    {
        int key = -1;

        if      ( event ->button() == Qt::XButton1 )
        {
            // 0x05 Virtual Code for GetAsyncKeyState

            key = 0x05;

            ui ->pushButton_pushtotalk ->setText("X1");
        }
        else if ( event ->button() == Qt::XButton2 )
        {
            // 0x06 Virtual Code for GetAsyncKeyState

            key = 0x06;

            ui ->pushButton_pushtotalk ->setText("X2");
        }

        if (key != -1)
        {
            iPushToTalkKey     = key;
            bWaitingForPushToTalkButtonInput = false;
            bPushToTalkChanged = true;
        }
    }
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    emit closedSettingsWindow();

    if (pSettingsManager ->getCurrentSettings() ->iInputVolumeMultiplier != iInputVolumeMult)
    {
        emit signalSetAudioInputVolume(pSettingsManager ->getCurrentSettings() ->iInputVolumeMultiplier);
    }

    deleteLater();
}

void SettingsWindow::slotSetVoiceVolume(int iVolume)
{
    ui ->voiceVolumeMeter ->setValue(iVolume);
}

void SettingsWindow::on_pushButton_pushtotalk_clicked()
{
    ui ->pushButton_pushtotalk ->setText("Waiting for button");

    bWaitingForPushToTalkButtonInput = true;
}

void SettingsWindow::refreshVolumeSliderText()
{
    // 0%    -  ui ->horizontalSlider ->minimum()
    // 100%  -  ui ->horizontalSlider ->maximum()

    int iInterval = ui ->horizontalSlider_volume ->maximum()  -  ui ->horizontalSlider_volume ->minimum();
    int iCurrent  = ui ->horizontalSlider_volume ->value();

    iMasterVolume = static_cast <unsigned short int> (iCurrent);

    ui ->label_4 ->setText ( QString::number( (iCurrent / static_cast<double>(iInterval) * 100), 'f', 0 ) + "%" );
}

void SettingsWindow::on_horizontalSlider_volume_valueChanged(int value)
{
    Q_UNUSED(value)

    refreshVolumeSliderText();

    bMasterVolumeChanged = true;
}



SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::on_pushButton_2_clicked()
{
    SettingsFile* pSettingsFile = pSettingsManager ->getCurrentSettings();

    if (bPushToTalkChanged)
    {
        pSettingsFile ->iPushToTalkButton = iPushToTalkKey;
    }

    if (bMasterVolumeChanged)
    {
        pSettingsFile ->iMasterVolume = iMasterVolume;
    }

    pSettingsFile ->sThemeName = ui ->comboBox_themes ->currentText() .toStdString();

    pSettingsFile ->bPlayPushToTalkSound = ui ->checkBox_pushToTalkSound ->isChecked();

    if (ui ->comboBox_input ->currentIndex() != 0)
    {
        pSettingsFile ->sInputDeviceName = ui ->comboBox_input ->currentText() .toStdWString();
    }

    pSettingsFile ->iInputVolumeMultiplier = ui ->horizontalSlider_input_volume_mult ->value();

    pSettingsManager ->saveCurrentSettings();

    emit applyNewMasterVolume();

    close();
}

void SettingsWindow::updateUIToSettings(std::vector<QString> vInputDevices)
{
    SettingsFile* pSettingsFile = pSettingsManager ->getCurrentSettings();

    iInputVolumeMult = pSettingsFile ->iInputVolumeMultiplier;

    ui ->label_input_voice_mult ->setText(QString::number(iInputVolumeMult) + "%");
    ui ->horizontalSlider_input_volume_mult ->setValue(iInputVolumeMult);

    ui ->pushButton_pushtotalk   ->setText  ( QString::fromStdString( pSettingsFile ->getPushToTalkButtonName() ) );
    ui ->horizontalSlider_volume ->setValue ( pSettingsFile ->iMasterVolume );

    showThemes();

    ui ->comboBox_themes ->setCurrentText( QString::fromStdString(pSettingsFile ->sThemeName) );

    ui ->checkBox_pushToTalkSound ->setChecked( pSettingsFile ->bPlayPushToTalkSound );


    ui ->comboBox_input ->addItem("Auto-select");

    for (size_t i = 0; i < vInputDevices.size(); i++)
    {
        ui ->comboBox_input ->addItem(vInputDevices[i]);
    }

    ui ->comboBox_input ->setCurrentIndex(0);

    for (size_t i = 0; i < vInputDevices.size(); i++)
    {
        if (vInputDevices[i] == pSettingsFile->sInputDeviceName)
        {
            ui ->comboBox_input ->setCurrentIndex(1 + i);
            break;
        }
    }
}

void SettingsWindow::showThemes()
{
    QDir directory(STYLE_THEMES_PATH_FROM_EXE);
    QStringList themes = directory .entryList(QDir::Files);

    for (int i = 0;   i < themes .size();   i++)
    {
        int pos = themes[i].lastIndexOf(QChar('.'));
        ui ->comboBox_themes ->addItem(themes[i].left(pos));
    }
}


void SettingsWindow::on_comboBox_input_currentIndexChanged(int index)
{
    if (bInit == false)
    {
        QMessageBox::warning(this, "Notice", "The audio input device will be changed only after the program is restarted!");
    }
}

void SettingsWindow::on_horizontalSlider_input_volume_mult_sliderMoved(int position)
{
    ui ->label_input_voice_mult ->setText(QString::number(position) + "%");

    emit signalSetAudioInputVolume(position);
}
